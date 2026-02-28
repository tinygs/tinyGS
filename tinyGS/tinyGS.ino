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
    T-BEAM + OLED (433MHz & 868-915MHz versions)
    T-BEAM V1.0 + OLED
    FOSSA 1W Ground Station (433MHz & 868-915MHz versions)
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
#include "src/Network/ConfigStore.h"
#include "src/Network/ConnectionManager.h"
#include "src/Network/WebServer.h"
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


#if  RADIOLIB_VERSION_MAJOR != (0x07) || RADIOLIB_VERSION_MINOR != (0x05) || RADIOLIB_VERSION_PATCH != (0x00) || RADIOLIB_VERSION_EXTRA != (0x00)
#error "You are not using the correct version of RadioLib please copy TinyGS/lib/RadioLib on Arduino/libraries"
#endif

#ifndef RADIOLIB_GODMODE
#if !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /RadioLib/src/BuildOpt.h and uncomment #define RADIOLIB_GODMODE around line 367" 
#endif
#endif

ConfigStore& configStore = ConfigStore::getInstance();
ConnectionManager& connMgr = ConnectionManager::getInstance();
MQTT_Client& mqtt = MQTT_Client::getInstance();
Radio& radio = Radio::getInstance();

const char* ntpServer = "time.cloudflare.com";

// Global status
Status status;

void printControls();
void checkButton();
void setupNTP();
void handleSerial();
void handleRawSerial();

// WebServer config-saved callback
void onConfigSaved() {
  Log::console(PSTR("Configuration saved, restarting..."));
  delay(500);
  ESP.restart();
}

void setup()
{
#if CONFIG_IDF_TARGET_ESP32C3
  setCpuFrequencyMhz(160);
#else
  setCpuFrequencyMhz(240);
#endif
  Serial.begin(115200);
  delay(100);

  // Initialize async logging early
  Log::initAsync();

  improvWiFi.setVersion(status.version);
  Log::console(PSTR("TinyGS Version %d - %s"), status.version, status.git_version);
  Log::console(PSTR("Chip  %s - %d"), ESP.getChipModel(), ESP.getChipRevision());

  // Initialize ConfigStore (NVS + EEPROM migration)
  configStore.init();

  // Generate OTP if MQTT credentials are missing
  if ((configStore.getMqttServer()[0] == '\0') ||
      (configStore.getMqttUser()[0] == '\0') ||
      (configStore.getMqttPass()[0] == '\0')) {
    mqttCredentials.generateOTPCode();
  }

  // Initialize display
  if (!configStore.getDisableOled()) {
    displayInit();
    displayShowInitialCredits();
  }

  // Initialize ConnectionManager (EthWiFiManager + AP fallback)
  connMgr.init();

  // Register network observers
  connMgr.registerObserver(&mqtt);

  // Start WebServer (always, so it's available in AP mode too)
  TinyGSWebServer& webServer = TinyGSWebServer::getInstance();
  webServer.setConfigSavedCallback(onConfigSaved);
  webServer.begin();
  connMgr.registerObserver(&webServer);

  // Wait for connection or AP mode
  connMgr.managedDelay(1000);

  // Initialize radio
  board_t board;
  if (configStore.getBoardConfig(board)) {
    pinMode(board.PROG__BUTTON, INPUT_PULLUP);
  }

  if (!configStore.isFailSafeActive()) {
    if (!configStore.getDisableRadio()) {
      radio.init();
    } else {
      Log::console(PSTR("Radio disabled (dev mode). Skipping radio init."));
    }
  } else {
    Log::console(PSTR("FATAL ERROR: Rescue mode. Connect to WiFi AP: %s, open 192.168.4.1"), configStore.getThingName());
  }

  if (configStore.getOledBright() == 0) {
    displayTurnOff();
  }

  printControls();
}

bool mqttAutoconf() {
  const time_t AUTOCONFIG_TIMEOUT = 30 * 60 * 1000;
  static time_t started_autoconf = millis();

  if (!connMgr.isConnected()) return false;

  if (millis() - started_autoconf > AUTOCONFIG_TIMEOUT) {
    Log::console(PSTR("Autoconfig timeout, please check your internet connection and restart the board"));
    delay(500);
    esp_deep_sleep_start();
    return false;
  }

  String result = mqttCredentials.fetchCredentials();
  if (result == "") return false;

  Log::debug("result: %s", result.c_str());
  delay(1000);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, result);
  if (error) {
    Log::console("deserializeJson() failed: %s", error.c_str());
    return false;
  }

  if (doc.containsKey("mqtt") &&
      doc["mqtt"].containsKey("user") &&
      doc["mqtt"].containsKey("pass") &&
      doc["mqtt"].containsKey("server") &&
      doc["mqtt"].containsKey("port")) {
    const char* mqttUser = doc["mqtt"]["user"];
    const char* mqttPass = doc["mqtt"]["pass"];
    const char* mqttServer = doc["mqtt"]["server"];
    const char* mqttPort = doc["mqtt"]["port"];

    Log::debug("MQTT User: %s", mqttUser);
    Log::debug("MQTT Pass: %s", mqttPass);
    Log::debug("MQTT Server: %s", mqttServer);
    Log::debug("MQTT Port: %s", mqttPort);

    configStore.setMqttServer(mqttServer);
    configStore.setMqttUser(mqttUser);
    configStore.setMqttPass(mqttPass);
    configStore.setMqttPort(mqttPort);
  } else {
    Log::console(PSTR("Invalid JSON format"));
    return false;
  }

  if (doc.containsKey("config")) {
    if (doc["config"].containsKey("stationName")) {
      const char* stationName = doc["config"]["stationName"];
      Log::console("Station Name: %s", stationName);
      configStore.setThingName(stationName);
    }
    if (doc["config"].containsKey("adminPw")) {
      const char* adminPw = doc["config"]["adminPw"];
      Log::console("Admin Password: %s", adminPw);
      configStore.setApPassword(adminPw);
    }
    if (doc["config"].containsKey("latitude") && doc["config"].containsKey("longitude")) {
      const char* latitude = doc["config"]["latitude"].as<const char*>();
      const char* longitude = doc["config"]["longitude"].as<const char*>();
      Log::console("Latitude: %s", latitude);
      Log::console("Longitude: %s", longitude);
      if (latitude) configStore.setLat(latitude);
      if (longitude) configStore.setLon(longitude);
    }
    if (doc["config"].containsKey("tz")) {
      const char* tz = doc["config"]["tz"];
      Log::console("Timezone: %s", tz);
      configStore.setTZ(tz);
    }
  }

  configStore.save();
  return true;
}
unsigned long lastTleRefresh = millis();

