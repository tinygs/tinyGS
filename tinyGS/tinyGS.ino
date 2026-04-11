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
#include "src/Power/Power.h"
#include <Wire.h>


#if  RADIOLIB_VERSION_MAJOR != (0x07) || RADIOLIB_VERSION_MINOR != (0x06) || RADIOLIB_VERSION_PATCH != (0x00) || RADIOLIB_VERSION_EXTRA != (0x00)
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

// -------------------------------------------------------------------------
// Bootloop detection
// RTC_DATA_ATTR survives software resets (crash/panic/abort/watchdog) but
// is cleared on power-off, so it tracks crash-loop cycles without
// permanently locking out a fresh power cycle.
// -------------------------------------------------------------------------
static constexpr uint8_t  BOOTLOOP_THRESHOLD = 5;   // consecutive crashes before rescue
static constexpr uint32_t STABLE_MS          = 60000UL; // ms of uptime = stable
RTC_DATA_ATTR static uint8_t s_bootCount = 0;

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
  delay(2000); // allow the HTTP response to be fully flushed before restart
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
  // Give the serial monitor time to connect before any important log output.
  // 3 s is enough for a human to open the monitor; the 10-s migration wait
  // is additive on top of this when a first-time migration is needed.
  delay(2000);

  // Initialize async logging early
  Log::initAsync();

  // -----------------------------------------------------------------------
  // Bootloop detection: increment RTC counter on every boot.
  // If we crash repeatedly before reaching STABLE_MS of uptime the counter
  // accumulates across reboots.  On hitting the threshold we force rescue
  // (failsafe) mode — radio init is skipped and only the WiFi AP + WebServer
  // run so the user can correct the configuration.
  // -----------------------------------------------------------------------
  ++s_bootCount;
  Log::console(PSTR("[Bootloop] Boot count: %d/%d"), (int)s_bootCount, (int)BOOTLOOP_THRESHOLD);
  if (s_bootCount >= BOOTLOOP_THRESHOLD) {
    Log::console(PSTR("[Bootloop] Bootloop detected! Forcing rescue mode."));
    configStore.setFailSafe(true);
    s_bootCount = 0; // reset so the boot after a config fix starts clean
  }

  improvWiFi.setVersion(status.version);
  Log::console(PSTR("TinyGS Version %d - %s"), status.version, status.git_version);
  Log::console(PSTR("Chip  %s - %d"), ESP.getChipModel(), ESP.getChipRevision());

  // -----------------------------------------------------------------------
  // STEP 1: Load configuration (and migrate from legacy EEPROM if needed).
  // THIS MUST RUN BEFORE ANY network, display, OTP or radio initialisation
  // so that every subsystem receives the correct, already-migrated values.
  // -----------------------------------------------------------------------
  configStore.init();

  // -----------------------------------------------------------------------
  // STEP 2: OTP – depends on config credentials, no network started yet.
  // -----------------------------------------------------------------------
  if ((configStore.getMqttServer()[0] == '\0') ||
      (configStore.getMqttUser()[0] == '\0') ||
      (configStore.getMqttPass()[0] == '\0')) {
    mqttCredentials.generateOTPCode();
  }

  // -----------------------------------------------------------------------
  // STEP 3: Display – local hardware only, no network.
  // -----------------------------------------------------------------------
  if (!configStore.getDisableOled()) {
    displayInit();
    displayShowInitialCredits();
  }

  // -----------------------------------------------------------------------
  // STEP 4: Network (WiFi / Ethernet + AP fallback). Config already loaded.
  // -----------------------------------------------------------------------
  connMgr.init();

  // -----------------------------------------------------------------------
  // STEP 4 (cont.): Network observers + WebServer (still STEP 4 – all
  // network-layer setup before radio or hardware init).
  // -----------------------------------------------------------------------
  // Register network observers
  connMgr.registerObserver(&mqtt);

  // Start WebServer (always, so it's available in AP mode too)
  TinyGSWebServer& webServer = TinyGSWebServer::getInstance();
  webServer.setConfigSavedCallback(onConfigSaved);
  webServer.begin();
  connMgr.registerObserver(&webServer);

  // Wait for connection or AP mode
  connMgr.managedDelay(1000);

  // -----------------------------------------------------------------------
  // STEP 5: Hardware peripherals (AXP power IC, radio). WiFi is up by now.
  // -----------------------------------------------------------------------
  board_t board;
  if (configStore.getBoardConfig(board)) {
    pinMode(board.PROG__BUTTON, INPUT_PULLUP);
  }

  // AXP power check runs on the main task — it uses I2C (Wire) which is
  // not safe to call from the radioInit task.  checkAXP() also calls
  // Wire.end(), so we must re-establish Wire for the OLED display after.
  {
    Power& power = Power::getInstance();
    power.checkAXP();
    // Only re-establish Wire if the board physically has an OLED on an I2C bus.
    // For ethernet-only boards (OLED__address == 0) both SDA and SCL are
    // UNUSED_PIN (255); calling Wire.begin(255, 255) on a fresh Wire instance
    // crashes the ESP32 via gpio_matrix_out() with an invalid pin number.
    if (!configStore.getDisableOled() && board.OLED__address != 0) {
      Wire.begin(board.OLED__SDA, board.OLED__SCL);
      Wire.setClock(700000);
      Wire.setTimeOut(50);
    }
  }

  if (!configStore.isFailSafeActive()) {
    if (!configStore.getDisableRadio()) {
      // Run radio.init() in a separate task to guard against an indefinite
      // SPI hang when no radio chip is physically present on the board.
      static TaskHandle_t   s_radioInitTask = nullptr;
      static volatile bool  s_radioInitDone = false;
      xTaskCreate(
        [](void*) {
          radio.init();
          s_radioInitDone = true;   // signal before self-delete; prevents double-delete
          vTaskDelete(nullptr);
        },
        "radioInit", 4096, nullptr, 1, &s_radioInitTask
      );
      const unsigned long RADIO_INIT_TIMEOUT_MS = 15000UL;
      unsigned long t0 = millis();
      while (!radio.isReady() && !s_radioInitDone && (millis() - t0 < RADIO_INIT_TIMEOUT_MS)) {
        delay(50);
      }
      if (!radio.isReady()) {
        if (!s_radioInitDone) {
          // Task is still running but SPI appears to be hung — force-kill it.
          Log::console(PSTR("[SX12xx] Init timeout after %ds - no radio chip detected"), RADIO_INIT_TIMEOUT_MS / 1000);
          vTaskDelete(s_radioInitTask);
        } else {
          Log::console(PSTR("[SX12xx] No radio configured for this board. Please configure via the console."));
        }
        s_radioInitTask = nullptr;
      }
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
bool ntpConfigured = false; // reset on disconnect so NTP is retried on every reconnect

void loop() {
  connMgr.loop();

  // Always process serial input (Improv provisioning + debug console)
  // regardless of connection state or failsafe mode.
  handleSerial();

  // Reset the bootloop counter once the device has been running stably for
  // STABLE_MS without crashing.  s_bootCount is 0 when failsafe is active
  // (we reset it on threshold), so this block only fires in normal operation.
  if (s_bootCount > 0 && millis() >= STABLE_MS) {
    s_bootCount = 0;
    Log::console(PSTR("[Bootloop] Stable operation confirmed. Boot counter reset."));
  }

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

  // AP mode - show AP screen
  if (connMgr.isApMode()) {
    displayShowApMode();
    return;
  }

  // Check button
  checkButton();

  // Track radio hardware status (independent of connection state)
  status.radio_ready = radio.isReady();

  // Not connected yet — reset NTP flag so it runs again on the next reconnect
  if (!connMgr.isConnected()) {
    ntpConfigured = false;
    displayShowStaMode(connMgr.isApMode());
    return;
  }

  // One-time setup on the very first successful connection
  static bool firstConnectDone = false;
  if (!firstConnectDone) {
    firstConnectDone = true;
    arduino_ota_setup();
    if (configStore.getLowPower()) {
      Log::debug(PSTR("Set low power CPU=80Mhz"));
      setCpuFrequencyMhz(80);
    }
  }

  // NTP: call on every (re)connect until the clock is synced.
  // configTime() is idempotent — calling it again after a reconnect triggers
  // a fresh SNTP request instead of waiting up to 15 min for the internal retry.
  if (!ntpConfigured) {
    ntpConfigured = true;
    LOG_CONSOLE(PSTR("[NTP] Configuring SNTP with server: %s"), ntpServer);
    setupNTP();
  }

  // MQTT credentials missing - show OTP and auto-configure
  if ((configStore.getMqttServer()[0] == '\0') ||
      (configStore.getMqttUser()[0] == '\0') ||
      (configStore.getMqttPass()[0] == '\0')) {
    displayShowConnected();
    mqttAutoconf();
    return;
  }

  if (radio.isReady())
  {
    status.radio_ready = true;
    radio.listen();
  }
  else {
    status.radio_ready = false;
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
  
  if (configStore.getBoardConfig(board) && board.PROG__BUTTON != UNUSED_PIN && !digitalRead(board.PROG__BUTTON))
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
  Serial.flush(); // drain TX buffer before the burst
  Log::console (PSTR ("------------- Controls -------------"));
  Serial.flush();
  Log::console (PSTR ("!e - erase board config and reset"));
  Serial.flush();
  Log::console (PSTR ("!b - reboot the board"));
  Serial.flush();
  Log::console (PSTR ("!p - send test packet to nearby stations (to check transmission)"));
  Serial.flush();
  Log::console (PSTR ("!w - ask for weblogin link"));
  Serial.flush();
  Log::console (PSTR ("!o - get OTP code"));
  Serial.flush();
  Log::console (PSTR ("------------------------------------"));
  Serial.flush(); // ensure everything is sent before loop() starts
}