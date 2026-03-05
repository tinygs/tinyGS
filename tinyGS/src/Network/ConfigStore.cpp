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
#include <Wire.h>

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
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 35, RADIO_SX1278, 8, 6, 14, UNUSED_PIN, 12, 11, 10, 9, 0.0f, UNUSED_PIN, UNUSED_PIN, "Custom ESP32-S3 433MHz SX1278"};
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 3, RADIO_SX1262, 10, UNUSED_PIN, 1, 4, 5, 13, 11, 12, 1.6f, UNUSED_PIN, UNUSED_PIN, "433 Mhz TTGO T-Beam Sup SX1262 V1.0"};
  _boards[i++] = {0x3c, 17, 18, UNUSED_PIN, 0, 37, RADIO_SX1280, 7, UNUSED_PIN, 9, UNUSED_PIN, 8, 3, 6, 5, 0.0f, 21, 10, "2.4Ghz LILYGO SX1280"};
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
#endif
}

// ============================================================
//  NVS Load/Save
// ============================================================

void ConfigStore::loadFromNVS() {
  _prefs.begin("tinygs", true); // read-only

  _prefs.getString("stName",    _stationName,   sizeof(_stationName));
  _prefs.getString("apPass",    _apPassword,    sizeof(_apPassword));
  _prefs.getString("wifiSSID",  _wifiSSID,      sizeof(_wifiSSID));
  _prefs.getString("wifiPass",  _wifiPass,      sizeof(_wifiPass));
  _prefs.getString("lat",       _latitude,      sizeof(_latitude));
  _prefs.getString("lng",       _longitude,     sizeof(_longitude));
  _prefs.getString("tz",        _tz,            sizeof(_tz));
  _prefs.getString("mqttSrv",   _mqttServer,    sizeof(_mqttServer));
  _prefs.getString("mqttPort",  _mqttPort,      sizeof(_mqttPort));
  _prefs.getString("mqttUser",  _mqttUser,      sizeof(_mqttUser));
  _prefs.getString("mqttPass",  _mqttPass,      sizeof(_mqttPass));
  _prefs.getString("board",     _board,         sizeof(_board));
  _prefs.getString("oledBr",    _oledBright,    sizeof(_oledBright));
  _prefs.getString("allowTx",   _allowTx,       sizeof(_allowTx));
  _prefs.getString("remoteTn",  _remoteTune,    sizeof(_remoteTune));
  _prefs.getString("tele3rd",   _telemetry3rd,  sizeof(_telemetry3rd));
  _prefs.getString("testMode",  _testMode,      sizeof(_testMode));
  _prefs.getString("autoUpd",   _autoUpdate,    sizeof(_autoUpdate));
  _prefs.getString("disOled",   _disableOled,   sizeof(_disableOled));
  _prefs.getString("disRadio",  _disableRadio,  sizeof(_disableRadio));
  _prefs.getString("alwaysAP",  _alwaysAP,      sizeof(_alwaysAP));
  _prefs.getString("brdTpl",    _boardTemplate,  sizeof(_boardTemplate));
  _prefs.getString("modemSt",   _modemStartup,   sizeof(_modemStartup));
  _prefs.getString("advCfg",    _advancedConfig, sizeof(_advancedConfig));
  _ifaceMode = (InterfaceMode)_prefs.getUChar("ifaceMode", (uint8_t)InterfaceMode::WIFI_ONLY);

  _prefs.end();
}