void loop() {
  connMgr.loop();

  if (configStore.isFailSafeActive()) {
    static bool updateAttempted = false;
    if (!updateAttempted && connMgr.isConnected()) {
      updateAttempted = true;
      OTA::update();
    }
    if (millis() > 10000 || updateAttempted)
      connMgr.forceApMode(true);
    return;
  }

  ArduinoOTA.handle();
  handleSerial();

  // AP mode - show AP screen
  if (connMgr.isApMode()) {
    displayShowApMode();
    return;
  }

  // MQTT credentials missing - auto-configure
  if ((configStore.getMqttServer()[0] == '\0') ||
      (configStore.getMqttUser()[0] == '\0') ||
      (configStore.getMqttPass()[0] == '\0')) {
    mqttAutoconf();
    return;
  }

  // Check button
  checkButton();

  // Radio
  if (radio.isReady()) {
    status.radio_ready = true;
    radio.listen();
  } else {
    status.radio_ready = false;
  }

  // Not connected yet
  if (!connMgr.isConnected()) {
    displayShowStaMode(connMgr.isApMode());
    return;
  }

  // Setup NTP on first connection
  static bool ntpConfigured = false;
  if (!ntpConfigured && connMgr.isConnected()) {
    ntpConfigured = true;
    setupNTP();
    displayShowConnected();
    arduino_ota_setup();
    if (configStore.getLowPower()) {
      Log::debug(PSTR("Set low power CPU=80Mhz"));
      setCpuFrequencyMhz(80);
    }
  }

  // Connected - run MQTT and OTA
  mqtt.loop();
  OTA::loop();
  displayUpdate();

  // Update TLE
  unsigned long currentTime = millis();
  if (radio.isReady() && currentTime - lastTleRefresh >= status.tle.refresh) {
    lastTleRefresh = currentTime;
    radio.tle();
  }
}


void setupNTP()
{
  configTime(0, 0, ntpServer);
  setenv("TZ", configStore.getTZ(), 1);
  tzset();
  delay(1000);
}

void checkButton()
{
  #define RESET_BUTTON_TIME 8000
  static unsigned long buttPressedStart = 0;
  board_t board;
  
  if (configStore.getBoardConfig(board) && !digitalRead(board.PROG__BUTTON))
  {
    if (!buttPressedStart)
    {
      buttPressedStart = millis();
    }
    else if (millis() - buttPressedStart > RESET_BUTTON_TIME) // long press
    {
      Log::console(PSTR("Rescue mode forced by button long press!"));
      Log::console(PSTR("Connect to the WiFi AP: %s and open a web browser on ip 192.168.4.1 to configure your station and manually reboot when you finish."), configStore.getThingName());
      configStore.resetAPConfig();
      connMgr.forceApMode(true);
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

void handleSerial() {
  while (Serial.available() > 0) {
    yield();
    byte next = Serial.peek();
    if (next == 'I') {
      improvWiFi.handleImprovPacket();
    } else {
      handleRawSerial();
    }
  }
}

void handleRawSerial()
{
  if (Serial.available())
  {
    radio.disableInterrupt();

    char serialCmd1 = Serial.read();
    char serialCmd = ' ';

    delay(500);
    if (serialCmd1 == '!') serialCmd = Serial.read();
    while (Serial.available()) Serial.read();

    switch (serialCmd) {
      case ' ':
        break;
      case 'e':
        configStore.resetAllConfig();
        ESP.restart();
        break;
      case 'b':
        ESP.restart();
        break;
      case 'p':
        if (!configStore.getAllowTx()) {
          Log::console(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
          break;
        }
        {
          static long lastTestPacketTime = 0;
          if (millis() - lastTestPacketTime < 20 * 1000) {
            Log::console(PSTR("Please wait a few seconds to send another test packet."));
            break;
          }
          radio.sendTestPacket();
          lastTestPacketTime = millis();
          Log::console(PSTR("Sending test packet to nearby stations!"));
        }
        break;
      case 'w':
        Log::console(PSTR("Getting weblogin"));
        mqtt.sendWeblogin();
        break;
      case 'o':
        Log::console(PSTR("OTP Code: %s"), mqttCredentials.getOTPCode());
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
  Log::console (PSTR ("------------------------------------"));
}