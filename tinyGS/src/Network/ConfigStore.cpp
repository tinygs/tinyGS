/*
  ConfigStore.cpp - NVS-based configuration storage

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
*/

#include "ConfigStore.h"
#include "../Logger/Logger.h"
#include <EEPROM.h>
#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C3)
#include <esp_eth_mac.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_eth_mac_esp.h>
#endif
#include <driver/gpio.h>
#include <esp_log.h>
#endif

// ============================================================
//  Board table - exact same data from old ConfigManager
// ============================================================

ConfigStore::ConfigStore() {
  initBoardTable();
}

void ConfigStore::initBoardTable() {
  uint8_t i = 0;
#if CONFIG_IDF_TARGET_ESP32S3
  _boards[i++] = {0x3c, 17, 18, 21, 0, 35, RADIO_SX1262, 8, UNUSED_PIN, 14, 13, 12, 11, 10, 9, 1.6f, UNUSED_PIN, UNUSED_PIN, "150-960Mhz - HELTEC LORA32 V3 SX1262"};
  // Heltec Wireless Stick Lite V3: no OLED, same radio SPI as LORA32 V3
  _boards[i++] = {0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 0, 35, RADIO_SX1262, 8, UNUSED_PIN, 14, 13, 12, 11, 10, 9, 1.6f, UNUSED_PIN, UNUSED_PIN, "150-960Mhz - HELTEC WSL V3 SX1262"};
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 35, RADIO_SX1278, 8, 6, 14, UNUSED_PIN, 12, 11, 10, 9, 0.0f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32-S3 433MHz SX1278"};
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 3, RADIO_SX1262, 10, UNUSED_PIN, 1, 4, 5, 13, 11, 12, 1.6f, UNUSED_PIN, UNUSED_PIN, "433 Mhz TTGO T-Beam Sup SX1262 V1.0"};
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 37, RADIO_SX1280, 7, UNUSED_PIN, 9, UNUSED_PIN, 8, 3, 6, 5, 0.0f, 21, 10, "2.4Ghz LILYGO SX1280"};
  // Waveshare ESP32-S3-ETH: no OLED, no radio — W5500 SPI ethernet
  _boards[i] = {0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 0, UNUSED_PIN, 0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 0.0f, UNUSED_PIN, UNUSED_PIN, "Waveshare ESP32-S3-ETH (W5500)"};
  _boards[i].ethEN = true;
  _boards[i].ethPHY = 0; // W5500
  _boards[i].ethSPI = 2; // HSPI
  _boards[i].ethCS = 14;
  _boards[i].ethINT = 10;
  _boards[i].ethRST = 9;
  _boards[i].ethMISO = 12;
  _boards[i].ethMOSI = 11;
  _boards[i].ethSCK = 13;
  i++;
  _boards[i++] = {0x3c, 18, 17, 21, 0, 35, RADIO_LR1121, 8, UNUSED_PIN, 14, 13, 12, 11, 10, 9, 1.8f, UNUSED_PIN, UNUSED_PIN, "EBYTE EoRa-HUB ESP32S3 + LR1121"};
