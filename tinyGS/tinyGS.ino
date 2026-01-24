/***********************************************************************
  tinyGS.ini - GroundStation firmware
  
  Copyright (C) 2020 -2021 @G4lile0, @gmag12 and @dev_4m1g0

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ***********************************************************************

  TinyGS is an open network of Ground Stations distributed around the
  world to receive and operate LoRa satellites, weather probes and other
  flying objects, using cheap and versatile modules.

  This project is based on ESP32 boards and is compatible with sx126x and
  sx127x you can build you own board using one of these modules but most
  of us use a development board like the ones listed in the Supported
  boards section.

  Supported boards
    Heltec WiFi LoRa 32 V1 (433MHz & 863-928MHz versions)
    Heltec WiFi LoRa 32 V2 (433MHz & 863-928MHz versions)
    TTGO LoRa32 V1 (433MHz & 868-915MHz versions)
    TTGO LoRa32 V2 (433MHz & 868-915MHz versions)
    TTGO LoRa32 V2 (Manually swapped SX1267 to SX1278)
    T-BEAM + OLED (433MHz & 863-928MHz versions)
    T-BEAM V1.0 + OLED
    LilyGO T-Beam Supreme (ESP32-S3, SX1262, GPS, PSRAM)
    FOSSA 1W Ground Station (433MHz & 863-928MHz versions)
    ESP32 dev board + SX126X with crystal (Custom build, OLED optional)
    ESP32 dev board + SX126X with TCXO (Custom build, OLED optional)
    ESP32 dev board + SX127X (Custom build, OLED optional)

  Supported modules
    sx126x
    sx127x

    Web of the project: https://tinygs.com/
    Github: https://github.com/G4lile0/tinyGS
    Main community chat: https://t.me/joinchat/DmYSElZahiJGwHX6jCzB3Q

    In order to onfigure your Ground Station please open a private chat to get your credentials https://t.me/tinygs_personal_bot
    Data channel (station status and received packets): https://t.me/tinyGS_Telemetry
    Test channel (simulator packets received by test groundstations): https://t.me/TinyGS_Test

    Developers:
      @gmag12       https://twitter.com/gmag12
      @dev_4m1g0    https://twitter.com/dev_4m1g0
      @g4lile0      https://twitter.com/G4lile0

====================================================
  IMPORTANT:
    - Follow this guide to get started: https://github.com/G4lile0/tinyGS/wiki/Quick-Start
    - Arduino IDE is NOT recommended, please use Platformio: https://github.com/G4lile0/tinyGS/wiki/Platformio

**************************************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "src/ConfigManager/ConfigManager.h"
#include "src/Display/Display.h"
#include "src/Mqtt/MQTT_Client.h"
#include "src/Status.h"
#include "src/Radio/Radio.h"
#include "src/ArduinoOTA/ArduinoOTA.h"
#include "src/OTA/OTA.h"
#include "src/Logger/Logger.h"
#include "time.h"
#include "src/Mqtt/MQTT_credentials.h"
#include "src/Improv/tinygs_improv.h"
#include "src/GnssManager/GnssManager.h"


#if  RADIOLIB_VERSION_MAJOR != (0x07) || RADIOLIB_VERSION_MINOR != (0x05) || RADIOLIB_VERSION_PATCH != (0x00) || RADIOLIB_VERSION_EXTRA != (0x00)
#error "You are not using the correct version of RadioLib please copy TinyGS/lib/RadioLib on Arduino/libraries"
#endif

#ifndef RADIOLIB_GODMODE
#if !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /RadioLib/src/BuildOpt.h and uncomment #define RADIOLIB_GODMODE around line 367" 
#endif
#endif

ConfigManager& configManager = ConfigManager::getInstance();
MQTT_Client& mqtt = MQTT_Client::getInstance();
Radio& radio = Radio::getInstance ();
//TinyGSImprov improvWiFi = TinyGSImprov ();

const char* ntpServer = "time.cloudflare.com";

// Global status
Status status;

void printControls();
void switchTestmode();
void checkButton();
void setupNTP ();
void handleSerial ();
void handleRawSerial ();
void checkStationStatus();

void wifiConnected()
{
  Log::console(PSTR("WiFi Connected! IP: %s"), WiFi.localIP().toString().c_str());
  setupNTP();
  arduino_ota_setup();
  displayShowConnected();
}

void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Log::console(PSTR("WiFi Disconnected! Reason: %d"), WiFi.status());
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Log::console(PSTR("WiFi Station Started"));
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Log::console(PSTR("WiFi Station Connected"));
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Log::console(PSTR("WiFi Station Got IP: %s"), WiFi.localIP().toString().c_str());
            break;
        default:
            break;
    }
}

void configured()
{
  configManager.setConfiguredCallback(NULL);
  configManager.printConfig();
  radio.init();
}

void setup()
{ 
#if CONFIG_IDF_TARGET_ESP32C3
  setCpuFrequencyMhz(160);
#else
  setCpuFrequencyMhz(240);
#endif
  Serial.begin(115200);
  WiFi.onEvent(onWiFiEvent);
  delay (100);
  
  // Initialize async logging early
  Log::initAsync();
  Log::setLogLevel(Log::LOG_LEVEL_DEBUG);
  
  improvWiFi.setVersion (status.version);
  Log::console (PSTR ("TinyGS Version %d - %s"), status.version, status.git_version);
  Log::console(PSTR("Chip  %s - %d"),  ESP.getChipModel(),ESP.getChipRevision());
  if ((configManager.getMqttServer ()[0] == '\0') || (configManager.getMqttUser ()[0] == '\0') || (configManager.getMqttPass ()[0] == '\0')) {
      mqttCredentials.generateOTPCode ();
  }
  setupNTP(); // Initialize NTP and Timezone
  configManager.setWifiConnectionCallback(wifiConnected);
  configManager.setConfiguredCallback(configured);
  configManager.init();
  Power::getInstance().checkAXP();
  radio.init(); // Initialize radio hardware early
  GnssManager::getInstance().begin();
  
  if (strlen(configManager.getWiFiSSID()) > 0) {
      Log::console(PSTR("WiFi credentials found, forcing station mode..."));
      WiFi.mode(WIFI_STA);
      WiFi.begin(configManager.getWiFiSSID(), configManager.getWiFiPassword());
      configManager.forceApMode(false);
  }

  if (configManager.isFailSafeActive())
  {
    configManager.setConfiguredCallback(NULL);
    configManager.setWifiConnectionCallback(NULL);
    Log::console(PSTR("FATAL ERROR: The board is in a boot loop, rescue mode launched. Connect to the WiFi AP: %s, and open a web browser on ip 192.168.4.1 to fix your configuration problem or upload a new firmware."), configManager.getThingName());
    return;
  }
  // make sure to call doLoop at least once before starting to use the configManager
  configManager.doLoop();
  board_t board;
  if(configManager.getBoardConfig(board))
    pinMode (board.PROG__BUTTON, INPUT_PULLUP);
  displayInit();
  displayShowInitialCredits();
  configManager.delay(1000);
  mqtt.begin ();

  if (configManager.getOledBright() == 0)
  {
    displayTurnOff();
  }
  
  printControls();
}

bool mqttAutoconf () {
    const time_t AUTOCONFIG_TIMEOUT = 30 * 60 * 1000;
    if (configManager.getState () == IOTWEBCONF_STATE_ONLINE) {
        static time_t started_autoconf = millis ();
        if (millis () - started_autoconf > AUTOCONFIG_TIMEOUT) {
            Log::console (PSTR ("Autoconfig timeout, please check your internet connection and restart the board"));
            delay (500);
            esp_deep_sleep_start ();
            return false;
        }
        String result = mqttCredentials.fetchCredentials ();
        if (result == "") {
          //Log::console(PSTR("Error fetching credentials, please check your internet connection and try again"));
            return false;
        }
        Log::debug ("result: %s", result.c_str ());
        delay (1000);
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson (doc, result);
        if (error) {
            Log::console ("deserializeJson() failed: %s", error.c_str ());
            return false;
        }

        if (doc.containsKey ("mqtt") &&
            doc["mqtt"].containsKey ("user") &&
            doc["mqtt"].containsKey ("pass") &&
            doc["mqtt"].containsKey ("server") &&
            doc["mqtt"].containsKey ("port")) {
            const char* mqttUser = doc["mqtt"]["user"];
            const char* mqttPass = doc["mqtt"]["pass"];
            const char* mqttServer = doc["mqtt"]["server"];
            const char* mqttPort = doc["mqtt"]["port"];

            Log::debug ("MQTT User: %s", mqttUser);
            Log::debug ("MQTT Pass: %s", mqttPass);
            Log::debug ("MQTT Server: %s", mqttServer);
            Log::debug ("MQTT Port: %s", mqttPort);

            strncpy (configManager.getMqttServerParameter ()->valueBuffer, mqttServer, configManager.getMqttServerParameter ()->getLength ());
            strncpy (configManager.getMqttUserParameter ()->valueBuffer, mqttUser, configManager.getMqttUserParameter ()->getLength ());
            strncpy (configManager.getMqttPassParameter ()->valueBuffer, mqttPass, configManager.getMqttPassParameter ()->getLength ());
            strncpy (configManager.getMqttPortParameter ()->valueBuffer, mqttPort, configManager.getMqttPortParameter ()->getLength ());
        } else {
            Log::console (PSTR ("Invalid JSON format"));
            return false;
        }

        if (doc.containsKey ("config")) {
            if (doc["config"].containsKey ("stationName")) {
                const char* stationName = doc["config"]["stationName"];
                Log::console ("Station Name: %s", stationName);
                strncpy (configManager.getThingNameParameter ()->valueBuffer, stationName, configManager.getThingNameParameter ()->getLength ());
            }

            if (doc["config"].containsKey ("adminPw")) {
                const char* adminPw = doc["config"]["adminPw"];
                Log::console ("Admin Password: %s", adminPw);
                strncpy (configManager.getApPasswordParameter ()->valueBuffer, adminPw, configManager.getApPasswordParameter ()->getLength ());
            }
            if (doc["config"].containsKey ("latitude") && doc["config"].containsKey ("longitude")) {
                const char* latitude = doc["config"]["latitude"].as<const char*>();
                const char* longitude = doc["config"]["longitude"].as<const char*> ();
                Log::console ("Latitude: %s", latitude);
                Log::console("Longitude: %s", longitude);

                auto latParam = configManager.getLatitudeParameter();
                auto lonParam = configManager.getLongitudeParameter();

                if (latParam && lonParam && latitude && longitude) {
                    // Use snprintf for safety and null-termination
                    snprintf(latParam->valueBuffer, latParam->getLength(), "%s", latitude);
                    snprintf(lonParam->valueBuffer, lonParam->getLength(), "%s", longitude);
                } else {
                    Log::console("Error: Invalid latitude/longitude or parameter pointer");
                }
            }

            if (doc["config"].containsKey ("tz")) {
                const char* tz = doc["config"]["tz"];
                Log::console ("Timezone: %s", tz);
                strncpy (configManager.getTZParameter ()->valueBuffer, tz, configManager.getTZParameter ()->getLength ());
            }
        }

        configManager.saveConfig ();

        return true;
    }
    return false;
}
unsigned long lastTleRefresh = millis();

void loop() {  
    configManager.doLoop ();
    GnssManager::getInstance().loop();
    checkStationStatus();
    if (configManager.isFailSafeActive ())
  {
    static bool updateAttepted = false;
    if (!updateAttepted && configManager.isConnected()) {
      updateAttepted = true;
      OTA::update(); // try to update as last resource to recover from this state
    }

    if (millis() > 10000 || updateAttepted)
      configManager.forceApMode(true);
    
    return;
  }

  ArduinoOTA.handle();
  handleSerial();

  if (configManager.getState () < IOTWEBCONF_STATE_AP_MODE) // not ready or not configured
  {
      displayShowApMode ();
      return;
  }

  if ((configManager.getMqttServer ()[0] == '\0') || (configManager.getMqttUser ()[0] == '\0') || (configManager.getMqttPass ()[0] == '\0')) {
      mqttAutoconf ();
      return;
  }

  // configured and no connection
  checkButton();
  if (radio.isReady())
  {
    status.radio_ready = true;
    radio.listen();
  }
  else {
    status.radio_ready = false;
  }

  if (configManager.getState() < 4) // connection or ap mode
  {
    displayShowStaMode(configManager.isApMode());
    return;
  }

  // connected

  // Check for GNSS Location Updates
  if (GnssManager::getInstance().isLocationUpdated()) {
      Log::console(PSTR("GNSS: Location changed, sending status to MQTT..."));
      mqtt.sendStatus();
      GnssManager::getInstance().clearLocationUpdated();
  }

  // Attempt to fetch configuration if location is missing (e.g. first run after partial config)
  if (configManager.getLatitude() == 0.0 && configManager.getLongitude() == 0.0) {
      static bool triedLocFetch = false;
      if (!triedLocFetch) {
          Log::console(PSTR("Location missing. Attempting to fetch configuration from TinyGS server..."));
          mqttAutoconf();
          triedLocFetch = true;
      }
  }

  mqtt.loop();
  OTA::loop();

  displayUpdate ();

  if (configManager.askedWebLogin () && mqtt.connected ())
  {
      Log::debug (PSTR ("Getting weblogin in loop"));
      mqtt.sendWeblogin ();
  }
  
  // Update TLE
  unsigned long currentTime = millis();
  if (currentTime - lastTleRefresh >= status.tle.refresh) {
      lastTleRefresh = currentTime;
      radio.tle();
  }

}


void setupNTP()
{
  configTime(0, 0, ntpServer);
  setenv("TZ", configManager.getTZ(), 1); 
  tzset();
  
  configManager.delay(1000);
}

void checkButton()
{
  #define RESET_BUTTON_TIME 8000
  static unsigned long buttPressedStart = 0;
  board_t board;
  
  if (configManager.getBoardConfig(board) && !digitalRead (board.PROG__BUTTON))
  {
    displayResetTimeout();
    if (!buttPressedStart)
    {
      buttPressedStart = millis();
    }
    else if (millis() - buttPressedStart > RESET_BUTTON_TIME) // long press
    {
      Log::console(PSTR("Rescue mode forced by button long press!"));
      Log::console(PSTR("Connect to the WiFi AP: %s and open a web browser on ip 192.168.4.1 to configure your station and manually reboot when you finish."), configManager.getThingName());
      configManager.forceDefaultPassword(true);
      configManager.forceApMode(true);
      buttPressedStart = 0;
    }
  }
  else {
    unsigned long elapsedTime = millis() - buttPressedStart;
    if (elapsedTime > 30 && elapsedTime < 1000) // short press
      displayNextFrame();
    buttPressedStart = 0;
  }
}

void checkStationStatus() {
    // Trigger in NOT_CONFIGURED (1) or AP_MODE (2)
    uint8_t state = configManager.getState();
    if (state != IOTWEBCONF_STATE_NOT_CONFIGURED && state != IOTWEBCONF_STATE_AP_MODE) return;

    bool hasWifi = strlen(configManager.getWiFiSSID()) > 0;
    bool hasMqtt = (configManager.getMqttUser()[0] != '\0') && (configManager.getMqttPass()[0] != '\0');
    bool hasLoc = (configManager.getLatitude() != 0.0) || (configManager.getLongitude() != 0.0);
    bool hasName = strcmp(configManager.getThingName(), "My TinyGS") != 0;
    bool isConnected = (WiFi.status() == WL_CONNECTED);

    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 10000) {
        Log::console(PSTR("Status Check (State %d): WiFi:%d, MQTT:%d, Loc:%d, Name:%d, Connected:%d"), 
            state, hasWifi, hasMqtt, hasLoc, hasName, isConnected);
        lastDebug = millis();
    }

    if (hasWifi && hasMqtt && hasLoc && hasName && isConnected) {
        Log::console(PSTR("Auto-config: All parameters present and WiFi connected. Transitioning to ONLINE..."));
        configManager.changeState(IOTWEBCONF_STATE_ONLINE);
    }
}

void handleSerial () {
    while (Serial.available () > 0) {
        displayResetTimeout();
        yield ();
        byte next = Serial.peek ();
        if (next == 'I') {
            improvWiFi.handleImprovPacket ();
        } else {
            handleRawSerial ();
        }
    }
}

void handleRawSerial()
{
  if(Serial.available())
  {
    radio.disableInterrupt();

    // get the first character
    char serialCmd1 = Serial.read();
    char serialCmd = ' ';

    // wait for a bit to receive any trailing characters
    configManager.delay(500);
    if (serialCmd1 == '!') serialCmd = Serial.read();
    
    // process serial command
    switch(serialCmd) {
      case ' ':
      break;         
      case 'e':
        configManager.resetAllConfig();
        ESP.restart();
        break;
      case 'b':
        ESP.restart();
        break;
      case 'p':
        if (!radio.isReady())
        {
          Log::console(PSTR("Radio is not initialized! Check your configuration."));
          break;
        }
        if (!configManager.getAllowTx())
        {
          Log::console(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
          break;
        }

        static long lastTestPacketTime = 0;
        if (millis() - lastTestPacketTime < 20*1000)
        {
          Log::console(PSTR("Please wait a few seconds to send another test packet."));
          break;
        }
        
        radio.sendTestPacket();
        lastTestPacketTime = millis();
        Log::console(PSTR("Sending test packet to nearby stations!"));
        break;
      case 'w':
        Log::console (PSTR ("Getting weblogin"));
        mqtt.sendWeblogin ();
        break;
      case 'o':
          Log::console (PSTR ("OTP Code: %s"), mqttCredentials.getOTPCode ());
          break;
      case 'C': {
          // Format: !CSSID PASSWORD
          String payload = Serial.readStringUntil('\n');
          payload.trim();
          int spaceIdx = payload.indexOf(' ');
          if (spaceIdx != -1) {
              String ssid = payload.substring(0, spaceIdx);
              String pass = payload.substring(spaceIdx + 1);
              if (ssid.length() > 0) {
                  Log::console(PSTR("Setting WiFi to SSID: %s"), ssid.c_str());
                  strncpy(configManager.getWifiSsidParameter()->valueBuffer, ssid.c_str(), configManager.getWifiSsidParameter()->getLength());
                  strncpy(configManager.getWifiPasswordParameter()->valueBuffer, pass.c_str(), configManager.getWifiPasswordParameter()->getLength());
                  configManager.saveConfig();
                  Log::console(PSTR("Config saved. Rebooting..."));
                  delay(1000);
                  ESP.restart();
              }
          } else {
              Log::console(PSTR("Usage: !CSSID PASSWORD"));
          }
          break;
      }
      case 'M': {
          // Format: !M USER PASS
          String payload = Serial.readStringUntil('\n');
          payload.trim();
          int spaceIdx = payload.indexOf(' ');
          if (spaceIdx != -1) {
              String user = payload.substring(0, spaceIdx);
              String pass = payload.substring(spaceIdx + 1);
              if (user.length() > 0) {
                  Log::console(PSTR("Setting MQTT User: %s"), user.c_str());
                  strncpy(configManager.getMqttUserParameter()->valueBuffer, user.c_str(), configManager.getMqttUserParameter()->getLength());
                  strncpy(configManager.getMqttPassParameter()->valueBuffer, pass.c_str(), configManager.getMqttPassParameter()->getLength());
                  configManager.saveConfig();
                  Log::console(PSTR("Config saved. Rebooting..."));
                  delay(1000);
                  ESP.restart();
              }
          } else {
              Log::console(PSTR("Usage: !M USER PASS"));
          }
          break;
      }
      case 'L': {
          // Format: !L LAT LON
          String payload = Serial.readStringUntil('\n');
          payload.trim();
          int spaceIdx = payload.indexOf(' ');
          if (spaceIdx != -1) {
              String lat = payload.substring(0, spaceIdx);
              String lon = payload.substring(spaceIdx + 1);
              Log::console(PSTR("Setting Location: %s, %s"), lat.c_str(), lon.c_str());
              configManager.setLat(lat.c_str());
              configManager.setLon(lon.c_str());
              Log::console(PSTR("Location saved."));
          } else {
              Log::console(PSTR("Usage: !L LAT LON"));
          }
          break;
      }
      case 'T': {
          // Format: !T NAME
          String name = Serial.readStringUntil('\n');
          name.trim();
          if (name.length() > 0) {
              Log::console(PSTR("Setting Station Name: %s"), name.c_str());
              configManager.setName(name.c_str());
              Log::console(PSTR("Name saved. Rebooting..."));
              delay(1000);
              ESP.restart();
          }
          break;
      }
      case 's': {
          Log::console(PSTR("Station Name: %s"), configManager.getThingName());
          Log::console(PSTR("Lat/Lon: %.3f, %.3f"), configManager.getLatitude(), configManager.getLongitude());
          Log::console(PSTR("MQTT Server: %s"), configManager.getMqttServer());
          Log::console(PSTR("MQTT User: %s"), configManager.getMqttUser());
          Log::console(PSTR("MQTT Pass: %s"), strlen(configManager.getMqttPass()) > 0 ? "SET" : "EMPTY");
          Log::console(PSTR("WiFi SSID: %s"), configManager.getWiFiSSID());
          Log::console(PSTR("WiFi Status: %d"), WiFi.status());
          Log::console(PSTR("WiFi Mode: %d"), WiFi.getMode());
          Log::console(PSTR("WiFi IP: %s"), WiFi.localIP().toString().c_str());
          Log::console(PSTR("IotWebConf State: %d"), configManager.getState());
          
          Log::console(PSTR("I2C Scan (PMU Bus)..."));
          TwoWire* scanWire = Power::getInstance().getPmuWire();
          for (uint8_t adr = 1; adr < 127; adr++) {
              scanWire->beginTransmission(adr);
              if (scanWire->endTransmission() == 0) {
                  Log::console(PSTR("I2C Device at 0x%02X"), adr);
              }
          }

          Log::console(PSTR("Battery: %.2fV (%d%%)"), Power::getInstance().getBatteryVoltage()/1000.0, Power::getInstance().getBatteryPercentage());
          Log::console(PSTR("VBUS: %.2fV"), Power::getInstance().getVbusVoltage()/1000.0);
          Log::console(PSTR("AutoLoc: %d, Interval: %d"), configManager.getAutoLocation(), configManager.getGnssInterval());
          Log::console(PSTR("Radio Ready: %s"), radio.isReady() ? "YES" : "NO");
          break;
      }
      case 'W':
          Log::console(PSTR("Forcing WiFi Connect (Aggressive STA)..."));
          WiFi.disconnect();
          WiFi.mode(WIFI_STA);
          WiFi.begin(configManager.getWiFiSSID(), configManager.getWiFiPassword());
          configManager.forceApMode(false);
          break;
      case 'R':
          Log::console(PSTR("Manually initializing radio..."));
          radio.init();
          break;
      default:
        Log::console(PSTR("Unknown command: %c"), serialCmd);
        break;
    }

    radio.enableInterrupt();
  }
}

// function to print controls
void printControls()
{
  Log::console (PSTR ("------------- Controls -------------"));
  Log::console (PSTR ("!e - erase board config and reset"));
  Log::console (PSTR ("!b - reboot the board"));
  Log::console (PSTR ("!p - send test packet to nearby stations (to check transmission)"));
  Log::console (PSTR ("!w - ask for weblogin link"));
  Log::console (PSTR ("!o - get OTP code"));
  Log::console (PSTR ("!C SSID PASSWORD - set WiFi credentials"));
  Log::console (PSTR ("!M USER PASS - set MQTT credentials"));
  Log::console (PSTR ("!L LAT LON - set Station location (manual GNSS override)"));
  Log::console (PSTR ("!T NAME - set Station name"));
  Log::console (PSTR ("!W - force WiFi connection"));
  Log::console (PSTR ("!s - show status and scan I2C"));
  Log::console (PSTR ("------------------------------------"));
}