/*
  ConfigStore.h - NVS-based configuration storage

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

#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <Preferences.h>
#include <Arduino.h>
#include "INetworkAware.h"
#include "ArduinoJson.h"
#include "../Status.h"
#include "html.h"

extern Status status;

// ---- Station defaults ----
static const char* MQTT_SERVER_HOST = "mqtt.tinygs.com";
static const char* MQTT_SERVER_PORT = "8883";

constexpr auto STATION_NAME_LENGTH = 21;
constexpr auto COORDINATE_LENGTH = 10;
constexpr auto MQTT_SERVER_LENGTH = 31;
constexpr auto MQTT_PORT_LENGTH = 6;
constexpr auto MQTT_USER_LENGTH = 31;
constexpr auto MQTT_PASS_LENGTH = 31;
constexpr auto TEMPLATE_LEN = 768;
constexpr auto MODEM_LEN = 256;
constexpr auto ADVANCED_LEN = 256;
constexpr auto AP_PASSWORD_LENGTH = 32;
constexpr auto WIFI_SSID_LENGTH = 33;
constexpr auto WIFI_PASS_LENGTH = 65;

#define MQTT_DEFAULT_SERVER "mqtt.tinygs.com"
#define MQTT_DEFAULT_PORT   "8883"

constexpr auto thingName = "My TinyGS";
constexpr auto initialApPassword = "";
constexpr auto configVersion = "0.06"; // bumped for NVS migration

// ---- Board enum ----
enum RadioModelNum_t {
  RADIO_SX1278 = 1,
  RADIO_SX1276 = 2,
  RADIO_SX1268 = 5,
  RADIO_SX1262 = 6,
  RADIO_SX1280 = 8
};

enum boardNum {
#if CONFIG_IDF_TARGET_ESP32S3
  HELTEC_LORA32_V3 = 0,
  ESP32S3_SX1278_LF,
  TTGO_TBEAM_SX1262,
  LILYGO_T3S3_SX1280,
  WAVESHARE_ESP32S3_ETH,
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
  TBEAM_SX1268_TCXO,
  WT32_ETH01,
#endif
  NUM_BOARDS
};

const uint8_t UNUSED_PIN = 255;
const uint8_t UNUSED = UNUSED_PIN;  // backward compatibility alias

struct board_t {
  uint8_t OLED__address;
  uint8_t OLED__SDA;
  uint8_t OLED__SCL;
  uint8_t OLED__RST;
  uint8_t PROG__BUTTON;
  uint8_t BOARD_LED;
  uint8_t L_radio;
  uint8_t L_NSS;
  uint8_t L_DI00;
  uint8_t L_DI01;
  uint8_t L_BUSSY;
  uint8_t L_RST;
  uint8_t L_MISO;
  uint8_t L_MOSI;
  uint8_t L_SCK;
  float   L_TCXO_V;
  uint8_t RX_EN;
  uint8_t TX_EN;
  String  BOARD;

#if CONFIG_IDF_TARGET_ESP32S3
  uint8_t L_SPI   = 2;        // SPI bus for radio: 2=SPI2(HSPI) — S3 default
#else
  uint8_t L_SPI   = 3;        // SPI bus for radio: 3=SPI3(VSPI) — Classic ESP32 default (matches beta)
#endif

  // Ethernet fields (from board template JSON)
  bool    ethEN   = false;
  uint8_t ethPHY  = 0;       // 0=W5500, 1=DM9051, 2=KSZ8851SNL (SPI); 0xFF=InternalEmac
  uint8_t ethSPI  = 2;       // SPI bus for ethernet: 2=SPI2, 3=SPI3
  uint8_t ethCS   = UNUSED_PIN;
  uint8_t ethINT  = UNUSED_PIN;
  uint8_t ethRST  = UNUSED_PIN;
  uint8_t ethMISO = UNUSED_PIN;
  uint8_t ethMOSI = UNUSED_PIN;
  uint8_t ethSCK  = UNUSED_PIN;
  // Internal EMAC fields (ESP32 classic RMII) — only used when ethPHY=0xFF
  uint8_t ethMDC     = UNUSED_PIN;  // MDC pin  (typically GPIO23)
  uint8_t ethMDIO    = UNUSED_PIN;  // MDIO pin (typically GPIO18)
  int8_t  ethPhyAddr = 1;           // SMI PHY address (1=typical for LAN8720)
  uint8_t ethPhyType = 0;           // 0=LAN8720,1=IP101,2=RTL8201,3=DP83848,4=KSZ8041,5=KSZ8081
  uint8_t ethRefClk  = 0;           // REF_CLK pin (typically GPIO0)
  bool    ethClkExt  = true;        // true = PHY drives REF_CLK (ext input), false = ESP32 outputs it
  uint8_t ethPhyRST  = UNUSED_PIN;  // PHY active-low hardware reset pin (library pulses it LOW-HIGH during phy->init)
  uint8_t ethOscEN   = UNUSED_PIN;  // Oscillator enable pin (active-high, e.g. ESP32-ETH01 GPIO16) — driven HIGH before EMAC init, never touched by library

  board_t() = default;
  board_t(uint8_t oled_addr, uint8_t oled_sda, uint8_t oled_scl, uint8_t oled_rst,
          uint8_t prog_btn, uint8_t board_led,
          uint8_t l_radio, uint8_t l_nss, uint8_t l_di00, uint8_t l_di01,
          uint8_t l_bussy, uint8_t l_rst, uint8_t l_miso, uint8_t l_mosi, uint8_t l_sck,
          float l_tcxo_v, uint8_t rx_en, uint8_t tx_en, String board_name)
    : OLED__address(oled_addr), OLED__SDA(oled_sda), OLED__SCL(oled_scl), OLED__RST(oled_rst),
      PROG__BUTTON(prog_btn), BOARD_LED(board_led),
      L_radio(l_radio), L_NSS(l_nss), L_DI00(l_di00), L_DI01(l_di01),
      L_BUSSY(l_bussy), L_RST(l_rst), L_MISO(l_miso), L_MOSI(l_mosi), L_SCK(l_sck),
      L_TCXO_V(l_tcxo_v), RX_EN(rx_en), TX_EN(tx_en), BOARD(board_name) {}
};

typedef struct {
  bool flipOled  = true;
  bool dnOled    = true;
  bool lowPower  = false;
} AdvancedConfig;

// Board name/length constants are defined in html.h (which is included above)

class ConfigStore {
public:
  static ConfigStore& getInstance() {
    static ConfigStore instance;
    return instance;
  }

  bool init();
  void save();
  void printConfig();
  void resetAPConfig();
  void resetAllConfig();
  void resetModemConfig();
  bool migrateFromEEPROM(); // migration from IotWebConf2 EEPROM format

  // ---- Station identity ----
  const char* getThingName()         const { return _stationName; }
  void        setThingName(const char* n);
  const char* getApPassword()        const { return _apPassword; }
  void        setApPassword(const char* p);

  // ---- WiFi ----
  const char* getWifiSSID()          const { return _wifiSSID; }
  void        setWifiSSID(const char* s);
  const char* getWifiPassword()      const { return _wifiPass; }
  void        setWifiPassword(const char* p);

  // ---- Location ----
  float       getLatitude()          const { return atof(_latitude); }
  float       getLongitude()         const { return atof(_longitude); }
  const char* getLatitudeStr()       const { return _latitude; }
  const char* getLongitudeStr()      const { return _longitude; }
  void        setLat(const char* v);
  void        setLon(const char* v);

  // ---- Timezone ----
  const char* getTZ()                const { return _tz[0] ? _tz + 3 : ""; } // skip 3-digit prefix
  const char* getTZRaw()             const { return _tz; }
  void        setTZ(const char* v);

  // ---- MQTT ----
  uint16_t    getMqttPort()          const { return (uint16_t)atoi(_mqttPort); }
  const char* getMqttServer()        const { return _mqttServer; }
  const char* getMqttUser()          const { return _mqttUser; }
  const char* getMqttPass()          const { return _mqttPass; }
  void        setMqttServer(const char* v);
  void        setMqttPort(const char* v);
  void        setMqttUser(const char* v);
  void        setMqttPass(const char* v);

  // ---- Board ----
  uint8_t     getBoard()             const { return (uint8_t)atoi(_board); }
  void        setBoard(const char* v);
  uint8_t     getOledBright()        const { return (uint8_t)atoi(_oledBright); }
  void        setOledBright(const char* v);
  bool        getAllowTx()           const { return !strcmp(_allowTx, "selected"); }
  void        setAllowTx(bool v);
  bool        getRemoteTune()        const { return !strcmp(_remoteTune, "selected"); }
  bool        getTelemetry3rd()      const { return !strcmp(_telemetry3rd, "selected"); }
  bool        getTestMode()          const { return !strcmp(_testMode, "selected"); }
  bool        getAutoUpdate()        const { return !strcmp(_autoUpdate, "selected"); }

  // ---- Advanced ----
  const char* getModemStartup()      const { return _modemStartup; }
  void        setModemStartup(const char* v);
  const char* getAdvancedConfig()    const { return _advancedConfig; }
  void        setAdvancedConfig(const char* v);
  const char* getBoardTemplate()     const { return _boardTemplate; }
  void        setBoardTemplate(const char* v);

  // ---- Derived config ----
  bool        getFlipOled()          const { return _advConf.flipOled; }
  bool        getDayNightOled()      const { return _advConf.dnOled; }
  bool        getLowPower()          const { return _advConf.lowPower; }
  bool        getDisableOled()       const { return !strcmp(_disableOled, "selected"); }
  bool        getDisableRadio()      const { return !strcmp(_disableRadio, "selected"); }
  bool        getAlwaysAP()          const { return !strcmp(_alwaysAP, "selected"); }
  void        setDisableOled(bool v);
  void        setDisableRadio(bool v);
  void        setAlwaysAP(bool v);

  // ---- Network mode ----
  InterfaceMode getInterfaceMode()   const { return _ifaceMode; }
  void          setInterfaceMode(InterfaceMode m);

  // ---- Board list (drives web UI dropdown) ----
  uint8_t     getBoardCount()           const { return NUM_BOARDS; }
  const char* getBoardName(uint8_t idx) const { return (idx < NUM_BOARDS) ? _boards[idx].BOARD.c_str() : ""; }

  // ---- Board config resolution ----
  bool getBoardConfig(board_t& board);
  // Returns true only when boardTemplate is non-empty AND differs from the
  // default config of the currently selected board index.
  bool isBoardTemplateModified() const;
  // Returns true only when boardTemplate is non-empty AND differs from the
  // default config of the currently selected board index.
  bool isBoardTemplateModified() const;

  // ---- Raw buffer access (for Improv/OTP) ----
  char* stationNameBuffer() { return _stationName; }
  char* apPasswordBuffer()  { return _apPassword; }
  char* wifiSSIDBuffer()    { return _wifiSSID; }
  char* wifiPassBuffer()    { return _wifiPass; }
  char* mqttServerBuffer()  { return _mqttServer; }
  char* mqttPortBuffer()    { return _mqttPort; }
  char* mqttUserBuffer()    { return _mqttUser; }
  char* mqttPassBuffer()    { return _mqttPass; }
  char* latitudeBuffer()    { return _latitude; }
  char* longitudeBuffer()   { return _longitude; }
  char* tzBuffer()          { return _tz; }

  // Buffer lengths for Improv
  size_t stationNameLen()  const { return STATION_NAME_LENGTH; }
  size_t apPasswordLen()   const { return AP_PASSWORD_LENGTH; }
  size_t wifiSSIDLen()     const { return WIFI_SSID_LENGTH; }
  size_t wifiPassLen()     const { return WIFI_PASS_LENGTH; }
  size_t mqttServerLen()   const { return MQTT_SERVER_LENGTH; }
  size_t mqttPortLen()     const { return MQTT_PORT_LENGTH; }
  size_t mqttUserLen()     const { return MQTT_USER_LENGTH; }
  size_t mqttPassLen()     const { return MQTT_PASS_LENGTH; }
  size_t latitudeLen()     const { return COORDINATE_LENGTH; }
  size_t longitudeLen()    const { return COORDINATE_LENGTH; }
  size_t tzLen()           const { return sizeof(_tz); }

  // ---- Fail-safe ----
  bool isFailSafeActive()  const { return _failSafe; }
  void setFailSafe(bool v)       { _failSafe = v; }

private:
  ConfigStore();
  Preferences _prefs;

  // ---- Buffers ----
  char _stationName[STATION_NAME_LENGTH]  = "";
  char _apPassword[AP_PASSWORD_LENGTH]    = "";
  char _wifiSSID[WIFI_SSID_LENGTH]        = "";
  char _wifiPass[WIFI_PASS_LENGTH]        = "";
  char _latitude[COORDINATE_LENGTH]       = "";
  char _longitude[COORDINATE_LENGTH]      = "";
  char _tz[49]                            = "";
  char _mqttServer[MQTT_SERVER_LENGTH]    = "";
  char _mqttPort[MQTT_PORT_LENGTH]        = "";
  char _mqttUser[MQTT_USER_LENGTH]        = "";
  char _mqttPass[MQTT_PASS_LENGTH]        = "";
  char _board[BOARD_LENGTH]               = "";
  char _oledBright[8]                     = "100";
  char _allowTx[12]                       = "";
  char _remoteTune[12]                    = "selected";
  char _telemetry3rd[12]                  = "selected";
  char _testMode[12]                      = "";
  char _autoUpdate[12]                    = "selected";
  char _disableOled[12]                   = "";
  char _disableRadio[12]                  = "";
  char _alwaysAP[12]                      = "";
  char _boardTemplate[TEMPLATE_LEN]       = "";
  char _modemStartup[MODEM_LEN]           = "";
  char _advancedConfig[ADVANCED_LEN]      = "";

  InterfaceMode _ifaceMode = InterfaceMode::WIFI_ONLY;
  AdvancedConfig _advConf;
  bool _failSafe = false;

  // Board tables
  board_t _boards[NUM_BOARDS];
  board_t _currentBoard;
  bool    _currentBoardDirty = true;

  // Internal
  void initBoardTable();
  void parseAdvancedConf();
  void parseModemStartup();
  bool parseBoardTemplate(board_t& board);
  void boardDetection();
  void loadFromNVS();
  void saveToNVS();
};

#endif // CONFIGSTORE_H