#elif CONFIG_IDF_TARGET_ESP32C3
  _boards[i++] = {0x3c, 0, 1, UNUSED_PIN, 20, 21, RADIO_SX1262, 8, UNUSED_PIN, 3, 4, 5, 6, 7, 10, 1.6f, UNUSED_PIN, UNUSED_PIN, "433MHz HELTEC LORA32 HT-CT62 SX1262"};
  _boards[i++] = {0x3c, 0, 1, UNUSED_PIN, 20, 21, RADIO_SX1278, 8, 4, UNUSED_PIN, UNUSED_PIN, 5, 6, 7, 10, 0.0f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32-C3 433MHz SX1278"};
#else
  _boards[i++] = {0x3c, 4, 15, 16, 0, 25, RADIO_SX1278, 18, 26, 12, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz HELTEC WiFi LoRA 32 V1"};
  _boards[i++] = {0x3c, 4, 15, 16, 0, 25, RADIO_SX1276, 18, 26, 12, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "863-928MHz HELTEC WiFi LoRA 32 V1"};
  _boards[i++] = {0x3c, 4, 15, 16, 0, 25, RADIO_SX1278, 18, 26, 35, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz HELTEC WiFi LoRA 32 V2"};
  _boards[i++] = {0x3c, 4, 15, 16, 0, 25, RADIO_SX1276, 18, 26, 35, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "863-928MHz HELTEC WiFi LoRA 32 V2"};
  _boards[i++] = {0x3c, 4, 15, 16, 0, 2, RADIO_SX1278, 18, 26, UNUSED_PIN, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433Mhz TTGO LoRa 32 v1"};
  _boards[i++] = {0x3c, 4, 15, 16, 0, 2, RADIO_SX1276, 18, 26, UNUSED_PIN, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz TTGO LoRa 32 v1"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 22, RADIO_SX1278, 18, 26, 33, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz TTGO LoRA 32 v2"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 22, RADIO_SX1276, 18, 26, 33, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz TTGO LoRA 32 v2"};
  _boards[i++] = {0x3c, 21, 22, 16, 39, 22, RADIO_SX1278, 18, 26, 33, 32, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz T-BEAM + OLED"};
  _boards[i++] = {0x3c, 21, 22, 16, 39, 22, RADIO_SX1276, 18, 26, 33, 32, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz T-BEAM + OLED"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 25, RADIO_SX1268, 5, UNUSED_PIN, 27, 26, 14, 19, 23, 18, 0.0f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32 Wroom + SX126x (Crystal)"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 25, RADIO_SX1268, 18, UNUSED_PIN, 33, 32, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "TTGO LoRa 32 V2 Modified with module SX126x (crystal)"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 25, RADIO_SX1268, 5, UNUSED_PIN, 2, 13, 26, 19, 23, 18, 1.6f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32 Wroom + SX126x DRF1268T (TCX0) (5, 2, 26, 13)"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 25, RADIO_SX1268, 5, UNUSED_PIN, 26, 12, 14, 19, 23, 18, 1.6f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32 Wroom + SX126x DRF1268T (TCX0) (5, 26, 14, 12)"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 38, 22, RADIO_SX1278, 18, 26, 33, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz T-BEAM V1.0 + OLED"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 2, RADIO_SX1268, 5, UNUSED_PIN, 34, 32, 14, 19, 27, 18, 1.6f, UNUSED_PIN, UNUSED_PIN, "433MHz FOSSA 1W Ground Station"};
  _boards[i++] = {0x3c, 21, 22, 16, 0, 2, RADIO_SX1276, 5, UNUSED_PIN, 34, 32, 14, 19, 27, 18, 1.6f, UNUSED_PIN, UNUSED_PIN, "868-915MHz FOSSA 1W Ground Station"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 22, RADIO_SX1280, 5, 26, 34, 32, 14, 19, 27, 18, 0.0f, UNUSED_PIN, UNUSED_PIN, "2.4GHz ESP32 + SX1280"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 38, 22, RADIO_SX1276, 18, 26, 33, UNUSED_PIN, 14, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz T-BEAM V1.0 + OLED"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 25, RADIO_SX1278, 18, 26, 33, UNUSED_PIN, 23, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "433MHz LILYGO T3_V1.6.1"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 25, RADIO_SX1276, 18, 26, 33, UNUSED_PIN, 23, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz LILYGO T3_V1.6.1"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 0, 25, RADIO_SX1276, 18, 26, UNUSED_PIN, 32, 23, 19, 27, 5, 0.0f, UNUSED_PIN, UNUSED_PIN, "868-915MHz LILYGO T3_V1.6.1 TCXO"};
  _boards[i++] = {0x3c, 21, 22, UNUSED_PIN, 38, 4, RADIO_SX1268, 18, 26, 33, 32, 23, 19, 27, 5, 1.6f, UNUSED_PIN, UNUSED_PIN, "433 Mhz T-Beam SX1268 V1.0"};
  // WT32-ETH01/02: no OLED, no radio — LAN8720 Internal EMAC
  _boards[i] = {0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 0.0f, UNUSED_PIN, UNUSED_PIN, "WT32-ETH01/02 (LAN8720)"};
  _boards[i].ethEN = true;
  _boards[i].ethPHY = 0xFF; // Internal EMAC
  _boards[i].ethMDC = 23;
  _boards[i].ethMDIO = 18;
  _boards[i].ethPhyAddr = 1;
  _boards[i].ethPhyType = 0;   // LAN8720
  _boards[i].ethRefClk = 0;    // GPIO0
  _boards[i].ethClkExt = true; // PHY drives REF_CLK
  _boards[i].ethOscEN = 16;    // Oscillator enable
  i++;
#endif
}

// ============================================================
//  NVS Load/Save
// ============================================================

void ConfigStore::loadFromNVS() {
  _prefs.begin("tinygs", true); // read-only

  _prefs.getString("stName", _stationName, sizeof(_stationName));
  _prefs.getString("apPass", _apPassword, sizeof(_apPassword));
  _prefs.getString("wifiSSID", _wifiSSID, sizeof(_wifiSSID));
  _prefs.getString("wifiPass", _wifiPass, sizeof(_wifiPass));
  _prefs.getString("lat", _latitude, sizeof(_latitude));
  _prefs.getString("lng", _longitude, sizeof(_longitude));
  _prefs.getString("tz", _tz, sizeof(_tz));
  _prefs.getString("mqttSrv", _mqttServer, sizeof(_mqttServer));
  _prefs.getString("mqttPort", _mqttPort, sizeof(_mqttPort));
  _prefs.getString("mqttUser", _mqttUser, sizeof(_mqttUser));
  _prefs.getString("mqttPass", _mqttPass, sizeof(_mqttPass));
  _prefs.getString("board", _board, sizeof(_board));
  _prefs.getString("oledBr", _oledBright, sizeof(_oledBright));
  _prefs.getString("allowTx", _allowTx, sizeof(_allowTx));
  _prefs.getString("remoteTn", _remoteTune, sizeof(_remoteTune));
  _prefs.getString("tele3rd", _telemetry3rd, sizeof(_telemetry3rd));
  _prefs.getString("testMode", _testMode, sizeof(_testMode));
  _prefs.getString("autoUpd", _autoUpdate, sizeof(_autoUpdate));
  _prefs.getString("disOled", _disableOled, sizeof(_disableOled));
  _prefs.getString("disRadio", _disableRadio, sizeof(_disableRadio));
  _prefs.getString("alwaysAP", _alwaysAP, sizeof(_alwaysAP));
  _prefs.getString("brdTpl", _boardTemplate, sizeof(_boardTemplate));
  _prefs.getString("modemSt", _modemStartup, sizeof(_modemStartup));
  _prefs.getString("advCfg", _advancedConfig, sizeof(_advancedConfig));
  _ifaceMode = (InterfaceMode)_prefs.getUChar(
      "ifaceMode", (uint8_t)InterfaceMode::WIFI_ONLY);

  _prefs.end();
}

void ConfigStore::saveToNVS() {
  _prefs.begin("tinygs", false); // read-write

  _prefs.putString("stName", _stationName);
  _prefs.putString("apPass", _apPassword);
  _prefs.putString("wifiSSID", _wifiSSID);
  _prefs.putString("wifiPass", _wifiPass);
  _prefs.putString("lat", _latitude);
  _prefs.putString("lng", _longitude);
  _prefs.putString("tz", _tz);
  _prefs.putString("mqttSrv", _mqttServer);
  _prefs.putString("mqttPort", _mqttPort);
  _prefs.putString("mqttUser", _mqttUser);
  _prefs.putString("mqttPass", _mqttPass);
  _prefs.putString("board", _board);
  _prefs.putString("oledBr", _oledBright);
  _prefs.putString("allowTx", _allowTx);
  _prefs.putString("remoteTn", _remoteTune);
  _prefs.putString("tele3rd", _telemetry3rd);
  _prefs.putString("testMode", _testMode);
  _prefs.putString("autoUpd", _autoUpdate);
  _prefs.putString("disOled", _disableOled);
  _prefs.putString("disRadio", _disableRadio);
  _prefs.putString("alwaysAP", _alwaysAP);
  _prefs.putString("brdTpl", _boardTemplate);
  _prefs.putString("modemSt", _modemStartup);
  _prefs.putString("advCfg", _advancedConfig);
  _prefs.putUChar("ifaceMode", (uint8_t)_ifaceMode);
  _prefs.putString("cfgVer", configVersion);

  _prefs.end();
}

// ============================================================
//  Init
// ============================================================

bool ConfigStore::init() {
  // Check if config already exists in NVS
  _prefs.begin("tinygs", true);
  String ver = _prefs.getString("cfgVer", "");
  _prefs.end();

  bool validConfig = false;

  if (ver.length() == 0) {
    // First boot or migrated from IotWebConf2 → attempt migration.
    // Wait 10 s so the serial monitor can connect before migration logs appear.
    LOG_CONSOLE(PSTR(
        "[Migration] Config version not found in NVS namespace 'tinygs'."));
    LOG_CONSOLE(
        PSTR("[Migration] Waiting 10 s for serial monitor to connect..."));
    for (int _s = 10; _s > 0; _s--) {
      LOG_CONSOLE(PSTR("[Migration] Starting in %d s..."), _s);
      delay(1000);
    }
    LOG_CONSOLE(PSTR("[Migration] Starting EEPROM migration check now."));
    if (!migrateFromEEPROM()) {
      // No legacy config found — fresh install, apply defaults.
      LOG_CONSOLE(PSTR("[Migration] No legacy EEPROM config found. Applying "
                       "factory defaults."));
      strncpy(_stationName, thingName, sizeof(_stationName));
      strncpy(_mqttServer, MQTT_DEFAULT_SERVER, sizeof(_mqttServer));
      strncpy(_mqttPort, MQTT_DEFAULT_PORT, sizeof(_mqttPort));
      strncpy(_oledBright, "100", sizeof(_oledBright));
      strncpy(_remoteTune, "selected", sizeof(_remoteTune));
      strncpy(_telemetry3rd, "selected", sizeof(_telemetry3rd));
      strncpy(_autoUpdate, "selected", sizeof(_autoUpdate));
      LOG_CONSOLE(PSTR("[Migration] Saving factory defaults to NVS..."));
      saveToNVS();
      LOG_CONSOLE(
          PSTR("[Migration] Factory defaults saved. Fresh install complete."));
    } else {
      validConfig = true;
    }
  } else {
    LOG_CONSOLE(PSTR("[Migration] NVS config version found: \"%s\". Loading "
                     "config from NVS."),
                ver.c_str());
    loadFromNVS();
    LOG_CONSOLE(PSTR("[Migration] NVS config loaded. WiFi SSID: \"%s\"."),
                _wifiSSID);

    // Check whether the legacy EEPROM blob still exists in NVS to confirm
    // whether a previous migration already ran (or if old data is still
    // available).
    {
      Preferences eepromCheck;
      if (eepromCheck.begin("eeprom", true)) {
        size_t oldBlobLen = eepromCheck.getBytesLength("eeprom");
        eepromCheck.end();
        if (oldBlobLen > 0) {
          LOG_CONSOLE(
              PSTR(
                  "[Migration] Legacy EEPROM NVS blob still present (%d bytes) "
                  "— previous migration ran and old data was preserved."),
              (int)oldBlobLen);
        } else {
          LOG_CONSOLE(PSTR("[Migration] Legacy EEPROM NVS namespace exists but "
                           "blob is empty."));
        }
      } else {
        LOG_CONSOLE(PSTR(
            "[Migration] Legacy EEPROM NVS namespace \"eeprom\" not found "
            "— either migration never ran or old data was erased externally."));
      }
    }
    // Detect an incomplete previous migration: re-run if any critical field
    // that should have been present in a configured device is still empty. This
    // covers both the "wrong namespace" bug and partial extractions.
    bool needsReMigration = false;
    if (_wifiSSID[0] == '\0') {
      LOG_CONSOLE(
          PSTR("[Migration] WARNING: WiFi SSID is empty after NVS load."));
      needsReMigration = true;
    }
    if (_mqttUser[0] == '\0') {
      LOG_CONSOLE(
          PSTR("[Migration] WARNING: MQTT user is empty after NVS load."));
      needsReMigration = true;
    }
    if (_mqttPass[0] == '\0') {
      LOG_CONSOLE(
          PSTR("[Migration] WARNING: MQTT password is empty after NVS load."));
      needsReMigration = true;
    }
    if (needsReMigration) {
      LOG_CONSOLE(PSTR("[Migration] One or more critical fields are empty — "
                       "attempting corrective re-migration from EEPROM..."));
      if (migrateFromEEPROM()) {
        LOG_CONSOLE(PSTR("[Migration] Corrective re-migration succeeded."));
        LOG_CONSOLE(PSTR("[Migration]   WiFi SSID  : \"%s\""), _wifiSSID);
        LOG_CONSOLE(PSTR("[Migration]   MQTT user  : %s"),
                    (_mqttUser[0] ? "(set)" : "(empty)"));
        LOG_CONSOLE(PSTR("[Migration]   MQTT pass  : %s"),
                    (_mqttPass[0] ? "(set)" : "(empty)"));
      } else {
        LOG_CONSOLE(PSTR("[Migration] Corrective re-migration found no EEPROM "
                         "data — fields remain empty."));
      }
    } else {
      LOG_CONSOLE(PSTR(
          "[Migration] All critical fields present — no re-migration needed."));
    }
    validConfig = true;
  }

  // No board selected → auto detect
  if (!strcmp(_board, "")) {
    boardDetection();
  }

  if (strlen(_advancedConfig))
    parseAdvancedConf();

  parseModemStartup();

  // Summary of effective configuration after init
  LOG_CONSOLE(PSTR("[Migration] Config init done. Summary:"));
  LOG_CONSOLE(PSTR("[Migration]   validConfig  : %s"),
              validConfig ? "true" : "false");
  LOG_CONSOLE(PSTR("[Migration]   stationName  : \"%s\""), _stationName);
  LOG_CONSOLE(PSTR("[Migration]   wifiSSID     : \"%s\""), _wifiSSID);
  LOG_CONSOLE(PSTR("[Migration]   mqttServer   : \"%s\""), _mqttServer);
  LOG_CONSOLE(PSTR("[Migration]   mqttPort     : \"%s\""), _mqttPort);
  LOG_CONSOLE(PSTR("[Migration]   mqttUser     : %s"),
              (_mqttUser[0] ? "(set)" : "(empty)"));
  LOG_CONSOLE(PSTR("[Migration]   mqttPass     : %s"),
              (_mqttPass[0] ? "(set)" : "(empty)"));
  LOG_CONSOLE(PSTR("[Migration]   board        : \"%s\""), _board);
  LOG_CONSOLE(PSTR("[Migration]   latitude     : \"%s\""), _latitude);
  LOG_CONSOLE(PSTR("[Migration]   longitude    : \"%s\""), _longitude);
  // Always log the resolved board name so it's visible on every boot,
  // not just when auto-detection runs (which only happens on first boot).
  {
    board_t resolvedBoard;
    if (getBoardConfig(resolvedBoard) && resolvedBoard.BOARD.length() > 0)
      LOG_CONSOLE(PSTR("Board: %s"), resolvedBoard.BOARD.c_str());
    else
      LOG_CONSOLE(PSTR("Board: (not configured)"));
  }

  return validConfig;
}

// ============================================================
//  Migration from old EEPROM (IotWebConf2) format
// ============================================================

// Helper: copy a field from the EEPROM blob into a destination buffer,
// ensuring null-termination.  Copies min(fieldLen, destSize-1) bytes.
static void readEepromField(const uint8_t *blob, int offset, int fieldLen, char *dest, size_t destSize) {
  size_t n = ((size_t)fieldLen < destSize) ? (size_t)fieldLen : destSize - 1;
  memcpy(dest, blob + offset, n);
  dest[n] = '\0';
}

bool ConfigStore::migrateFromEEPROM() {
  // IotWebConf2 on ESP32 uses Arduino EEPROM.h which is emulated via NVS:
  //   namespace = "eeprom", key = "eeprom", stored as a single binary blob.
  // The blob layout is sequential fields written by
  // ParameterGroup::storeValue(). We read the raw blob and extract each field
  // at its known byte offset.

  LOG_CONSOLE(
      PSTR("[Migration] Opening legacy EEPROM NVS namespace (\"eeprom\")..."));

  Preferences oldPrefs;
  if (!oldPrefs.begin("eeprom", true)) { // read-only
    LOG_CONSOLE(PSTR("[Migration] NVS namespace \"eeprom\" not found. No "
                     "legacy config to migrate."));
    return false;
  }

  size_t blobLen = oldPrefs.getBytesLength("eeprom");
  LOG_CONSOLE(PSTR("[Migration] EEPROM blob length: %d bytes"), (int)blobLen);

  if (blobLen == 0) {
    LOG_CONSOLE(PSTR("[Migration] EEPROM blob is empty. Nothing to migrate."));
    oldPrefs.end();
    return false;
  }

  uint8_t *blob = (uint8_t *)malloc(blobLen);
  if (!blob) {
    LOG_CONSOLE(PSTR("[Migration] ERROR: malloc(%d) failed."), (int)blobLen);
    oldPrefs.end();
    return false;
  }
  oldPrefs.getBytes("eeprom", blob, blobLen);
  oldPrefs.end();
  LOG_CONSOLE(PSTR("[Migration] EEPROM blob read successfully."));

  // Beta's configVersion is "0.05" (4 chars at offset 0)
  const char oldConfigVersion[] = "0.05";
  char foundVer[5] = {0};
  memcpy(foundVer, blob, (blobLen >= 4) ? 4 : blobLen);
  LOG_CONSOLE(PSTR("[Migration] Config version tag in blob: \"%.4s\" (expected "
                   "\"0.05\")"),
              foundVer);

  if (blobLen < 4 || memcmp(blob, oldConfigVersion, 4) != 0) {
    LOG_CONSOLE(
        PSTR("[Migration] Version mismatch — skipping EEPROM migration."));
    free(blob);
    return false;
  }

  LOG_CONSOLE(PSTR("[Migration] Version tag OK. Starting field extraction from "
                   "IotWebConf2 EEPROM blob..."));

  // ---- EEPROM blob layout (IotWebConf2 serialization order) ----
  // Offset  Field                   Size (bytes)
  //   0     configVersion             4
  //   4     thingName                33   (IOTWEBCONF_WORD_LEN)
  //  37     apPassword               33   (IOTWEBCONF_PASSWORD_LEN)
  //  70     wifiSsid                 33   (IOTWEBCONF_WORD_LEN)
  // 103     wifiPassword             65   (IOTWEBCONF_WIFI_PASSWORD_LEN)
  // 168     apTimeout                33   (IOTWEBCONF_WORD_LEN)  -- not used in
  // new config 201     latitude                 10   (COORDINATE_LENGTH) 211
  // longitude                10   (COORDINATE_LENGTH) 221     tz 49 (TZ_LENGTH)
  // 270     mqttServer               31   (MQTT_SERVER_LENGTH)
  // 301     mqttPort                  6   (MQTT_PORT_LENGTH)
  // 307     mqttUser                 31   (MQTT_USER_LENGTH)
  // 338     mqttPass                 31   (MQTT_PASS_LENGTH)
  // 369     board                     3   (BOARD_LENGTH)
  // 372     oledBright               32   (NUMBER_LEN)
  // 404     allowTx                   9   (CHECKBOX_LENGTH)
  // 413     remoteTune                9   (CHECKBOX_LENGTH)
  // 422     telemetry3rd              9   (CHECKBOX_LENGTH)
  // 431     testMode                  9   (CHECKBOX_LENGTH)
  // 440     autoUpdate                9   (CHECKBOX_LENGTH)
  // 449     boardTemplate           256   (old TEMPLATE_LEN)
  // 705     modemStartup            256   (MODEM_LEN)
  // 961     advancedConfig          256   (ADVANCED_LEN)
  // 1217    failsafe                  1   (not migrated)
  // Total = 1218 bytes

  const int MIN_BLOB = 449; // minimum: up to autoUpdate end
  if ((int)blobLen < MIN_BLOB) {
    LOG_CONSOLE(PSTR("[Migration] Blob too small (%d bytes, need %d). Skipping "
                     "migration."),
                (int)blobLen,
                MIN_BLOB);
    free(blob);
    return false;
  }

  LOG_CONSOLE(PSTR("[Migration] Blob size OK (%d bytes). Reading fields..."),
              (int)blobLen);

  readEepromField(blob, 4, 33, _stationName, sizeof(_stationName));
  LOG_CONSOLE(PSTR("[Migration]   thingName    (off=  4, len=33): \"%s\""),
              _stationName);

  readEepromField(blob, 37, 33, _apPassword, sizeof(_apPassword));
  LOG_CONSOLE(PSTR("[Migration]   apPassword   (off= 37, len=33): %s"),
              (_apPassword[0] ? "(set)" : "(empty)"));

  readEepromField(blob, 70, 33, _wifiSSID, sizeof(_wifiSSID));
  LOG_CONSOLE(PSTR("[Migration]   wifiSSID     (off= 70, len=33): \"%s\""),
              _wifiSSID);

  readEepromField(blob, 103, 65, _wifiPass, sizeof(_wifiPass));
  LOG_CONSOLE(PSTR("[Migration]   wifiPassword (off=103, len=65): %s"),
              (_wifiPass[0] ? "(set)" : "(empty)"));

  // offset 168: apTimeout (33 bytes) — skipped, not used in new config
  LOG_CONSOLE(PSTR("[Migration]   apTimeout    (off=168, len=33): skipped"));

  readEepromField(blob, 201, 10, _latitude, sizeof(_latitude));
  LOG_CONSOLE(PSTR("[Migration]   latitude     (off=201, len=10): \"%s\""),
              _latitude);

  readEepromField(blob, 211, 10, _longitude, sizeof(_longitude));
  LOG_CONSOLE(PSTR("[Migration]   longitude    (off=211, len=10): \"%s\""),
              _longitude);

  readEepromField(blob, 221, 49, _tz, sizeof(_tz));
  LOG_CONSOLE(PSTR("[Migration]   tz           (off=221, len=49): \"%s\""),
              _tz);

  readEepromField(blob, 270, 31, _mqttServer, sizeof(_mqttServer));
  LOG_CONSOLE(PSTR("[Migration]   mqttServer   (off=270, len=31): \"%s\""),
              _mqttServer);

  readEepromField(blob, 301, 6, _mqttPort, sizeof(_mqttPort));
  LOG_CONSOLE(PSTR("[Migration]   mqttPort     (off=301, len= 6): \"%s\""),
              _mqttPort);

  readEepromField(blob, 307, 31, _mqttUser, sizeof(_mqttUser));
  LOG_CONSOLE(PSTR("[Migration]   mqttUser     (off=307, len=31): %s"),
              (_mqttUser[0] ? "(set)" : "(empty)"));

  readEepromField(blob, 338, 31, _mqttPass, sizeof(_mqttPass));
  LOG_CONSOLE(PSTR("[Migration]   mqttPass     (off=338, len=31): %s"),
              (_mqttPass[0] ? "(set)" : "(empty)"));

  readEepromField(blob, 369, 3, _board, sizeof(_board));
  LOG_CONSOLE(PSTR("[Migration]   board        (off=369, len= 3): \"%s\""),
              _board);

  readEepromField(blob, 372, 32, _oledBright, sizeof(_oledBright));
  LOG_CONSOLE(PSTR("[Migration]   oledBright   (off=372, len=32): \"%s\""),
              _oledBright);

  readEepromField(blob, 404, 9, _allowTx, sizeof(_allowTx));
  LOG_CONSOLE(PSTR("[Migration]   allowTx      (off=404, len= 9): \"%s\""),
              _allowTx);

  readEepromField(blob, 413, 9, _remoteTune, sizeof(_remoteTune));
  LOG_CONSOLE(PSTR("[Migration]   remoteTune   (off=413, len= 9): \"%s\""),
              _remoteTune);

  readEepromField(blob, 422, 9, _telemetry3rd, sizeof(_telemetry3rd));
  LOG_CONSOLE(PSTR("[Migration]   telemetry3rd (off=422, len= 9): \"%s\""),
              _telemetry3rd);

  readEepromField(blob, 431, 9, _testMode, sizeof(_testMode));
  LOG_CONSOLE(PSTR("[Migration]   testMode     (off=431, len= 9): \"%s\""),
              _testMode);

  readEepromField(blob, 440, 9, _autoUpdate, sizeof(_autoUpdate));
  LOG_CONSOLE(PSTR("[Migration]   autoUpdate   (off=440, len= 9): \"%s\""),
              _autoUpdate);

  // These larger fields only exist if the blob is big enough
  if ((int)blobLen >= 705) {
    readEepromField(blob, 449, 256, _boardTemplate, sizeof(_boardTemplate));
    LOG_CONSOLE(PSTR("[Migration]   boardTemplate(off=449, len=256): %s"),
                (_boardTemplate[0] ? "(set)" : "(empty)"));
  } else {
    LOG_CONSOLE(
        PSTR("[Migration]   boardTemplate: blob too small (%d < 705), skipped"),
        (int)blobLen);
  }

  if ((int)blobLen >= 961) {
    readEepromField(blob, 705, 256, _modemStartup, sizeof(_modemStartup));
    LOG_CONSOLE(PSTR("[Migration]   modemStartup (off=705, len=256): %s"),
                (_modemStartup[0] ? "(set)" : "(empty)"));
  } else {
    LOG_CONSOLE(
        PSTR("[Migration]   modemStartup: blob too small (%d < 961), skipped"),
        (int)blobLen);
  }

  if ((int)blobLen >= 1217) {
    readEepromField(blob, 961, 256, _advancedConfig, sizeof(_advancedConfig));
    LOG_CONSOLE(PSTR("[Migration]   advancedConfig(off=961,len=256): %s"),
                (_advancedConfig[0] ? "(set)" : "(empty)"));
  } else {
    LOG_CONSOLE(PSTR("[Migration]   advancedConfig: blob too small (%d < "
                     "1217), skipped"),
                (int)blobLen);
  }

  free(blob);

  // Apply defaults for critical empty fields
  LOG_CONSOLE(
      PSTR("[Migration] Applying defaults for empty critical fields..."));
  if (_mqttServer[0] == '\0') {
    strncpy(_mqttServer, MQTT_DEFAULT_SERVER, sizeof(_mqttServer));
    LOG_CONSOLE(PSTR("[Migration]   mqttServer was empty → default: %s"),
                _mqttServer);
  }
  if (_mqttPort[0] == '\0') {
    strncpy(_mqttPort, MQTT_DEFAULT_PORT, sizeof(_mqttPort));
    LOG_CONSOLE(PSTR("[Migration]   mqttPort was empty → default: %s"),
                _mqttPort);
  }
  if (_stationName[0] == '\0') {
    strncpy(_stationName, thingName, sizeof(_stationName));
    LOG_CONSOLE(PSTR("[Migration]   stationName was empty → default: %s"),
                _stationName);
  }

  // Save to new NVS namespace
  LOG_CONSOLE(PSTR(
      "[Migration] Saving migrated config to NVS namespace \"tinygs\"..."));
  saveToNVS();

  // Verify the write succeeded by re-reading cfgVer from NVS.
  {
    Preferences verifyPrefs;
    verifyPrefs.begin("tinygs", true);
    String savedVer = verifyPrefs.getString("cfgVer", "");
    String savedSSID = verifyPrefs.getString("wifiSSID", "");
    verifyPrefs.end();
    if (savedVer.length() > 0) {
      LOG_CONSOLE(PSTR("[Migration] NVS write verified: cfgVer=\"%s\", "
                       "wifiSSID=\"%s\"."),
                  savedVer.c_str(),
                  savedSSID.c_str());
    } else {
      LOG_CONSOLE(PSTR("[Migration] ERROR: NVS write FAILED — cfgVer not found "
                       "after saveToNVS()!"));
    }
  }

  // NOTE: The old EEPROM NVS data (namespace "eeprom") is intentionally
  // preserved so firmware can be downgraded back to the beta branch.
  LOG_CONSOLE(PSTR("[Migration] Legacy EEPROM NVS data (namespace \"eeprom\") "
                   "preserved for potential downgrade."));

  LOG_CONSOLE(PSTR("[Migration] *** Migration from EEPROM complete. Old config "
                   "was NOT erased. ***"));
  return true;
}

// ============================================================
//  Save
// ============================================================

void ConfigStore::save() {
  saveToNVS();
  _currentBoardDirty = true;
}

// ============================================================
//  Setters
// ============================================================

void ConfigStore::setThingName(const char *n) {
  strncpy(_stationName, n, sizeof(_stationName) - 1);
  save();
}
void ConfigStore::setApPassword(const char *p) {
  strncpy(_apPassword, p, sizeof(_apPassword) - 1);
  save();
}
void ConfigStore::setWifiSSID(const char *s) {
  strncpy(_wifiSSID, s, sizeof(_wifiSSID) - 1);
  save();
}
void ConfigStore::setWifiPassword(const char *p) {
  strncpy(_wifiPass, p, sizeof(_wifiPass) - 1);
  save();
}
void ConfigStore::setLat(const char *v) {
  strncpy(_latitude, v, sizeof(_latitude) - 1);
  save();
}
void ConfigStore::setLon(const char *v) {
  strncpy(_longitude, v, sizeof(_longitude) - 1);
  save();
}
void ConfigStore::setTZ(const char *v) {
  strncpy(_tz, v, sizeof(_tz) - 1);
  save();
}
void ConfigStore::setMqttServer(const char *v) {
  strncpy(_mqttServer, v, sizeof(_mqttServer) - 1);
  save();
}
void ConfigStore::setMqttPort(const char *v) {
  strncpy(_mqttPort, v, sizeof(_mqttPort) - 1);
  save();
}
void ConfigStore::setMqttUser(const char *v) {
  strncpy(_mqttUser, v, sizeof(_mqttUser) - 1);
  save();
}
void ConfigStore::setMqttPass(const char *v) {
  strncpy(_mqttPass, v, sizeof(_mqttPass) - 1);
  save();
}
void ConfigStore::setBoard(const char *v) {
  strncpy(_board, v, sizeof(_board) - 1);
  _currentBoardDirty = true;
  save();
}
void ConfigStore::setOledBright(const char *v) {
  strncpy(_oledBright, v, sizeof(_oledBright) - 1);
  save();
}

void ConfigStore::setAllowTx(bool v) {
  if (v)
    strcpy(_allowTx, "selected");
  else
    _allowTx[0] = '\0';
  save();
}

void ConfigStore::setDisableOled(bool v) {
  if (v)
    strcpy(_disableOled, "selected");
  else
    _disableOled[0] = '\0';
  save();
}

void ConfigStore::setDisableRadio(bool v) {
  if (v)
    strcpy(_disableRadio, "selected");
  else
    _disableRadio[0] = '\0';
  save();
}

void ConfigStore::setAlwaysAP(bool v) {
  if (v)
    strcpy(_alwaysAP, "selected");
  else
    _alwaysAP[0] = '\0';
  save();
}

void ConfigStore::setModemStartup(const char *v) {
  strncpy(_modemStartup, v, sizeof(_modemStartup) - 1);
  save();
  parseModemStartup();
}

void ConfigStore::setAdvancedConfig(const char *v) {
  strncpy(_advancedConfig, v, sizeof(_advancedConfig) - 1);
  save();
  parseAdvancedConf();
}

void ConfigStore::setBoardTemplate(const char *v) {
  strncpy(_boardTemplate, v, sizeof(_boardTemplate) - 1);
  _currentBoardDirty = true;
  save();
}

void ConfigStore::setInterfaceMode(InterfaceMode m) {
  _ifaceMode = m;
  save();
}

// ============================================================
//  Reset
// ============================================================

void ConfigStore::resetAPConfig() {
  _wifiSSID[0] = '\0';
  _wifiPass[0] = '\0';
  strncpy(_apPassword, initialApPassword, sizeof(_apPassword));
  strncpy(_stationName, thingName, sizeof(_stationName));
  save();
}

void ConfigStore::resetAllConfig() {
  _wifiSSID[0] = '\0';
  _wifiPass[0] = '\0';
  strncpy(_apPassword, initialApPassword, sizeof(_apPassword));
  strncpy(_stationName, thingName, sizeof(_stationName));
  strncpy(_mqttPort, MQTT_DEFAULT_PORT, sizeof(_mqttPort));
  strncpy(_mqttServer, MQTT_DEFAULT_SERVER, sizeof(_mqttServer));
  _mqttUser[0] = '\0';
  _mqttPass[0] = '\0';
  _latitude[0] = '\0';
  _longitude[0] = '\0';
  _allowTx[0] = '\0';
  _remoteTune[0] = '\0';
  _telemetry3rd[0] = '\0';
  _testMode[0] = '\0';
  _autoUpdate[0] = '\0';
  _boardTemplate[0] = '\0';
  _modemStartup[0] = '\0';
  _advancedConfig[0] = '\0';
  _ifaceMode = InterfaceMode::WIFI_ONLY;
  save();
}

void ConfigStore::resetModemConfig() {
  _modemStartup[0] = '\0';
  save();
}

// ============================================================
//  Board config
// ============================================================

bool ConfigStore::getBoardConfig(board_t &board) {
  bool ret = true;
  if (!_currentBoardDirty) {
    board = _currentBoard;
    return ret;
  }

  if (_boardTemplate[0] == '\0') {
    _currentBoard = _boards[getBoard()];
  } else {
    ret = parseBoardTemplate(_currentBoard);
  }
  _currentBoardDirty = false;
  board = _currentBoard;
  return ret;
}

bool ConfigStore::parseBoardTemplate(board_t &board) {
  StaticJsonDocument<1024> doc;
  DeserializationError error =
      deserializeJson(doc, (const char *)_boardTemplate);

  if (error.code() != DeserializationError::Ok || !doc.containsKey("radio")) {
    LOG_CONSOLE(
        PSTR("Error: Board template parse failed (%s). Template: %.80s"),
        error.c_str(),
        _boardTemplate);
    return false;
  }

  board.OLED__address = doc["aADDR"];
  board.OLED__SDA = doc["oSDA"];
  board.OLED__SCL = doc["oSCL"];
  board.OLED__RST = doc.containsKey("oRST") ? (uint8_t)doc["oRST"] : UNUSED_PIN;
  board.PROG__BUTTON = doc["pBut"];
  board.BOARD_LED = doc["led"];
  board.L_radio = doc["radio"];
  board.L_NSS = doc["lNSS"];
  board.L_DI00 = doc["lDIO0"];
  board.L_DI01 = doc["lDIO1"];
  board.L_BUSSY = doc["lBUSSY"];
  board.L_RST = doc["lRST"];
  board.L_MISO = doc["lMISO"];
  board.L_MOSI = doc["lMOSI"];
  board.L_SCK = doc["lSCK"];
  board.L_TCXO_V = doc["lTCXOV"];
  board.RX_EN = doc.containsKey("RXEN") ? (uint8_t)doc["RXEN"] : UNUSED_PIN;
  board.TX_EN = doc.containsKey("TXEN") ? (uint8_t)doc["TXEN"] : UNUSED_PIN;
  board.L_SPI = doc.containsKey("lSPI") ? (uint8_t)doc["lSPI"] : (uint8_t)HSPI;

  // Restore the board name from the template — parseBoardTemplate is called
  // every time getBoardConfig() needs to rebuild _currentBoard, so BOARD must
  // be populated here or it will be an empty string and display as "(not
  // configured)".
  board.BOARD =
      doc.containsKey("bNAME") ? doc["bNAME"].as<String>() : String("");

  // Ethernet extension keys
  board.ethEN = doc.containsKey("ethEN") ? doc["ethEN"].as<bool>() : false;
  board.ethPHY = doc.containsKey("ethPHY") ? (uint8_t)doc["ethPHY"] : 0;
  board.ethSPI = doc.containsKey("ethSPI") ? (uint8_t)doc["ethSPI"] : 2;
  board.ethCS = doc.containsKey("ethCS") ? (uint8_t)doc["ethCS"] : UNUSED_PIN;
  board.ethINT =
      doc.containsKey("ethINT") ? (uint8_t)doc["ethINT"] : UNUSED_PIN;
  board.ethRST =
      doc.containsKey("ethRST") ? (uint8_t)doc["ethRST"] : UNUSED_PIN;
  board.ethMISO =
      doc.containsKey("ethMISO") ? (uint8_t)doc["ethMISO"] : UNUSED_PIN;
  board.ethMOSI =
      doc.containsKey("ethMOSI") ? (uint8_t)doc["ethMOSI"] : UNUSED_PIN;
  board.ethSCK =
      doc.containsKey("ethSCK") ? (uint8_t)doc["ethSCK"] : UNUSED_PIN;
  // Internal EMAC fields (only used when ethPHY=0xFF / InternalEmac mode)
  board.ethMDC =
      doc.containsKey("ethMDC") ? (uint8_t)doc["ethMDC"] : UNUSED_PIN;
  board.ethMDIO =
      doc.containsKey("ethMDIO") ? (uint8_t)doc["ethMDIO"] : UNUSED_PIN;
  board.ethPhyAddr =
      doc.containsKey("ethPhyAddr") ? (int8_t)doc["ethPhyAddr"] : 1;
  board.ethPhyType =
      doc.containsKey("ethPhyType") ? (uint8_t)doc["ethPhyType"] : 0;
  board.ethRefClk =
      doc.containsKey("ethRefClk") ? (uint8_t)doc["ethRefClk"] : 0;
  board.ethClkExt =
      doc.containsKey("ethClkExt") ? doc["ethClkExt"].as<bool>() : true;
  board.ethPhyRST =
      doc.containsKey("ethPhyRST") ? (uint8_t)doc["ethPhyRST"] : UNUSED_PIN;
  board.ethOscEN =
      doc.containsKey("ethOscEN") ? (uint8_t)doc["ethOscEN"] : UNUSED_PIN;

  return true;
}

bool ConfigStore::isBoardTemplateModified() const {
  // Empty template → not modified (using the board table directly).
  if (_boardTemplate[0] == '\0')
    return false;

  uint8_t idx = (uint8_t)atoi(_board);
  if (idx >= NUM_BOARDS)
    return true; // unknown index → treat as custom
  const board_t &def = _boards[idx];

  // Parse the stored template and compare every hardware field against the
  // default board entry.  Any mismatch means the user has customised it.
  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, (const char *)_boardTemplate) !=
          DeserializationError::Ok ||
      !doc.containsKey("radio")) {
    return true; // unparseable → treat as custom
  }

  auto u8field = [&](const char *key, uint8_t fallback) -> uint8_t {
    return doc.containsKey(key) ? (uint8_t)doc[key] : fallback;
  };

  if (u8field("aADDR", 0) != def.OLED__address)
    return true;
  if (u8field("oSDA", 0) != def.OLED__SDA)
    return true;
  if (u8field("oSCL", 0) != def.OLED__SCL)
    return true;
  if (u8field("oRST", UNUSED_PIN) != def.OLED__RST)
    return true;
  if (u8field("pBut", 0) != def.PROG__BUTTON)
    return true;
  if (u8field("led", 0) != def.BOARD_LED)
    return true;
  if (u8field("radio", 0) != def.L_radio)
    return true;
  if (u8field("lNSS", 0) != def.L_NSS)
    return true;
  if (u8field("lDIO0", 0) != def.L_DI00)
    return true;
  if (u8field("lDIO1", 0) != def.L_DI01)
    return true;
  if (u8field("lBUSSY", 0) != def.L_BUSSY)
    return true;
  if (u8field("lRST", 0) != def.L_RST)
    return true;
  if (u8field("lMISO", 0) != def.L_MISO)
    return true;
  if (u8field("lMOSI", 0) != def.L_MOSI)
    return true;
  if (u8field("lSCK", 0) != def.L_SCK)
    return true;
  if ((float)doc["lTCXOV"] != def.L_TCXO_V)
    return true;
  if (u8field("RXEN", UNUSED_PIN) != def.RX_EN)
    return true;
  if (u8field("TXEN", UNUSED_PIN) != def.TX_EN)
    return true;
#if CONFIG_IDF_TARGET_ESP32S3
  if (u8field("lSPI", 2) != def.L_SPI)
    return true;
#else
  if (u8field("lSPI", 3) != def.L_SPI)
    return true;
#endif
  bool tplEthEN = doc.containsKey("ethEN") ? doc["ethEN"].as<bool>() : false;
  if (tplEthEN != def.ethEN)
    return true;

  return false; // all fields match → template is unmodified
}

// ---------------------------------------------------------------------------
// Ethernet auto-detection helpers
// ---------------------------------------------------------------------------

#if CONFIG_IDF_TARGET_ESP32S3
// Probe a W5500 on the given SPI pins.  Reads the VERSIONR common register
// (address 0x0039) and checks for the expected value 0x04.
// Performs a hardware reset via the RST pin first (if provided).
static bool probeW5500Spi(uint8_t cs, uint8_t miso, uint8_t mosi, uint8_t sck, uint8_t rst) {
  // Hardware reset — W5500 needs ~150 ms after RST de-assert to be ready
  if (rst != UNUSED_PIN) {
    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
    delay(150);
  }

  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPIClass spi(HSPI);
  spi.begin(sck, miso, mosi, cs);

  // Trama: [ADDR_HI=0x00][ADDR_LO=0x39][CTRL=0x00][dummy]
  // CTRL=0x00 → common block, read, variable-length mode
  spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs, LOW);
  spi.transfer(0x00); // VERSIONR address high byte
  spi.transfer(0x39); // VERSIONR address low  byte
  spi.transfer(0x00); // Control: common block, read, variable length
  uint8_t ver = spi.transfer(0x00);
  digitalWrite(cs, HIGH);
  spi.endTransaction();
  spi.end();

  LOG_CONSOLE(PSTR("[ETH] W5500 VERSIONR = 0x%02X (expected 0x04)"), ver);
  return (ver == 0x04);
}
#endif // CONFIG_IDF_TARGET_ESP32S3

// ---------------------------------------------------------------------------
// Radio auto-detection via RadioLib.
//
// Uses the same RadioLib begin() call path as the real radio initialisation,
// so TCXO supply voltage, BUSY handshake, and chip-version verification are
// all handled correctly by RadioLib's proven driver code.
//
// Note: SX1276 and SX1278 are identical silicon — they respond the same way
// to begin(), so boards sharing the same SPI pins fall back to the first
// matching entry in the board table.
// ---------------------------------------------------------------------------
static bool probeRadio(const board_t &b) {
  if (b.L_NSS == UNUSED_PIN || b.L_SCK == UNUSED_PIN || b.L_radio == 0)
    return false;

  // ── Initialise SPI and probe via RadioLib ──────────────────────────────────
  // Follows the same Module pin layout as Radio.cpp so that RadioLib's
  // begin() properly configures DIO/BUSY/RST pins and handles the full
  // chip-version + TCXO handshake.  No manual gpio_reset_pin() or NSS
  // reclaim is needed — RadioLib's Module::init() takes care of pin setup.
  SPIClass spi(b.L_SPI);
  spi.begin(b.L_SCK, b.L_MISO, b.L_MOSI, b.L_NSS);

  bool found = false;
  int16_t err = RADIOLIB_ERR_UNKNOWN;
  const char *name = "?";
  LOG_CONSOLE(
      PSTR("[Radio] Probing for radio on SPI%d (SCK=%u, MISO=%u, MOSI=%u, "
           "NSS=%u)..."),
      b.L_SPI,
      b.L_SCK,
      b.L_MISO,
      b.L_MOSI,
      b.L_NSS);
  switch (b.L_radio) {
  case RADIO_SX1278: {
    Module mod(b.L_NSS, b.L_DI00, b.L_RST, b.L_DI01, spi);
    SX1278 r(&mod);
    err = r.begin();
    name = "SX1278";
    if (err == RADIOLIB_ERR_NONE)
      r.sleep();
    break;
  }
  case RADIO_SX1276: {
    Module mod(b.L_NSS, b.L_DI00, b.L_RST, b.L_DI01, spi);
    SX1276 r(&mod);
    err = r.begin();
    name = "SX1276";
    if (err == RADIOLIB_ERR_NONE)
      r.sleep();
    break;
  }
  case RADIO_SX1262:
  case RADIO_SX1268: {
    Module mod(b.L_NSS, b.L_DI01, b.L_RST, b.L_BUSSY, spi);
    SX1262 r(&mod);
    err = r.begin();
    LOG_CONSOLE(PSTR("[Radio] SX126x begin() returned %d"), err);
    name = (b.L_radio == RADIO_SX1262) ? "SX1262" : "SX1268";
    if (err == RADIOLIB_ERR_NONE)
      r.sleep();
    break;
  }
  case RADIO_SX1280: {
    Module mod(b.L_NSS, b.L_DI01, b.L_RST, b.L_BUSSY, spi);
    SX1280 r(&mod);
    err = r.begin();
    name = "SX1280";
    if (err == RADIOLIB_ERR_NONE)
      r.sleep();
    break;
  }
  case RADIO_LR1121:
  case RADIO_LR2021: {
    Module mod(b.L_NSS, b.L_DI01, b.L_RST, b.L_BUSSY, spi);
    LR1121 r(&mod);
    err = r.begin();
    name = "LR1121";
    if (err == RADIOLIB_ERR_NONE)
      r.sleep();
    break;
  }
  default:
    break;
  }

  spi.end();
  found = (err == RADIOLIB_ERR_NONE);
  LOG_CONSOLE(PSTR("[Radio] %s err=%d \u2192 %s"), name, err, found ? "OK" : "FAIL");
  return found;
}

#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C3)
// Minimal no-op mediator stubs — MAC init() requires a non-NULL mediator
// (it calls eth->on_state_changed() internally).  For a probe-only flow we
// don't route frames or PHY interrupts so all callbacks can be no-ops.
static esp_err_t _probe_state_cb(esp_eth_mediator_t *, esp_eth_state_t, void *) {
  return ESP_OK;
}
static esp_err_t _probe_phy_read(esp_eth_mediator_t *, uint32_t, uint32_t, uint32_t *) {
  return ESP_OK;
}
static esp_err_t _probe_phy_write(esp_eth_mediator_t *, uint32_t, uint32_t, uint32_t) {
  return ESP_OK;
}
static esp_err_t _probe_stack_rx(esp_eth_mediator_t *, uint8_t *, uint32_t) {
  return ESP_OK;
}
static esp_eth_mediator_t s_probe_mediator = {
    .phy_reg_read = _probe_phy_read,
    .phy_reg_write = _probe_phy_write,
    .stack_input = _probe_stack_rx,
    .on_state_changed = _probe_state_cb,
};

// Use the ESP-IDF EMAC MAC layer to probe for a PHY via MDIO.
// Enables the oscillator, initialises just the MAC (not a full ETH stack),
// reads PHY ID register 2 (PHYID1) and then immediately tears down the MAC.
// Returns true when the PHY responds with a valid (non-trivial) ID value.
static bool probeInternalEmac(uint8_t mdcPin, uint8_t mdioPin, uint8_t phyAddr, uint8_t refClkPin, bool clkExt, uint8_t oscEN) {
  LOG_CONSOLE(PSTR("[ETH probe] EMAC probe: MDC=%u MDIO=%u PHY=%u refClk=%u "
                   "clkExt=%d oscEN=%u"),
              mdcPin,
              mdioPin,
              phyAddr,
              refClkPin,
              (int)clkExt,
              oscEN);

  // 1. Enable oscillator; give the PLL ~100 ms to lock before EMAC init
  if (oscEN != UNUSED_PIN) {
    gpio_set_direction((gpio_num_t)oscEN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)oscEN, 1);
    LOG_CONSOLE(PSTR("[ETH probe] oscEN GPIO%u HIGH — waiting 100ms"), oscEN);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  // 2. Configure REF_CLK pin as input when the PHY drives the clock
  if (clkExt) {
    gpio_set_direction((gpio_num_t)refClkPin, GPIO_MODE_INPUT);
    LOG_CONSOLE(PSTR("[ETH probe] refClk GPIO%u set INPUT — waiting 50ms"),
                refClkPin);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  eth_mac_config_t macCfg = ETH_MAC_DEFAULT_CONFIG();
  macCfg.sw_reset_timeout_ms = 200; // fail fast when clock absent

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  LOG_CONSOLE(PSTR("[ETH probe] Using IDF 5.x code path"));
  eth_esp32_emac_config_t emacCfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  emacCfg.smi_mdc_gpio_num = (gpio_num_t)mdcPin;
  emacCfg.smi_mdio_gpio_num = (gpio_num_t)mdioPin;
#pragma GCC diagnostic pop
  emacCfg.clock_config.rmii.clock_mode =
      clkExt ? EMAC_CLK_EXT_IN : EMAC_CLK_OUT;
  emacCfg.clock_config.rmii.clock_gpio = (emac_rmii_clock_gpio_t)refClkPin;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emacCfg, &macCfg);
#else
  LOG_CONSOLE(PSTR("[ETH probe] Using IDF 4.x code path"));
  macCfg.smi_mdc_gpio_num = mdcPin;
  macCfg.smi_mdio_gpio_num = mdioPin;
  macCfg.clock_config.rmii.clock_mode = clkExt ? EMAC_CLK_EXT_IN : EMAC_CLK_OUT;
  macCfg.clock_config.rmii.clock_gpio = (emac_rmii_clock_gpio_t)refClkPin;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&macCfg);
#endif

  if (!mac) {
    LOG_CONSOLE(PSTR("[ETH probe] esp_eth_mac_new_esp32 returned NULL"));
    if (oscEN != UNUSED_PIN) {
      gpio_set_level((gpio_num_t)oscEN, 0);
      gpio_set_direction((gpio_num_t)oscEN, GPIO_MODE_INPUT);
    }
    return false;
  }
  LOG_CONSOLE(PSTR("[ETH probe] MAC created OK"));

  // Link a dummy mediator so MAC init() doesn't crash dereferencing a NULL
  // eth pointer when it calls eth->on_state_changed().
  mac->set_mediator(mac, &s_probe_mediator);

  esp_err_t initRet = mac->init(mac);
  if (initRet != ESP_OK) {
    LOG_CONSOLE(PSTR("[ETH probe] MAC init FAILED: 0x%x"), (unsigned)initRet);
    mac->del(mac);
    if (oscEN != UNUSED_PIN) {
      gpio_set_level((gpio_num_t)oscEN, 0);
      gpio_set_direction((gpio_num_t)oscEN, GPIO_MODE_INPUT);
    }
    return false;
  }
  LOG_CONSOLE(PSTR("[ETH probe] MAC init OK"));

  uint32_t phyId = 0;
  esp_err_t readRet = mac->read_phy_reg(mac, phyAddr, 2, &phyId);
  bool found = (readRet == ESP_OK) && (phyId != 0x0000 && phyId != 0xFFFF);
  LOG_CONSOLE(PSTR("[ETH probe] read_phy_reg ret=0x%x phyId=0x%04X → %s"),
              (unsigned)readRet,
              (unsigned)phyId,
              found ? "FOUND" : "NOT FOUND");

  // Log BEFORE deinit/del: mac->deinit() temporarily stalls the UART DMA
  // via EMAC driver teardown, causing any log printed after it to be lost.
  Serial.flush();

  mac->deinit(mac);
  mac->del(mac);

  // Brief pause so the EMAC DMA teardown finishes before UART is used again.
  vTaskDelay(pdMS_TO_TICKS(10));

  if (!found && oscEN != UNUSED_PIN) {
    gpio_set_level((gpio_num_t)oscEN, 0);
    gpio_set_direction((gpio_num_t)oscEN, GPIO_MODE_INPUT);
  }
  return found;
}
#endif // !ESP32S3 && !ESP32C3

// ---------------------------------------------------------------------------
// boardToTemplateJson() — serializes a board_t to the JSON template format
// so the user can see and edit it in the web UI "Board Template" field.
// ---------------------------------------------------------------------------
static void boardToTemplateJson(const board_t &b, char *buf, size_t bufLen) {
  StaticJsonDocument<1024> doc;
  doc["aADDR"] = b.OLED__address;
  doc["oSDA"] = b.OLED__SDA;
  doc["oSCL"] = b.OLED__SCL;
  doc["oRST"] = b.OLED__RST;
  doc["pBut"] = b.PROG__BUTTON;
  doc["led"] = b.BOARD_LED;
  doc["radio"] = b.L_radio;
  doc["lNSS"] = b.L_NSS;
  doc["lDIO0"] = b.L_DI00;
  doc["lDIO1"] = b.L_DI01;
  doc["lBUSSY"] = b.L_BUSSY;
  doc["lRST"] = b.L_RST;
  doc["lMISO"] = b.L_MISO;
  doc["lMOSI"] = b.L_MOSI;
  doc["lSCK"] = b.L_SCK;
  doc["lTCXOV"] = b.L_TCXO_V;
  doc["RXEN"] = b.RX_EN;
  doc["TXEN"] = b.TX_EN;
  doc["lSPI"] = b.L_SPI;
  doc["bNAME"] = b.BOARD;
  if (b.ethEN) {
    doc["ethEN"] = true;
    doc["ethPHY"] = b.ethPHY;
    if (b.ethPHY != 0xFF) {
      // SPI ethernet (W5500, DM9051, KSZ8851SNL)
      doc["ethSPI"] = b.ethSPI;
      doc["ethCS"] = b.ethCS;
      doc["ethINT"] = b.ethINT;
      doc["ethRST"] = b.ethRST;
      doc["ethMISO"] = b.ethMISO;
      doc["ethMOSI"] = b.ethMOSI;
      doc["ethSCK"] = b.ethSCK;
    } else {
      // Internal EMAC (LAN8720, etc.)
      doc["ethMDC"] = b.ethMDC;
      doc["ethMDIO"] = b.ethMDIO;
      doc["ethPhyAddr"] = b.ethPhyAddr;
      doc["ethPhyType"] = b.ethPhyType;
      doc["ethRefClk"] = b.ethRefClk;
      doc["ethClkExt"] = b.ethClkExt;
      doc["ethPhyRST"] = b.ethPhyRST;
      doc["ethOscEN"] = b.ethOscEN;
    }
  }
  serializeJson(doc, buf, bufLen);
}

// ---------------------------------------------------------------------------
// boardDetection() — probes hardware to auto-select the board configuration.
//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │  Detection algorithm (3-phase)                                          │
// │                                                                         │
// │  Phase 1 — OLED probe                                                   │
// │    For each unique I2C bus (addr, SDA, SCL) in the board table:         │
// │      • RST pulse if present + 100 ms SSD1306 startup delay              │
// │      • Wire.begin(SDA, SCL) + Wire.beginTransmission(addr)              │
// │      • First bus to ACK → detected OLED bus; break                      │
// │                                                                         │
// │    If OLED found → Phase 3a (radio probe among OLED-matching boards)    │
// │      Candidates = all boards with same (addr, SDA, SCL).                │
// │      First candidate where radio SPI confirms → selected.               │
// │      Fallback (radio inconclusive) → first candidate.                   │
// │                                                                         │
// │    If NO OLED → Phase 2 (Ethernet probe)                                │
// │      For each board with ethEN=true: probe W5500 SPI or EMAC.           │
// │      If detected → select, set InterfaceMode::BOTH, return.             │
// │                                                                         │
// │    If no OLED and no ETH → Phase 2b (radio-only boards)                 │
// │      Candidates = boards where OLED__address==0 AND ethEN==false.       │
// │      First candidate where radio SPI confirms → selected.               │
// │      Enables boards without OLED or ETH (e.g. Heltec WSL V3).          │
// │                                                                         │
// │  Radio disambiguation (probeRadioSpi):                                  │
// │    SX127x (SX1276/1278) : reg 0x42 = 0x12  (same silicon → RF_SX127X)  │
// │    SX126x (SX1262/1268) : SyncWord MSB 0x0740 = 0x14                   │
// │    SX1280               : GetStatus circuit-mode bits [6:4] != 0        │
// │    LR11xx (LR1121/2021) : GetVersion 0x0307, 32-bit != 0/0xFFFFFFFF    │
// │                                                                         │
// │  Known limitation: SX1276 and SX1278 are identical silicon —            │
// │  they respond the same way to SPI probe → unresolvable collision.       │
// └─────────────────────────────────────────────────────────────────────────┘
// ---------------------------------------------------------------------------
void ConfigStore::boardDetection() {
  LOG_CONSOLE(PSTR("Automatic board detection running..."));

#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C3)
  if (strcmp(ESP.getChipModel(), "ESP32-PICO-D4") == 0)
    return;
#endif

  // ── Phase 1: OLED probe ──────────────────────────────────────────────────
  // Iterate unique I2C buses; stop at the first bus that ACKs.
  uint8_t oledAddr = 0, oledSda = UNUSED_PIN, oledScl = UNUSED_PIN;

  // Suppress IDF i2c.master NACK errors during probe — NACKs on buses where
  // no display is present are expected and handled below.
  esp_log_level_set("i2c.master", ESP_LOG_NONE);
  {
    // Track already-tried (SDA, SCL) pairs to avoid re-probing the same bus
    // for boards that share the same I2C pins (very common).
    uint8_t triedSda[8], triedScl[8];
    uint8_t triedCount = 0;

    for (uint8_t ite = 0; ite < NUM_BOARDS && oledAddr == 0; ite++) {
      if (_boards[ite].OLED__address == 0)
        continue;

      uint8_t sda = _boards[ite].OLED__SDA;
      uint8_t scl = _boards[ite].OLED__SCL;

      // Skip buses already probed
      bool alreadyTried = false;
      for (uint8_t t = 0; t < triedCount; t++) {
        if (triedSda[t] == sda && triedScl[t] == scl) {
          alreadyTried = true;
          break;
        }
      }
      if (alreadyTried)
        continue;
      if (triedCount < 8) {
        triedSda[triedCount] = sda;
        triedScl[triedCount++] = scl;
      }

      // RST pulse — only once per bus
      if (_boards[ite].OLED__RST != UNUSED_PIN) {
        pinMode(_boards[ite].OLED__RST, OUTPUT);
        digitalWrite(_boards[ite].OLED__RST, LOW);
        delay(25);
        digitalWrite(_boards[ite].OLED__RST, HIGH);
        // SSD1306 requires ~100 ms after RESET before I2C is ready.
        // On IDF 5.x (pioarduino) Wire.begin() is fast enough to race the
        // display without this delay.
        delay(100);
      }

      Wire.begin(sda, scl);
      Wire.beginTransmission(_boards[ite].OLED__address);
      if (!Wire.endTransmission()) {
        oledAddr = _boards[ite].OLED__address;
        oledSda = sda;
        oledScl = scl;
      }
    }
  }
  esp_log_level_set("i2c.master", ESP_LOG_WARN); // restore normal log level

  if (oledAddr != 0) {
    LOG_CONSOLE(PSTR("OLED found: 0x%02X (SDA=%u SCL=%u)"), oledAddr, oledSda, oledScl);
  } else {
    LOG_CONSOLE(PSTR("OLED probe: no display found"));
  }

  if (oledAddr != 0) {
    // ── Phase 3a: Radio probe among OLED-matching candidates ─────────────
    int8_t firstCandidate = -1;
    for (uint8_t ite = 0; ite < NUM_BOARDS; ite++) {
      if (_boards[ite].OLED__address != oledAddr)
        continue;
      if (_boards[ite].OLED__SDA != oledSda)
        continue;
      if (_boards[ite].OLED__SCL != oledScl)
        continue;
      if (firstCandidate < 0)
        firstCandidate = (int8_t)ite;
      LOG_CONSOLE(PSTR("Probing radio for: %s"), _boards[ite].BOARD.c_str());
      if (probeRadio(_boards[ite])) {
        LOG_CONSOLE(PSTR("Board confirmed: %s"), _boards[ite].BOARD.c_str());
        itoa(ite, _board, 10);
        boardToTemplateJson(_boards[ite], _boardTemplate, sizeof(_boardTemplate));
        save();
        return;
      }
    }
    // Fallback: radio inconclusive → first OLED-matching board
    if (firstCandidate >= 0) {
      LOG_CONSOLE(PSTR("Radio probe inconclusive — fallback: %s"),
                  _boards[firstCandidate].BOARD.c_str());
      itoa(firstCandidate, _board, 10);
      boardToTemplateJson(_boards[firstCandidate], _boardTemplate, sizeof(_boardTemplate));
      save();
    }
    return;
  }

  // ── Phase 2: No OLED — Ethernet probe ────────────────────────────────────
  for (uint8_t ite = 0; ite < NUM_BOARDS; ite++) {
    if (!_boards[ite].ethEN)
      continue;

    bool detected = false;

    if (_boards[ite].ethPHY != 0xFF) {
      // SPI ethernet (W5500, etc.)
#if CONFIG_IDF_TARGET_ESP32S3
      LOG_CONSOLE(
          PSTR("Probing %s (SPI CS=%u SCK=%u MOSI=%u MISO=%u RST=%u)..."),
          _boards[ite].BOARD.c_str(),
          _boards[ite].ethCS,
          _boards[ite].ethSCK,
          _boards[ite].ethMOSI,
          _boards[ite].ethMISO,
          _boards[ite].ethRST);
      detected = probeW5500Spi(_boards[ite].ethCS, _boards[ite].ethMISO, _boards[ite].ethMOSI, _boards[ite].ethSCK, _boards[ite].ethRST);
#endif
    } else {
      // Internal EMAC (LAN8720, etc.)
#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C3)
      LOG_CONSOLE(PSTR("Probing %s (EMAC MDC=%u MDIO=%u PHYaddr=%d)..."),
                  _boards[ite].BOARD.c_str(),
                  _boards[ite].ethMDC,
                  _boards[ite].ethMDIO,
                  _boards[ite].ethPhyAddr);
      detected =
          probeInternalEmac(_boards[ite].ethMDC, _boards[ite].ethMDIO, _boards[ite].ethPhyAddr, _boards[ite].ethRefClk, _boards[ite].ethClkExt, _boards[ite].ethOscEN);
#endif
    }

    if (detected) {
      LOG_CONSOLE(PSTR("Ethernet detected: %s"), _boards[ite].BOARD.c_str());
      boardToTemplateJson(_boards[ite], _boardTemplate, sizeof(_boardTemplate));
      itoa(ite, _board, 10);
      _currentBoardDirty = true;
      _ifaceMode = InterfaceMode::BOTH;
      save();
      return;
    }
  }

  // ── Phase 2b: No OLED, no ETH — radio-only boards ────────────────────────
  // Boards that have neither OLED nor ethernet (e.g. Heltec Wireless Stick Lite
  // V3) can still be identified by probing their radio SPI bus directly.
  for (uint8_t ite = 0; ite < NUM_BOARDS; ite++) {
    if (_boards[ite].OLED__address != 0)
      continue; // skip OLED boards
    if (_boards[ite].ethEN)
      continue; // skip ETH boards
    LOG_CONSOLE(PSTR("Probing radio-only board: %s"),
                _boards[ite].BOARD.c_str());
    if (probeRadio(_boards[ite])) {
      LOG_CONSOLE(PSTR("Board confirmed: %s"), _boards[ite].BOARD.c_str());
      itoa(ite, _board, 10);
      boardToTemplateJson(_boards[ite], _boardTemplate, sizeof(_boardTemplate));
      save();
      return;
    }
  }

  LOG_CONSOLE(PSTR("Board detection: no board found"));
}

// ============================================================
//  Parse advanced config
// ============================================================

void ConfigStore::parseAdvancedConf() {
  if (!strlen(_advancedConfig))
    return;

  StaticJsonDocument<512> doc;
  deserializeJson(doc, (const char *)_advancedConfig);

  if (doc.containsKey(F("dmode")))
    Log::setLogLevel(doc["dmode"]);
  if (doc.containsKey(F("flipOled")))
    _advConf.flipOled = doc["flipOled"];
  if (doc.containsKey(F("dnOled")))
    _advConf.dnOled = doc["dnOled"];
  if (doc.containsKey(F("lowPower")))
    _advConf.lowPower = doc["lowPower"];
}

// ============================================================
//  Parse modem startup
// ============================================================

void ConfigStore::parseModemStartup() {
  if (_modemStartup[0] == '\0')
    return;

  StaticJsonDocument<768> doc;
  DeserializationError error =
      deserializeJson(doc, (const char *)_modemStartup);

  if (error.code() != DeserializationError::Ok || !doc.containsKey("mode")) {
    LOG_CONSOLE(
        PSTR("ERROR: Your modem config is invalid. Resetting to default"));
    _modemStartup[0] = '\0';
    save();
    return;
  }

  ModemInfo &m = status.modeminfo;
  const char *mode = doc["mode"].as<const char *>();
  strncpy(m.modem_mode, mode ? mode : "", sizeof(m.modem_mode) - 1);
  m.modem_mode[sizeof(m.modem_mode) - 1] = '\0';
  strcpy(m.satellite, doc["sat"].as<const char *>());
  m.NORAD = doc["NORAD"];

  if (strcmp(m.modem_mode, "LoRa") == 0) {
    m.frequency = doc["freq"];
    m.bw = doc["bw"];
    m.sf = doc["sf"];
    m.cr = doc["cr"];
    m.sw = doc["sw"];
    m.power = doc["pwr"];
    m.preambleLength = doc["pl"];
    m.gain = doc["gain"];
    m.crc = doc["crc"];
    m.fldro = doc["fldro"];
    m.iIQ = doc["iIQ"] ? doc["iIQ"].as<bool>() : false;
    m.len = doc["len"] ? doc["len"].as<int>() : 0;
  } else {
    m.frequency = doc["freq"];
    m.bw = doc["bw"];
    m.bitrate = doc["br"];
    m.freqDev = doc["fd"];
    m.power = doc["pwr"];
    m.preambleLength = doc["pl"];
    m.OOK = doc["ook"];
    m.len = doc["len"];
    m.swSize = doc["fsw"].size();
    for (int i = 0; i < 8; i++) {
      m.fsw[i] = (i < m.swSize) ? (uint8_t)doc["fsw"][i] : 0;
    }
    m.enc = doc["enc"];
    m.whitening_seed = doc["ws"];
    m.framing = doc["fr"];
    m.crc_by_sw = doc["cSw"];
    m.crc_nbytes = doc["cB"];
    m.crc_init = doc["cI"];
    m.crc_poly = doc["cP"];
    m.crc_finalxor = doc["cF"];
    m.crc_refIn = doc["cRI"];
    m.crc_refOut = doc["cRO"];
  }

  uint8_t filterSize = doc["filter"].size();
  for (int i = 0; i < 8; i++) {
    status.modeminfo.filter[i] =
        (i < filterSize) ? (uint8_t)doc["filter"][i] : 0;
  }
}

// ============================================================
//  Print config
// ============================================================

void ConfigStore::printConfig() {
  LOG_DEBUG(PSTR("MQTT Port: %u\nMQTT Server: %s\nMQTT User: %s\nLatitude: "
                 "%f\nLongitude: %f"),
            getMqttPort(),
            getMqttServer(),
            getMqttUser(),
            getLatitude(),
            getLongitude());
  LOG_DEBUG(PSTR("tz: %s\nOLED Bright: %u\nTX %s"), getTZ(), getOledBright(), getAllowTx() ? "Enable" : "Disable");
  if (_boardTemplate[0] != '\0')
    LOG_DEBUG(PSTR("board_template: %s"), _boardTemplate);
  else
    LOG_DEBUG(PSTR("board: %u --> %s"), getBoard(), _boards[getBoard()].BOARD.c_str());
}