void ConfigStore::saveToNVS() {
  _prefs.begin("tinygs", false); // read-write

  _prefs.putString("stName",    _stationName);
  _prefs.putString("apPass",    _apPassword);
  _prefs.putString("wifiSSID",  _wifiSSID);
  _prefs.putString("wifiPass",  _wifiPass);
  _prefs.putString("lat",       _latitude);
  _prefs.putString("lng",       _longitude);
  _prefs.putString("tz",        _tz);
  _prefs.putString("mqttSrv",   _mqttServer);
  _prefs.putString("mqttPort",  _mqttPort);
  _prefs.putString("mqttUser",  _mqttUser);
  _prefs.putString("mqttPass",  _mqttPass);
  _prefs.putString("board",     _board);
  _prefs.putString("oledBr",    _oledBright);
  _prefs.putString("allowTx",   _allowTx);
  _prefs.putString("remoteTn",  _remoteTune);
  _prefs.putString("tele3rd",   _telemetry3rd);
  _prefs.putString("testMode",  _testMode);
  _prefs.putString("autoUpd",   _autoUpdate);
  _prefs.putString("disOled",   _disableOled);
  _prefs.putString("disRadio",  _disableRadio);
  _prefs.putString("alwaysAP",  _alwaysAP);
  _prefs.putString("brdTpl",    _boardTemplate);
  _prefs.putString("modemSt",   _modemStartup);
  _prefs.putString("advCfg",    _advancedConfig);
  _prefs.putUChar("ifaceMode",  (uint8_t)_ifaceMode);
  _prefs.putString("cfgVer",    configVersion);

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
    // First boot or migrated from IotWebConf2 → attempt migration
    if (!migrateFromEEPROM()) {
      // Fresh install: set defaults
      strncpy(_stationName, thingName, sizeof(_stationName));
      strncpy(_mqttServer, MQTT_DEFAULT_SERVER, sizeof(_mqttServer));
      strncpy(_mqttPort, MQTT_DEFAULT_PORT, sizeof(_mqttPort));
      strncpy(_oledBright, "100", sizeof(_oledBright));
      strncpy(_remoteTune, "selected", sizeof(_remoteTune));
      strncpy(_telemetry3rd, "selected", sizeof(_telemetry3rd));
      strncpy(_autoUpdate, "selected", sizeof(_autoUpdate));
      saveToNVS();
    } else {
      validConfig = true;
    }
  } else {
    loadFromNVS();
    validConfig = true;
  }

  // No board selected → auto detect
  if (!strcmp(_board, "")) {
    boardDetection();
  }

  if (strlen(_advancedConfig))
    parseAdvancedConf();

  parseModemStartup();

  return validConfig;
}

// ============================================================
//  Migration from old EEPROM (IotWebConf2) format
// ============================================================

