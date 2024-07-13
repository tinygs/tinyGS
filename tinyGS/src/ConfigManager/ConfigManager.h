/*
  ConfigManager.h - Config Manager class
  
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
#ifndef ConfigManager_h
#define ConfigManager_h

#include "IotWebConf2.h"
#if IOTWEBCONF_DEBUG_DISABLED == 0 && !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /IotWebCong2/src/IotWebConf2Settings.h and add this line at the beggining of the file #define IOTWEBCONF_DEBUG_DISABLED 1"
#endif
#include "logos.h"
#include <Wire.h>
#include "html.h"

#ifdef ESP8266
#include "ESP8266HTTPUpdateServer.h"
#elif defined(ESP32)
#include "IotWebConf2ESP32HTTPUpdateServer.h"
#endif

constexpr auto STATION_NAME_LENGTH = 21;
constexpr auto COORDINATE_LENGTH = 10;
constexpr auto MQTT_SERVER_LENGTH = 31;
constexpr auto MQTT_PORT_LENGTH = 6;
constexpr auto MQTT_USER_LENGTH = 31;
constexpr auto MQTT_PASS_LENGTH = 31;
constexpr auto CHECKBOX_LENGTH = 9;
constexpr auto NUMBER_LEN = 32;
constexpr auto TEMPLATE_LEN = 256;
constexpr auto MODEM_LEN = 256;
constexpr auto ADVANCED_LEN = 256;
constexpr auto CB_SELECTED_STR = "selected";

constexpr auto ROOT_URL = "/";
constexpr auto CONFIG_URL = "/config";
constexpr auto DASHBOARD_URL = "/dashboard";
constexpr auto UPDATE_URL = "/firmware";
constexpr auto RESTART_URL = "/restart";
constexpr auto REFRESH_CONSOLE_URL = "/cs";
constexpr auto REFRESH_WORLDMAP_URL = "/wm";

const char TITLE_TEXT[] PROGMEM = "TinyGS Configuration";

constexpr auto thingName = "My TinyGS";
constexpr auto initialApPassword = "";
constexpr auto configVersion = "0.05"; //max 4 chars

#define MQTT_DEFAULT_SERVER "mqtt.tinygs.com"
#define MQTT_DEFAULT_PORT "8883"

constexpr auto AP_TIMEOUT_MS = "300000";

enum boardNum
{
 #if CONFIG_IDF_TARGET_ESP32S3
  HELTEC_LORA32_V3 = 0,
  ESP32S3_SX1278_LF,
 #elif CONFIG_IDF_TARGET_ESP32C3
  HELTEC_LORA32_HTCT62 = 0,
  ESP32C3_SX1278_LF,
 #else
  HELTEC_V1_LF = 0,
  HELTEC_V1_HF,
  HELTEC_V2_LF,
  HELTEC_V2_HF,
  TTGO_V1_LF,
  TTGO_V1_HF,
  TTGO_V2_LF,
  TTGO_V2_HF,
  TBEAM_OLED_LF,
  TBEAM_OLED_HF,
  ESP32_SX126X_XTAL,
  TTGO_V2_SX126X_XTAL,
  ESP32_SX126X_TXC0_1,
  ESP32_SX126X_TXC0_2,
  TBEAM_OLED_v1_0,
  ESP32_SX126X_TXC0_1W_LF,
  ESP32_SX126X_TXC0_1W_HF,
  ESP32_SX1280_1,
  TBEAM_OLED_v1_0_HF,
  LILYGO_T3_V1_6_1_LF,
  LILYGO_T3_V1_6_1_HF,
  LILYGO_T3_V1_6_1_HF_TCXO,
#endif
  NUM_BOARDS //this line always has to be the last one
};

typedef struct
{
  uint8_t OLED__address;
  uint8_t OLED__SDA;
  uint8_t OLED__SCL;
  uint8_t OLED__RST;
  uint8_t PROG__BUTTON;
  uint8_t BOARD_LED;
  uint8_t L_radio; // 0 SX1262  1 SX1278
  uint8_t L_NSS;    // CS
  uint8_t L_DI00;
  uint8_t L_DI01;
  uint8_t L_BUSSY;
  uint8_t L_RST;
  uint8_t L_MISO;
  uint8_t L_MOSI;
  uint8_t L_SCK;
  float L_TCXO_V;
  uint8_t RX_EN;
  uint8_t TX_EN;
  String BOARD;
} board_t;

const uint8_t UNUSED = -1;

typedef struct
{
  bool flipOled = true;
  bool dnOled = true;
  bool lowPower = false;
} AdvancedConfig;

class ConfigManager : public IotWebConf2
{
public:
  static ConfigManager &getInstance()
  {
    static ConfigManager instance;
    return instance;
  }
  void resetAPConfig();
  void resetAllConfig();
  void resetModemConfig();
  boolean init();
  void printConfig();

  uint16_t getMqttPort() { return (uint16_t)atoi(mqttPort); }
  const char *getMqttServer() { return mqttServer; }
  const char *getMqttUser() { return mqttUser; }
  const char *getMqttPass() { return mqttPass; }
  float getLatitude() { return atof(latitude); }
  float getLongitude() { return atof(longitude); }
  const char *getTZ() { return tz + 3; } // +3 removes the first 3 digits used for time zone deduplication
  uint8_t getBoard() { return atoi(board); }
  uint8_t getOledBright() { return atoi(oledBright); }
  bool getAllowTx() { return !strcmp(allowTx, CB_SELECTED_STR); }
  void setAllowTx(bool status)
  {
    if (status)
      strcpy(allowTx, CB_SELECTED_STR);
    else
      allowTx[0] = '\0';
    this->saveConfig();
  }
  const char *getModemStartup() { return modemStartup; }
  void setModemStartup(const char *modemStr)
  {
    strcpy(modemStartup, modemStr);
    this->saveConfig();
    parseModemStartup();
  }
  const char *getAvancedConfig() { return advancedConfig; }
  void setAvancedConfig(const char *adv_prmStr)
  {
    strcpy(advancedConfig, adv_prmStr);
    this->saveConfig();
  }
  const char *getBoardTemplate() { return boardTemplate; }
  void setBoardTemplate(const char *boardTemplateStr)
  {
    strcpy(boardTemplate, boardTemplateStr);
    this->saveConfig();
  }

  const char *getWiFiSSID() { return getWifiSsidParameter()->valueBuffer; }
  bool isConnected() { return getState() == IOTWEBCONF_STATE_ONLINE; };
  bool isApMode() { return (getState() != IOTWEBCONF_STATE_CONNECTING && getState() != IOTWEBCONF_STATE_ONLINE); }
  bool getFlipOled() { return advancedConf.flipOled; }
  bool getDayNightOled() { return advancedConf.dnOled; }
  bool getLowPower() { return advancedConf.lowPower; }
  bool getBoardConfig(board_t &board)
  {
    bool ret = true;
    if (!currentBoardDirty) { board = currentBoard; return ret; }

    if (getBoardTemplate()[0] == '\0') { currentBoard = boards[getBoard()]; }
    else { ret = parseBoardTemplate(currentBoard); }
    currentBoardDirty = false;
    board = currentBoard;
    return ret;
  }
  void saveConfig()
  {
    remoteSave = true;
    IotWebConf2::saveConfig();
    currentBoardDirty = true;
  };

private:
  class GSConfigHtmlFormatProvider : public iotwebconf2::HtmlFormatProvider
  {
  public:
    GSConfigHtmlFormatProvider(ConfigManager &x) : configManager(x) {}

  protected:
    String getScriptInner() override
    {
      return String(FPSTR(ADVANCED_CONFIG_SCRIPT)) +
             iotwebconf2::HtmlFormatProvider::getScriptInner();
    }
    String getBodyInner() override
    {
      return String(FPSTR(LOGO)) +
             iotwebconf2::HtmlFormatProvider::getBodyInner();
    }

    String getStyleInner() override
    {
      return iotwebconf2::HtmlFormatProvider::getStyleInner() +
             String(IOTWEBCONF_CONFIG_STYLE_INNER);
    }

    ConfigManager &configManager;
  };

  ConfigManager();
  void handleRadioBegin();
  void handleGetAllPackets();
  void handleRoot();
  void handleDashboard();
  void handleRefreshConsole();
  void handleRefreshWorldmap();
  void handleBoardTemplateRequest();
  void handleRestart();
  bool formValidator(iotwebconf2::WebRequestWrapper *);
  void boardDetection();
  void configSavedCallback();
  void parseAdvancedConf();
  void parseModemStartup();
  bool parseBoardTemplate(board_t &);

  std::function<boolean(iotwebconf2::WebRequestWrapper *)> formValidatorStd;
  DNSServer dnsServer;
  WebServer server;
#ifdef ESP8266
  ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
  HTTPUpdateServer httpUpdater;
#endif
  GSConfigHtmlFormatProvider gsConfigHtmlFormatProvider;
  board_t boards[NUM_BOARDS];
  board_t currentBoard;
  bool currentBoardDirty = true;
  AdvancedConfig advancedConf;
  char savedThingName[IOTWEBCONF_WORD_LEN] = "";
  bool remoteSave = false;

  char latitude[COORDINATE_LENGTH] = "";
  char longitude[COORDINATE_LENGTH] = "";
  char tz[TZ_LENGTH] = "";
  char mqttServer[MQTT_SERVER_LENGTH] = MQTT_DEFAULT_SERVER;
  char mqttPort[MQTT_PORT_LENGTH] = MQTT_DEFAULT_PORT;
  char mqttUser[MQTT_USER_LENGTH] = "";
  char mqttPass[MQTT_PASS_LENGTH] = "";
  char board[BOARD_LENGTH] = "";
  char oledBright[NUMBER_LEN] = "";
  char allowTx[CHECKBOX_LENGTH] = "";
  char remoteTune[CHECKBOX_LENGTH] = "";
  char telemetry3rd[CHECKBOX_LENGTH] = "";
  char testMode[CHECKBOX_LENGTH] = "";
  char autoUpdate[CHECKBOX_LENGTH] = "";
  char boardTemplate[TEMPLATE_LEN] = "";
  char modemStartup[MODEM_LEN] = "";
  char advancedConfig[ADVANCED_LEN] = "";

  iotwebconf2::NumberParameter latitudeParam = iotwebconf2::NumberParameter("Latitude (3 decimals, will be public)", "lat", latitude, COORDINATE_LENGTH, NULL, "0.000", "required min='-180' max='180' step='0.001'");
  iotwebconf2::NumberParameter longitudeParam = iotwebconf2::NumberParameter("Longitude (3 decimals, will be public)", "lng", longitude, COORDINATE_LENGTH, NULL, "-0.000", "required min='-180' max='180' step='0.001'");
  iotwebconf2::SelectParameter tzParam = iotwebconf2::SelectParameter("Time Zone", "tz", tz, TZ_LENGTH, (char *)TZ_VALUES, (char *)TZ_NAMES, sizeof(TZ_VALUES) / TZ_LENGTH, TZ_NAME_LENGTH);

  iotwebconf2::ParameterGroup groupMqtt = iotwebconf2::ParameterGroup("MQTT credentials", "MQTT credentials (First join the group <a href='https://t.me/joinchat/DmYSElZahiJGwHX6jCzB3Q'>here</a>)<br>Then open a private chat with <a href='https://t.me/tinygs_personal_bot'>@tinygs_personal_bot</a> and ask /mqtt");
  iotwebconf2::TextParameter mqttServerParam = iotwebconf2::TextParameter("Server address", "mqtt_server", mqttServer, MQTT_SERVER_LENGTH, MQTT_DEFAULT_SERVER, MQTT_DEFAULT_SERVER, "required type=\"text\" maxlength=30");
  iotwebconf2::NumberParameter mqttPortParam = iotwebconf2::NumberParameter("Server Port", "mqtt_port", mqttPort, MQTT_PORT_LENGTH, MQTT_DEFAULT_PORT, MQTT_DEFAULT_PORT, "required min=\"0\" max=\"65536\" step=\"1\"");
  iotwebconf2::TextParameter mqttUserParam = iotwebconf2::TextParameter("MQTT Username", "mqtt_user", mqttUser, MQTT_USER_LENGTH, NULL, NULL, "required type=\"text\" maxlength=30");
  iotwebconf2::TextParameter mqttPassParam = iotwebconf2::TextParameter("MQTT Password", "mqtt_pass", mqttPass, MQTT_PASS_LENGTH, NULL, NULL, "required type=\"text\" maxlength=30");

  iotwebconf2::ParameterGroup groupBoardConfig = iotwebconf2::ParameterGroup("Board config", "Board config");
  iotwebconf2::SelectParameter boardParam = iotwebconf2::SelectParameter("Board type", "board", board, BOARD_LENGTH, (char *)BOARD_VALUES, (char *)BOARD_NAMES, sizeof(BOARD_VALUES) / BOARD_LENGTH, BOARD_NAME_LENGTH);
  iotwebconf2::NumberParameter oledBrightParam = iotwebconf2::NumberParameter("OLED Bright", "oled_bright", oledBright, NUMBER_LEN, "100", "0..100", "min='0' max='100' step='1'");
  iotwebconf2::CheckboxParameter AllowTxParam = iotwebconf2::CheckboxParameter("Enable TX (HAM licence/ no preamp)", "tx", allowTx, CHECKBOX_LENGTH, true);
  iotwebconf2::CheckboxParameter remoteTuneParam = iotwebconf2::CheckboxParameter("Allow Automatic Tuning", "remote_tune", remoteTune, CHECKBOX_LENGTH, true);
  iotwebconf2::CheckboxParameter telemetry3rdParam = iotwebconf2::CheckboxParameter("Allow sending telemetry to third party", "telemetry3rd", telemetry3rd, CHECKBOX_LENGTH, true);
  iotwebconf2::CheckboxParameter testParam = iotwebconf2::CheckboxParameter("Test mode", "test", testMode, CHECKBOX_LENGTH, false);
  iotwebconf2::CheckboxParameter autoUpdateParam = iotwebconf2::CheckboxParameter("Automatic Firmware Update", "auto_update", autoUpdate, CHECKBOX_LENGTH, true);

  iotwebconf2::ParameterGroup groupAdvanced = iotwebconf2::ParameterGroup("Advanced config", "Advanced Config (do not modify unless you know what you are doing)");
  iotwebconf2::TextParameter boardTemplateParam = iotwebconf2::TextParameter("Board Template (requires manual restart)", "board_template", boardTemplate, TEMPLATE_LEN, NULL, NULL, "type=\"text\" maxlength=255");
  iotwebconf2::TextParameter modemParam = iotwebconf2::TextParameter("Modem startup", "modem_startup", modemStartup, MODEM_LEN, "", "", "type=\"text\" maxlength=255");
  iotwebconf2::TextParameter advancedConfigParam = iotwebconf2::TextParameter("Advanced parameters", "advanced_config", advancedConfig, ADVANCED_LEN, NULL, NULL, "type=\"text\" maxlength=255");
};

#endif