bool ConfigStore::migrateFromEEPROM() {
  // IotWebConf2 stores data in EEPROM (Preferences namespace "iotwebconf")
  // We attempt to read from that namespace and migrate to ours
  Preferences oldPrefs;
  
  // IotWebConf2 uses a namespace based composite key system in EEPROM
  // Since EEPROM on ESP32 is emulated via NVS, check for its namespace
  if (!oldPrefs.begin("iotwebconf", true)) {
    return false;
  }

  // Check if old config exists
  String oldVer = oldPrefs.getString("ConfigVersion", "");
  if (oldVer.length() == 0) {
    oldPrefs.end();
    return false;
  }

  LOG_CONSOLE(PSTR("Migrating config from IotWebConf2 EEPROM to NVS..."));

  // Read old values - IotWebConf2 stores them with specific keys
  oldPrefs.getString("ThingName",      _stationName,  sizeof(_stationName));
  oldPrefs.getString("APPassword",     _apPassword,   sizeof(_apPassword));
  oldPrefs.getString("WiFiSsid",       _wifiSSID,     sizeof(_wifiSSID));
  oldPrefs.getString("WiFiPassword",   _wifiPass,     sizeof(_wifiPass));
  oldPrefs.getString("lat",            _latitude,     sizeof(_latitude));
  oldPrefs.getString("lng",            _longitude,    sizeof(_longitude));
  oldPrefs.getString("tz",             _tz,           sizeof(_tz));
  oldPrefs.getString("mqtt_server",    _mqttServer,   sizeof(_mqttServer));
  oldPrefs.getString("mqtt_port",      _mqttPort,     sizeof(_mqttPort));
  oldPrefs.getString("mqtt_user",      _mqttUser,     sizeof(_mqttUser));
  oldPrefs.getString("mqtt_pass",      _mqttPass,     sizeof(_mqttPass));
  oldPrefs.getString("board",          _board,        sizeof(_board));
  oldPrefs.getString("oled_bright",    _oledBright,   sizeof(_oledBright));
  oldPrefs.getString("tx",             _allowTx,      sizeof(_allowTx));
  oldPrefs.getString("remote_tune",    _remoteTune,   sizeof(_remoteTune));
  oldPrefs.getString("telemetry3rd",   _telemetry3rd, sizeof(_telemetry3rd));
  oldPrefs.getString("test",           _testMode,     sizeof(_testMode));
  oldPrefs.getString("auto_update",    _autoUpdate,   sizeof(_autoUpdate));
  oldPrefs.getString("board_template", _boardTemplate, sizeof(_boardTemplate));
  oldPrefs.getString("modem_startup",  _modemStartup,  sizeof(_modemStartup));
  oldPrefs.getString("advanced_config",_advancedConfig,sizeof(_advancedConfig));

  oldPrefs.end();

  // Apply defaults if empty
  if (_mqttServer[0] == '\0') strncpy(_mqttServer, MQTT_DEFAULT_SERVER, sizeof(_mqttServer));
  if (_mqttPort[0] == '\0')   strncpy(_mqttPort, MQTT_DEFAULT_PORT, sizeof(_mqttPort));
  if (_stationName[0] == '\0') strncpy(_stationName, thingName, sizeof(_stationName));

  // Save to new NVS namespace
  saveToNVS();

  // Clean up old namespace
  Preferences cleanup;
  cleanup.begin("iotwebconf", false);
  cleanup.clear();
  cleanup.end();

  LOG_CONSOLE(PSTR("Migration complete. Old EEPROM config cleared."));
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

void ConfigStore::setThingName(const char* n)  { strncpy(_stationName, n, sizeof(_stationName) - 1); save(); }
void ConfigStore::setApPassword(const char* p) { strncpy(_apPassword, p, sizeof(_apPassword) - 1); save(); }
void ConfigStore::setWifiSSID(const char* s)   { strncpy(_wifiSSID, s, sizeof(_wifiSSID) - 1); save(); }
void ConfigStore::setWifiPassword(const char* p){ strncpy(_wifiPass, p, sizeof(_wifiPass) - 1); save(); }
void ConfigStore::setLat(const char* v)        { strncpy(_latitude, v, sizeof(_latitude) - 1); save(); }
void ConfigStore::setLon(const char* v)        { strncpy(_longitude, v, sizeof(_longitude) - 1); save(); }
void ConfigStore::setTZ(const char* v)         { strncpy(_tz, v, sizeof(_tz) - 1); save(); }
void ConfigStore::setMqttServer(const char* v) { strncpy(_mqttServer, v, sizeof(_mqttServer) - 1); save(); }
void ConfigStore::setMqttPort(const char* v)   { strncpy(_mqttPort, v, sizeof(_mqttPort) - 1); save(); }
void ConfigStore::setMqttUser(const char* v)   { strncpy(_mqttUser, v, sizeof(_mqttUser) - 1); save(); }
void ConfigStore::setMqttPass(const char* v)   { strncpy(_mqttPass, v, sizeof(_mqttPass) - 1); save(); }
void ConfigStore::setBoard(const char* v)      { strncpy(_board, v, sizeof(_board) - 1); _currentBoardDirty = true; save(); }
void ConfigStore::setOledBright(const char* v) { strncpy(_oledBright, v, sizeof(_oledBright) - 1); save(); }

void ConfigStore::setAllowTx(bool v) {
  if (v) strcpy(_allowTx, "selected");
  else _allowTx[0] = '\0';
  save();
}

void ConfigStore::setDisableOled(bool v) {
  if (v) strcpy(_disableOled, "selected");
  else _disableOled[0] = '\0';
  save();
}

void ConfigStore::setDisableRadio(bool v) {
  if (v) strcpy(_disableRadio, "selected");
  else _disableRadio[0] = '\0';
  save();
}

void ConfigStore::setAlwaysAP(bool v) {
  if (v) strcpy(_alwaysAP, "selected");
  else _alwaysAP[0] = '\0';
  save();
}

void ConfigStore::setModemStartup(const char* v) {
  strncpy(_modemStartup, v, sizeof(_modemStartup) - 1);
  save();
  parseModemStartup();
}

void ConfigStore::setAdvancedConfig(const char* v) {
  strncpy(_advancedConfig, v, sizeof(_advancedConfig) - 1);
  save();
  parseAdvancedConf();
}

void ConfigStore::setBoardTemplate(const char* v) {
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

bool ConfigStore::getBoardConfig(board_t& board) {
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

bool ConfigStore::parseBoardTemplate(board_t& board) {
  StaticJsonDocument<640> doc;
  DeserializationError error = deserializeJson(doc, (const char*)_boardTemplate);

  if (error.code() != DeserializationError::Ok || !doc.containsKey("radio")) {
    LOG_CONSOLE(PSTR("Error: Your Board template is not valid. Unable to finish setup."));
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
  board.L_SPI = doc.containsKey("lSPI") ? (uint8_t)doc["lSPI"] : 2;

  // Ethernet extension keys
  board.ethEN   = doc.containsKey("ethEN")   ? doc["ethEN"].as<bool>() : false;
  board.ethPHY  = doc.containsKey("ethPHY")  ? (uint8_t)doc["ethPHY"]  : 0;
  board.ethSPI  = doc.containsKey("ethSPI")  ? (uint8_t)doc["ethSPI"]  : 2;
  board.ethCS   = doc.containsKey("ethCS")   ? (uint8_t)doc["ethCS"]   : UNUSED_PIN;
  board.ethINT  = doc.containsKey("ethINT")  ? (uint8_t)doc["ethINT"]  : UNUSED_PIN;
  board.ethRST  = doc.containsKey("ethRST")  ? (uint8_t)doc["ethRST"]  : UNUSED_PIN;
  board.ethMISO = doc.containsKey("ethMISO") ? (uint8_t)doc["ethMISO"] : UNUSED_PIN;
  board.ethMOSI = doc.containsKey("ethMOSI") ? (uint8_t)doc["ethMOSI"] : UNUSED_PIN;
  board.ethSCK  = doc.containsKey("ethSCK")  ? (uint8_t)doc["ethSCK"]  : UNUSED_PIN;

  return true;
}

void ConfigStore::boardDetection() {
  LOG_ERROR(PSTR("Automatic board detection running... "));

#if CONFIG_IDF_TARGET_ESP32S3
  // not implemented yet for S3
#elif CONFIG_IDF_TARGET_ESP32C3
  // not implemented yet for C3
#else
  if (strcmp(ESP.getChipModel(), "ESP32-PICO-D4") != 0) {
    for (uint8_t ite = 0; ite < NUM_BOARDS; ite++) {
      LOG_ERROR(PSTR("%s"), _boards[ite].BOARD.c_str());
      if (_boards[ite].OLED__RST != UNUSED_PIN) {
        pinMode(_boards[ite].OLED__RST, OUTPUT);
        digitalWrite(_boards[ite].OLED__RST, LOW);
        delay(25);
        digitalWrite(_boards[ite].OLED__RST, HIGH);
      }
      Wire.begin(_boards[ite].OLED__SDA, _boards[ite].OLED__SCL);
      Wire.beginTransmission(_boards[ite].OLED__address);
      if (!Wire.endTransmission()) {
        LOG_ERROR(PSTR("Compatible OLED FOUND"));
        itoa(ite, _board, 10);
        return;
      } else {
        LOG_ERROR(PSTR("Not Compatible board found, please select it manually on the web config panel"));
      }
    }
  }
#endif
}

// ============================================================
//  Parse advanced config
// ============================================================

void ConfigStore::parseAdvancedConf() {
  if (!strlen(_advancedConfig)) return;

  StaticJsonDocument<512> doc;
  deserializeJson(doc, (const char*)_advancedConfig);

  if (doc.containsKey(F("dmode")))    Log::setLogLevel(doc["dmode"]);
  if (doc.containsKey(F("flipOled"))) _advConf.flipOled = doc["flipOled"];
  if (doc.containsKey(F("dnOled")))   _advConf.dnOled   = doc["dnOled"];
  if (doc.containsKey(F("lowPower"))) _advConf.lowPower = doc["lowPower"];
}

// ============================================================
//  Parse modem startup
// ============================================================

void ConfigStore::parseModemStartup() {
  if (_modemStartup[0] == '\0') return;

  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, (const char*)_modemStartup);

  if (error.code() != DeserializationError::Ok || !doc.containsKey("mode")) {
    LOG_CONSOLE(PSTR("ERROR: Your modem config is invalid. Resetting to default"));
    _modemStartup[0] = '\0';
    save();
    return;
  }

  ModemInfo& m = status.modeminfo;
  const char* mode = doc["mode"].as<const char*>();
  strncpy(m.modem_mode, mode ? mode : "", sizeof(m.modem_mode) - 1);
  m.modem_mode[sizeof(m.modem_mode) - 1] = '\0';
  strcpy(m.satellite, doc["sat"].as<char*>());
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
    status.modeminfo.filter[i] = (i < filterSize) ? (uint8_t)doc["filter"][i] : 0;
  }
}

// ============================================================
//  Print config
// ============================================================

void ConfigStore::printConfig() {
  LOG_DEBUG(PSTR("MQTT Port: %u\nMQTT Server: %s\nMQTT User: %s\nLatitude: %f\nLongitude: %f"),
             getMqttPort(), getMqttServer(), getMqttUser(), getLatitude(), getLongitude());
  LOG_DEBUG(PSTR("tz: %s\nOLED Bright: %u\nTX %s"), getTZ(), getOledBright(), getAllowTx() ? "Enable" : "Disable");
  if (_boardTemplate[0] != '\0')
    LOG_DEBUG(PSTR("board_template: %s"), _boardTemplate);
  else
    LOG_DEBUG(PSTR("board: %u --> %s"), getBoard(), _boards[getBoard()].BOARD.c_str());
}
