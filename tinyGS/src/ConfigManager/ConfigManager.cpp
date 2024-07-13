/*
  ConfigManager.cpp - Config Manager class

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

#include "ConfigManager.h"
#include "../Mqtt/MQTT_Client.h"
#include "../Logger/Logger.h"
#include "../Radio/Radio.h"
#include "../Display/graphics.h"
#include "ArduinoJson.h"
#if ARDUINOJSON_USE_LONG_LONG == 0 && !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /ArduinoJson/src/ArduinoJson/Configuration.hpp and amend to #define ARDUINOJSON_USE_LONG_LONG 1 around line 68"
#endif
/*

id_   Part   Freq. Range L.Bud(dB) RXCurrent(mA) FSK Max DR(kbps) LoRa DR (kbps) Max Sensitivity(dBm) TX Power(dBm)
1    SX1278    137–525     168       11              300          0.018–40           -148              +20
2    SX1276    137–1020    168       11              300         0.018–40            -148              +20

5    SX1268    410–810     170       4.6             300         0.018–62.5          -148              +22
6    SX1262    150–960     170       4.6             300         0.018–62.5          -148              +22

8    SX1280    2.4–2.5Ghz  130       5.5            2000         0.476-202           -132              +12.5

na   SX1272    862–1020    158       10              300           0.3–40            -138              +20
na   SX1273    862–1020    150       10              300           1.7–40            -130              +20
na   SX1277    137–1020    158       11              300           1.7–40            -138              +20
na   SX1279    137–960     168       11              300          0.018–40           -148              +20
na   SX1261    150–960     163       4.6             300         0.018–62.5          -148              +15
na   SX1281    2.4–2.5Ghz  130       5.5            2000         0.476-202           -132              +12.5


*/

ConfigManager::ConfigManager()
    : IotWebConf2(thingName, &dnsServer, &server, initialApPassword, configVersion), server(80), gsConfigHtmlFormatProvider(*this), boards({
  //OLED_add, OLED_SDA,  OLED_SCL, OLED_RST, PROG_BUTTON, BOARD_LED,      L_SX127X?,   L_NSS, L_DI00, L_DI01, L_BUSSY, L_RST,  L_MISO, L_MOSI, L_SCK, L_TCXO_V, RX_EN, TX_EN,   BOARD
#if CONFIG_IDF_TARGET_ESP32S3
  {      0x3c,       17,        18,       21,           0,        35,      RADIO_SX1262,    8,   UNUSED,   14,      13,   12,      11,     10,     9,     1.6f,   UNUSED, UNUSED, "150–960Mhz - HELTEC LORA32 V3 SX1262"    },  // SX1262
  {      0x3c,       17,        18,     UNUSED,         0,        35,      RADIO_SX1278,    8,      6,     14,   UNUSED,  12,      11,     10,     9,     0.0f,   UNUSED, UNUSED, "Custom ESP32-S3 433MHz SX1278"     },  // SX1278 @g4lile0
#elif CONFIG_IDF_TARGET_ESP32C3
  {      0x3c,        0,        1,       UNUSED,        20,       21,      RADIO_SX1262,    8,   UNUSED,    3,      4,     5,       6,      7,    10,     1.6f,    UNUSED, UNUSED, "433MHz HELTEC LORA32 HT-CT62 SX1262" },  // SX1262  @gargomoma
  {      0x3c,        0,        1,       UNUSED,        20,       21,      RADIO_SX1278,    8,     4,   UNUSED,  UNUSED,   5,       6,      7,    10,     0.0f,    UNUSED, UNUSED, "Custom ESP32-C3 433MHz SX1278"     },  // SX1278 @gargomoma
#else
  {      0x3c,        4,        15,       16,           0,        25,      RADIO_SX1278,    18,     26,     12,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz HELTEC WiFi LoRA 32 V1" },      // SX1278 @4m1g0
  {      0x3c,        4,        15,       16,           0,        25,      RADIO_SX1276,    18,     26,     12,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "863-928MHz HELTEC WiFi LoRA 32 V1" },  // SX1276
  {      0x3c,        4,        15,       16,           0,        25,      RADIO_SX1278,    18,     26,     35,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz HELTEC WiFi LoRA 32 V2" },      // SX1278 @4m1g0  
  {      0x3c,        4,        15,       16,           0,        25,      RADIO_SX1276,    18,     26,     35,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "863-928MHz HELTEC WiFi LoRA 32 V2" },  // SX1276
  {      0x3c,        4,        15,       16,           0,         2,      RADIO_SX1278,    18,     26,   UNUSED, UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433Mhz  TTGO LoRa 32 v1"        },     // SX1278 @g4lile0 
  {      0x3c,        4,        15,       16,           0,         2,      RADIO_SX1276,    18,     26,   UNUSED, UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz TTGO LoRa 32 v1"        },  // SX1276
  {      0x3c,       21,        22,     UNUSED,         0,        22,      RADIO_SX1278,    18,     26,     33,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz TTGO LoRA 32 v2"        },      // SX1278  @TCRobotics
  {      0x3c,       21,        22,       16,           0,        22,      RADIO_SX1276,    18,     26,     33,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz TTGO LoRA 32 v2"        },  // SX1276
  {      0x3c,       21,        22,       16,          39,        22,      RADIO_SX1278,    18,     26,     33,     32,    14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz T-BEAM + OLED"        },        // SX1278
  {      0x3c,       21,        22,       16,          39,        22,      RADIO_SX1276,    18,     26,     33,     32,    14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz T-BEAM + OLED"        },    // SX1276
  {      0x3c,       21,        22,       16,           0,        25,      RADIO_SX1268,     5,   UNUSED,   27,     26,    14,      19,     23,    18,     0.0f,   UNUSED, UNUSED, "Custom ESP32 Wroom + SX126x (Crystal)"  }, // SX1268 @4m1g0, @lillefyr
  {      0x3c,       21,        22,     UNUSED,         0,        25,      RADIO_SX1268,    18,   UNUSED,   33,     32,    14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "TTGO LoRa 32 V2 Modified with module SX126x (crystal)"  }, // SX1268 @TCRobotics
  {      0x3c,       21,        22,       16,           0,        25,      RADIO_SX1268,     5,   UNUSED,    2,     13,    26,      19,     23,    18,     1.6f,   UNUSED, UNUSED, "Custom ESP32 Wroom + SX126x DRF1268T (TCX0) (5, 2, 26, 13)"  }, // SX1268 @sdey76
  {      0x3c,       21,        22,       16,           0,        25,      RADIO_SX1268,     5,   UNUSED,   26,     12,    14,      19,     23,    18,     1.6f,   UNUSED, UNUSED, "Custom ESP32 Wroom + SX126x DRF1268T (TCX0) (5, 26, 14, 12)"  }, // SX1268 @imants
  {      0x3c,       21,        22,     UNUSED,        38,        22,      RADIO_SX1278,    18,     26,     33,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz T-BEAM V1.0 + OLED"     },       // SX1278 @fafu
  {      0x3c,       21,        22,       16,           0,         2,      RADIO_SX1268,     5,   UNUSED,   34,     32,    14,      19,     27,    18,     1.6f,   UNUSED, UNUSED, "433MHz FOSSA 1W Ground Station"  },     // SX1268 @jgromes
  {      0x3c,       21,        22,       16,           0,         2,      RADIO_SX1276,     5,   UNUSED,   34,     32,    14,      19,     27,    18,     1.6f,   UNUSED, UNUSED, "868-915MHz FOSSA 1W Ground Station"  }, //SX1276 @jgromes
  {      0x3c,       21,        22,     UNUSED,         0,        22,      RADIO_SX1280,     5,     26,     34,     32,    14,      19,     27,    18,     0.0f,   UNUSED, UNUSED, "2.4GHz ESP32 + SX1280"  },              //SX1280 @g4lile0
  {      0x3c,       21,        22,     UNUSED,        38,        22,      RADIO_SX1276,    18,     26,     33,   UNUSED,  14,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz T-BEAM V1.0 + OLED"     },   // SX1276 @fafu
  {      0x3c,       21,        22,     UNUSED,         0,        25,      RADIO_SX1278,    18,     26,     33,   UNUSED,  23,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "433MHz LILYGO T3_V1.6.1"     },         // SX1278
  {      0x3c,       21,        22,     UNUSED,         0,        25,      RADIO_SX1276,    18,     26,     33,   UNUSED,  23,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz LILYGO T3_V1.6.1"     },     // SX1276
  {      0x3c,       21,        22,     UNUSED,         0,        25,      RADIO_SX1276,    18,     26,   UNUSED,   32,    23,      19,     27,     5,     0.0f,   UNUSED, UNUSED, "868-915MHz LILYGO T3_V1.6.1 TCXO"    }, // SX1262

 #endif
  })
{
  server.on("/radioBegin", [this] { handleRadioBegin(); });
  server.on("/packets", [this] { handleGetAllPackets(); });
  server.on(ROOT_URL, [this] { handleRoot(); });
  server.on(CONFIG_URL, [this] { handleConfig(); });
  server.on(DASHBOARD_URL, [this] { handleDashboard(); });
  server.on(RESTART_URL, [this] { handleRestart(); });
  server.on(REFRESH_CONSOLE_URL, [this] { handleRefreshConsole(); });
  server.on(REFRESH_WORLDMAP_URL, [this] { handleRefreshWorldmap(); });
  setupUpdateServer(
      [this](const char *updatePath) { httpUpdater.setup(&server, updatePath); },
      [this](const char *userName, char *password) { httpUpdater.updateCredentials(userName, password); });
  setHtmlFormatProvider(&gsConfigHtmlFormatProvider);
  formValidatorStd = std::bind(&ConfigManager::formValidator, this, std::placeholders::_1);
  setFormValidator(formValidatorStd);
  setConfigSavedCallback([this] { configSavedCallback(); });
  skipApStartup();

  // Customize own parameters
  getThingNameParameter()->label = "GroundStation Name (will be seen on the map)";
  getApPasswordParameter()->label = "Password for this dashboard (user is <b>admin</b>)";

  addSystemParameter(&latitudeParam);
  addSystemParameter(&longitudeParam);
  addSystemParameter(&tzParam);

  groupMqtt.addItem(&mqttServerParam);
  groupMqtt.addItem(&mqttPortParam);
  groupMqtt.addItem(&mqttUserParam);
  groupMqtt.addItem(&mqttPassParam);
  addParameterGroup(&groupMqtt);

  groupBoardConfig.addItem(&boardParam);
  groupBoardConfig.addItem(&oledBrightParam);
  groupBoardConfig.addItem(&AllowTxParam);
  groupBoardConfig.addItem(&remoteTuneParam);
  groupBoardConfig.addItem(&telemetry3rdParam);
  groupBoardConfig.addItem(&testParam);
  groupBoardConfig.addItem(&autoUpdateParam);
  addParameterGroup(&groupBoardConfig);

  groupAdvanced.addItem(&boardTemplateParam);
  groupAdvanced.addItem(&modemParam);
  groupAdvanced.addItem(&advancedConfigParam);
  addParameterGroup(&groupAdvanced);
}

void ConfigManager::handleRadioBegin()
{
  if (!Radio::getInstance().isReady())
  {
    server.send(200, "application/json; charset=UTF-8", "{\"message\":\"Radio is not ready to begin\"}");
    Log::console(PSTR("Radio is not ready to begin"));
    return;
  }
  if (Radio::getInstance().begin() == 0)
  {
    server.send(200, "application/json; charset=UTF-8", "{\"message\":\"Radio begun successfully after /radioBegin request\"}");
    Log::console(PSTR("Radio begun successfully after /radioBegin request"));
    return;
  }
  server.send(200, "application/json; charset=UTF-8", "{\"message\":\"An error occurred while trying to start the Radio\"}");
  Log::console(PSTR("An error occurred while trying to start the Radio"));
}

void ConfigManager::handleGetAllPackets()
{
  String string = String(FPSTR("{\"message\":\"Packets were fetched successfully\",\"packets\":["));
  for (size_t i = 0; i < status.allPackets.size(); i++)
  {
    string += "{\"time\":\"" + status.allPackets.at(i).time
      + "\",\"encoded_message\":\"" + status.allPackets.at(i).encoded_message
      + "\",\"decoded_message\":\"" + status.allPackets.at(i).decoded_message
      + "\",\"rssi\":" + status.allPackets.at(i).rssi
      + ",\"snr\":" + status.allPackets.at(i).snr
      + ",\"frequencyerror\":" + status.allPackets.at(i).frequencyerror
      + ",\"crc_error\":" + status.allPackets.at(i).crc_error
      + "}";
    if (i < status.allPackets.size() - 1)
    {
      string += ",";
    }
  }
  string += "]}";
  server.send(200, "application/json; charset=UTF-8", string);
  Log::console(PSTR("Endpoint /packets was requested and handled successfully"));
}

void ConfigManager::handleRoot()
{
  // -- Let IotWebConf2 test and handle captive portal requests.
  if (handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

  String s = String(FPSTR(IOTWEBCONF_HTML_HEAD));
  s += "<style>" + String(FPSTR(IOTWEBCONF_HTML_STYLE_INNER)) + "</style>";
  s += FPSTR(IOTWEBCONF_HTML_HEAD_END);
  s += FPSTR(IOTWEBCONF_HTML_BODY_INNER);
  s += String(FPSTR(LOGO)) + "<br />";
  s += "<button onclick=\"window.location.href='" + String(DASHBOARD_URL) + "';\">Station dashboard</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(CONFIG_URL) + "';\">Configure parameters</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(UPDATE_URL) + "';\">Upload new version</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(RESTART_URL) + "';\">Restart Station</button><br /><br />";
  s += FPSTR(IOTWEBCONF_HTML_END);

  s.replace("{v}", FPSTR(TITLE_TEXT));

  // server.sendHeader("Content-Length", String(s.length()));
  server.send(200, "text/html; charset=UTF-8", s);
}

void ConfigManager::handleDashboard()
{
  if (getState() == IOTWEBCONF_STATE_ONLINE)
  {
    // -- Authenticate
    if (!server.authenticate(IOTWEBCONF_ADMIN_USER_NAME, getApPasswordParameter()->valueBuffer))
    {
      IOTWEBCONF_DEBUG_LINE(F("Requesting authentication."));
      server.requestAuthentication();
      return;
    }
  }

  // uint64_t time = millis(); // TODO: add current time
  String s = String(FPSTR(IOTWEBCONF_HTML_HEAD));
  s += "<style>" + String(FPSTR(IOTWEBCONF_HTML_STYLE_INNER)) + "</style>";
  s += "<style>" + String(FPSTR(IOTWEBCONF_DASHBOARD_STYLE_INNER)) + "</style>";
  s += "<script>" + String(FPSTR(IOTWEBCONF_CONSOLE_SCRIPT)) + "</script>";
  s += "<script>" + String(FPSTR(IOTWEBCONF_WORLDMAP_SCRIPT)) + "</script>";
  s += FPSTR(IOTWEBCONF_HTML_HEAD_END);
  s += FPSTR(IOTWEBCONF_DASHBOARD_BODY_INNER);
  s += String(FPSTR(LOGO)) + "<br />";

  // build svg of world map with animated satellite position
  uint ix = 0;
  uint sx;
  String svg = "<div style=""margin-left:35px""><svg width""100%"" height=""auto"" viewBox=""0 0 262 134"" xmlns=""http://www.w3.org/2000/svg"">";
  svg += "<rect x=""1"" y=""1"" width=""262"" height=""134"" stroke=""gray"" fill=""none"" stroke-width=""2"" />";
  for (uint y = 0; y < earth_height; y++)
  {
    uint n = 0;
    for (uint x = 0; x < earth_width / 8; x++)
    {
      for (uint i = 0; i < 8; i++)
      {
        if ((earth_bits[ix] >> i) & 1)
        {
          if (n == 0)
          {
            sx = (x * 8) + i;
          }
          n++;
        }
        if (!((earth_bits[ix] >> i) & 1) || ((x == earth_width / 8 - 1) && (i == 7)))
        {
          if (n > 0)
          {
            // append current land pixel string
            svg += "<rect x="""+ String(sx * 2 + 3) + """ y=""" + String(y * 2 + 3) + """ width=""" + String(n * 2) + """ height=""2"" />";
            n = 0;
          }
        }
      }
      ix++;
    }
  }
  // add animated satellite position
  svg += "<circle id=""wmsatpos"" cx=""" + String(status.satPos[0] * 2 + 3) + """ cy=""" + String(status.satPos[1] * 2 + 3) + """ stroke=""red"" fill=""none"" stroke-width=""2"">";
  svg += "  <animate attributeName=""r"" values=""2;4;6"" dur=""0.75s"" repeatCount=""indefinite"" />";
  svg += "</circle>";
  svg += "</svg></div>";
  s += svg;

  s += F("</table></div><div class=\"card\"><h3>Groundstation Status</h3><table id=""gsstatus"">");
  s += "<tr><td>Name </td><td>" + String(getThingName()) + "</td></tr>";
  s += "<tr><td>Version </td><td>" + String(status.version) + "</td></tr>";
  s += "<tr><td>MQTT Server </td><td>" + String(status.mqtt_connected ? "<span class='G'>CONNECTED</span>" : "<span class='R'>NOT CONNECTED</span>") + "</td></tr>";
  if (WiFi.isConnected() ){
      s += "<tr><td>WiFi RSSI </td><td>" + String(WiFi.RSSI()) + "</td></tr>";
  }

  s += "<tr><td>Radio </td><td>" + String(Radio::getInstance().isReady() ? "<span class='G'>READY</span>" : "<span class='R'>NOT READY</span>") + "</td></tr>";
   s += "<tr><td>Noise floor </td><td>" + String(status.modeminfo.currentRssi) + "</td></tr>"; 
  s += F("</table></div>");
  s += F("<div class=\"card\"><h3>Modem Configuration</h3><table id=""modemconfig"">");
  s += "<tr><td>Listening to </td><td>" + String(status.modeminfo.satellite) + "</td></tr>";
  s += "<tr><td>Modulation </td><td>" + String(status.modeminfo.modem_mode) + "</td></tr>";
  s += "<tr><td>Frequency </td><td>" + String(status.modeminfo.frequency) + "</td></tr>";
  if (status.modeminfo.modem_mode == "LoRa")
  {
    s += "<tr><td>Spreading Factor </td><td>" + String(status.modeminfo.sf) + "</td></tr>";
    s += "<tr><td>Coding Rate </td><td>" + String(status.modeminfo.cr) + "</td></tr>";
    s += "<tr><td>Bandwidth </td><td>" + String(status.modeminfo.bw) + "</td></tr>";
  }
  else
  {
    s += "<tr><td>Bitrate </td><td>" + String(status.modeminfo.bitrate) + "</td></tr>";
    s += "<tr><td>Frequency dev </td><td>" + String(status.modeminfo.freqDev) + "</td></tr>";
    s += "<tr><td>Bandwidth </td><td>" + String(status.modeminfo.bw) + "</td></tr>";
  }
  s += F("</table></div><div class=\"card\"><h3>Last Packet Received</h3><table id=""lastpacket"">");
  s += "<tr><td>Received at </td><td>" + String(status.lastPacketInfo.time) + "</td></tr>";
  s += "<tr><td>Signal RSSI </td><td>" + String(status.lastPacketInfo.rssi) + "</td></tr>";
  s += "<tr><td>Signal SNR </td><td>" + String(status.lastPacketInfo.snr) + "</td></tr>";
  s += "<tr><td>Frequency error </td><td>" + String(status.lastPacketInfo.frequencyerror) + "</td></tr>";
  s += "<tr><td colspan=\"2\" style=\"text-align:center;\">" + String(status.lastPacketInfo.crc_error ? "CRC ERROR!" : "") + "</td></tr>";
  s += F("</table></div>");
  s += FPSTR(IOTWEBCONF_CONSOLE_BODY_INNER);
  s += "<br /><button style='max-width: 1080px;' onclick=\"window.location.href='" + String(ROOT_URL) + "';\">Go Back</button><br /><br />";
  s += FPSTR(IOTWEBCONF_HTML_END);

  s.replace("{v}", FPSTR(TITLE_TEXT));

  // server.sendHeader("Content-Length", String(s.length()));
  server.send(200, "text/html; charset=UTF-8", s);
}

void ConfigManager::handleRefreshConsole()
{
  if (getState() == IOTWEBCONF_STATE_ONLINE)
  {
    // -- Authenticate
    if (!server.authenticate(IOTWEBCONF_ADMIN_USER_NAME, getApPasswordParameter()->valueBuffer))
    {
      IOTWEBCONF_DEBUG_LINE(F("Requesting authentication."));
      server.requestAuthentication();
      return;
    }
  }

  uint32_t counter = 0;

  String svalue = server.arg("c1");
  if (svalue.length())
  {
    Log::console(PSTR("COMMAND: %s"), svalue.c_str());

    if (strcmp(svalue.c_str(), "p") == 0)
    {
      if (!getAllowTx())
      {
        Log::console(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
      }
      else
      {
        static long lastTestPacketTime = 0;
        if (millis() - lastTestPacketTime < 20 * 1000)
        {
          Log::console(PSTR("Please wait a few seconds to send another test packet."));
        }
        else
        {
          Radio &radio = Radio::getInstance();
          radio.sendTestPacket();
          lastTestPacketTime = millis();
          Log::console(PSTR("Sending test packet to nearby stations!"));
        }
      }
    }
    else
    {
      // Log::console(PSTR("%s"), F("Command still not supported in web serial console!"));
      if (Radio::getInstance().sendTx((uint8_t *) svalue.c_str(), strlen(svalue.c_str())) == 0)
      {
        Log::console(PSTR("%s"), F("Message was sent successfully"));
      }
      else
      {
        Log::console(PSTR("%s"), F("An error occurred while sending the message!"));
      }
    }
  }

  char stmp[8];
  String s = server.arg("c2");
  strlcpy(stmp, s.c_str(), sizeof(stmp));
  if (strlen(stmp))
  {
    counter = atoi(stmp);
  }
  server.client().flush();
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("-1"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/plain"), "");
  server.sendContent(String((uint8_t)Log::getLogIdx()) + "\n");
  if (counter != Log::getLogIdx())
  {
    if (!counter)
    {
      counter = Log::getLogIdx();
    }
    do
    {
      char *tmp;
      size_t len;
      Log::getLog(counter, &tmp, &len);
      if (len)
      {
        char stemp[len + 1];
        memcpy(stemp, tmp, len);
        stemp[len - 1] = '\n';
        stemp[len] = '\0';
        server.sendContent(stemp);
      }
      counter++;
      counter &= 0xFF;
      if (!counter)
      {
        counter++;
      } // Skip log index 0 as it is not allowed
    } while (counter != Log::getLogIdx());
  }

  server.sendContent("");
  server.client().stop();
}

void ConfigManager::handleRefreshWorldmap()
{
  if (getState() == IOTWEBCONF_STATE_ONLINE)
  {
    // -- Authenticate
    if (!server.authenticate(IOTWEBCONF_ADMIN_USER_NAME, getApPasswordParameter()->valueBuffer))
    {
      IOTWEBCONF_DEBUG_LINE(F("Requesting authentication."));
      server.requestAuthentication();
      return;
    }
  }

  server.client().flush();
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("-1"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/plain"), "");

  // world map satellite position (for wmsatpos id attributes)
  String cx= String(status.satPos[0] * 2 + 3);
  String cy= String(status.satPos[1] * 2 + 3);
  String data_string = cx + "," + cy + ",";

  // modem configuration (for modemconfig id table data)
  data_string += String(status.modeminfo.satellite) + "," +
                 String(status.modeminfo.modem_mode) + "," +
                 String(status.modeminfo.frequency) + ",";
  if (status.modeminfo.modem_mode == "LoRa")
  {
    data_string += String(status.modeminfo.sf) + ",";
    data_string += String(status.modeminfo.cr) + ",";
  }
  else
  {
    data_string += String(status.modeminfo.bitrate) + ",";
    data_string += String(status.modeminfo.freqDev) + ",";
  }
  data_string += String(status.modeminfo.bw) + ",";

  // ground station status (for gsstatus id table data)
  data_string += String(getThingName()) + ",";
  data_string += String(status.version) + ",";
  data_string += String(status.mqtt_connected ? "<span class='G'>CONNECTED</span>" : "<span class='R'>NOT CONNECTED</span>") + ",";
  if (WiFi.isConnected() ){
    data_string += String(WiFi.RSSI()) + ",";
  }
  data_string += String(Radio::getInstance().isReady() ? "<span class='G'>READY</span>" : "<span class='R'>NOT READY</span>") + ",";
  Radio &radio = Radio::getInstance();
  radio.currentRssi();
  data_string += String(status.modeminfo.currentRssi) + ",";
  
  // last packet received data (for lastpacket id table data)
  data_string += String(status.lastPacketInfo.time) + ",";
  data_string += String(status.lastPacketInfo.rssi) + ",";
  data_string += String(status.lastPacketInfo.snr) + ",";
  data_string += String(status.lastPacketInfo.frequencyerror) + ",";
  data_string += String(status.lastPacketInfo.crc_error ? "CRC ERROR!" : "");
  server.sendContent(data_string + "\n");

  server.sendContent("");
  server.client().stop();
}

void ConfigManager::handleRestart()
{
  if (getState() == IOTWEBCONF_STATE_ONLINE)
  {
    // -- Authenticate
    if (!server.authenticate(IOTWEBCONF_ADMIN_USER_NAME, getApPasswordParameter()->valueBuffer))
    {
      IOTWEBCONF_DEBUG_LINE(F("Requesting authentication."));
      server.requestAuthentication();
      return;
    }
  }

  String s = String(FPSTR(IOTWEBCONF_HTML_HEAD));
  s += "<style>" + String(FPSTR(IOTWEBCONF_HTML_STYLE_INNER)) + "</style>";
  s += "<meta http-equiv=\"refresh\" content=\"8; url=/\">";
  s += FPSTR(IOTWEBCONF_HTML_HEAD_END);
  s += FPSTR(IOTWEBCONF_HTML_BODY_INNER);
  s += String(FPSTR(LOGO)) + "<br />";
  s += "Ground Station is restarting...<br /><br/>";
  s += FPSTR(IOTWEBCONF_HTML_END);

  s.replace("{v}", FPSTR(TITLE_TEXT));

  // server.sendHeader("Content-Length", String(s.length()));
  server.send(200, "text/html; charset=UTF-8", s);
  delay(100);
  ESP.restart();
}

bool ConfigManager::formValidator(iotwebconf2::WebRequestWrapper *webRequestWrapper)
{
  String name = webRequestWrapper->arg(this->getThingNameParameter()->getId());

  if (name.length() < 3)
  {
    this->getThingNameParameter()->errorMessage = "Your ground station name must have more then 3 characters and less then 25 and should be unique.";
    return false;
  }

  if (!strcmp(name.c_str(), thingName))
  {
    this->getThingNameParameter()->errorMessage = "Please, change your station name to something unique. Be creative!";
    return false;
  }

  for (const char *c = name.c_str(); *c != '\0'; c++)
  {
    if (!((*c >= '0' && *c <= '9') || (*c >= 'A' && *c <= 'Z') || *c == '_' || (*c >= 'a' && *c <= 'z')))
    {
      this->getThingNameParameter()->errorMessage = "Allowed characters are: [0-9 A-Z a-z _]";
      return false;
    }
  }

  return true;
}

void ConfigManager::resetAPConfig()
{
  getWifiSsidParameter()->valueBuffer[0] = '\0';
  getWifiPasswordParameter()->valueBuffer[0] = '\0';
  strncpy(getApPasswordParameter()->valueBuffer, initialApPassword, IOTWEBCONF_WORD_LEN);
  strncpy(getThingNameParameter()->valueBuffer, thingName, IOTWEBCONF_WORD_LEN);
  strncpy(getThingName(), thingName, IOTWEBCONF_WORD_LEN);

  saveConfig();
}

void ConfigManager::resetAllConfig()
{
  getWifiSsidParameter()->valueBuffer[0] = '\0';
  getWifiPasswordParameter()->valueBuffer[0] = '\0';
  strncpy(getApPasswordParameter()->valueBuffer, initialApPassword, IOTWEBCONF_WORD_LEN);
  strncpy(getThingNameParameter()->valueBuffer, thingName, IOTWEBCONF_WORD_LEN);
  strncpy(getThingName(), thingName, IOTWEBCONF_WORD_LEN);
  strncpy(mqttPortParam.valueBuffer, MQTT_DEFAULT_PORT, MQTT_PORT_LENGTH);
  strncpy(mqttServerParam.valueBuffer, MQTT_DEFAULT_SERVER, MQTT_SERVER_LENGTH);
  mqttUserParam.valueBuffer[0] = '\0';
  mqttPassParam.valueBuffer[0] = '\0';
  latitude[0] = '\0';
  longitude[0] = '\0';
  oledBright[0] = '\0';
  allowTx[0] = '\0';
  remoteTune[0] = '\0';
  telemetry3rd[0] = '\0';
  testMode[0] = '\0';
  autoUpdate[0] = '\0';
  boardTemplate[0] = '\0';
  modemStartup[0] = '\0';
  advancedConfig[0] = '\0';

  saveConfig();
}

boolean ConfigManager::init()
{
  boolean validConfig = IotWebConf2::init();

  // when wifi credentials are set but we are not able to connect (maybe wrong credentials)
  // we fall back to AP mode during 2 minutes after which we try to connect again and repeat.
  setApTimeoutMs(atoi(AP_TIMEOUT_MS));

  // no board selected
  if (!strcmp(board, ""))
  {
    boardDetection();
  }

  if (strlen(advancedConfig))
    parseAdvancedConf();

  parseModemStartup();

  strcpy(savedThingName, this->getThingName());
  return validConfig;
}

void ConfigManager::boardDetection()
{
  // List all compatible boards configuration
  /*Serial.println(F("\nSupported boards:"));
  for (uint8_t ite=0; ite<((sizeof(boards)/sizeof(boards[0])));ite++)
  {
    Serial.println("");
    Serial.println(boards[ite].BOARD);
    Serial.print(F(" OLED: Adrs 0x"));    Serial.print(boards[ite].OLED__address,HEX);
    Serial.print(F(" SDA:"));      Serial.print(boards[ite].OLED__SDA);
    Serial.print(F(" SCL:"));      Serial.print(boards[ite].OLED__SCL);
    Serial.print(F(" RST:"));      Serial.print(boards[ite].OLED__RST);
    Serial.print(F(" BUTTON:"));   Serial.println(boards[ite].PROG__BUTTON);
    Serial.print(F(" Lora Module "));
    if (boards[ite].L_radio) {Serial.print(F("SX1278 ")); } else {Serial.print(F("SX1268:"));} ;
    Serial.print(F(" NSS:"));      Serial.print(boards[ite].L_NSS);
    Serial.print(F(" MOSI:"));     Serial.print(boards[ite].L_MOSI);
    Serial.print(F(" MISO:"));     Serial.print(boards[ite].L_MISO);
    Serial.print(F(" SCK:"));      Serial.print(boards[ite].L_SCK);

    if (boards[ite].L_DI00) {Serial.print(F(" DI00:")); Serial.print(boards[ite].L_DI00);}
    if (boards[ite].L_DI01) {Serial.print(F(" DI01:")); Serial.print(boards[ite].L_DI01);}
    if (boards[ite].L_BUSSY) {Serial.print(F(" BUSSY:")); Serial.print(boards[ite].L_BUSSY);}
    Serial.println("");
  }*/

  // test OLED configuration
  Log::error(PSTR("Automatic board detection running... "));
 
 
  // If the cpu is a ESP32-PICO-D4 (Lilygo T3_v1.6.1) we have to avoid using pin GPIO16 as this is used for the FLASH_CS 
  // https://github.com/mpmarks/tinyGS-newboards/commit/e520086f1b43c7cea4cb85d996f0fc379f2d2786

#if CONFIG_IDF_TARGET_ESP32S3
// nothing yet
#elif CONFIG_IDF_TARGET_ESP32C3
// nothing yet
#else
  if (strcmp(ESP.getChipModel(), "ESP32-PICO-D4") != 0) {
        for (uint8_t ite = 0; ite < ((sizeof(boards) / sizeof(boards[0]))); ite++)
      {
        Log::error(PSTR("%s \n"), boards[ite].BOARD);
      if (boards[ite].OLED__RST != UNUSED) {
        pinMode(boards[ite].OLED__RST, OUTPUT);
        digitalWrite(boards[ite].OLED__RST, LOW);
        delay(25);
        digitalWrite(boards[ite].OLED__RST, HIGH);
      }
        Wire.begin(boards[ite].OLED__SDA, boards[ite].OLED__SCL);
        Wire.beginTransmission(boards[ite].OLED__address);
        if (!Wire.endTransmission())
        {
          Log::error(PSTR("Compatible OLED FOUND"));
          itoa(ite, board, 10);
          return;
        }
        else
        {
          Log::error(PSTR("Not Compatible board found, please select it manually on the web config panel"));
        }
      }
    };
#endif
}

void ConfigManager::printConfig()
{
  Log::debug(PSTR("MQTT Port: %u\nMQTT Server: %s\nMQTT Pass: %s\nLatitude: %f\nLongitude: %f"), getMqttPort(), getMqttServer(), getMqttPass(), getLatitude(), getLongitude());
  Log::debug(PSTR("tz: %s\nOLED Bright: %u\nTX %s"), getTZ(),  getOledBright(), getAllowTx() ? "Enable" : "Disable");
  if (getBoardTemplate()[0] != '\0') 
    Log::debug(PSTR("board_template: %s"),getBoardTemplate());
  else 
    Log::debug(PSTR("board: %u --> %s\n:"),getBoard(), boards[getBoard()].BOARD.c_str());
}

void ConfigManager::configSavedCallback()
{
  // If the station name changes we have to restart as it is considered a different station
  if (strcmp(getThingName(), savedThingName))
  {
    ESP.restart();
  }

  if (!remoteSave) // remote save is set to true when saving programatically, it's false if the callback comes from web
  {
    forceApMode(false);
    parseModemStartup();
    MQTT_Client::getInstance().scheduleRestart();
 
    // Prog button already pressed so something is wrong.. trying to amend it..
    if (!digitalRead(boards[getBoard()].PROG__BUTTON)) {
        Log::error(PSTR("Wrong selection Prog button pressed, trying to solve it"));
        switch (getBoard()) {
          case 8:
               Log::error(PSTR("8->14"));
               strcpy(board, "14");
               this->saveConfig();
               break;
          case 9:
               Log::error(PSTR("9->18"));
               strcpy(board, "18");
               this->saveConfig();
               break;
          case 14:
               Log::error(PSTR("14->8"));
               strcpy(board, "8");
               this->saveConfig();
               break;
          case 18:
               Log::error(PSTR("18->9"));
               strcpy(board, "9");
               this->saveConfig();
               break;
        } 
      // seems that prog butto is still pressed wrong so chosing a safe config.
        if (!digitalRead(boards[getBoard()].PROG__BUTTON)) {
               Log::error(PSTR("Wrong board moving to a safe config"));
               strcpy(board, "0");
               this->saveConfig();
                    }
    }
  }

  parseAdvancedConf();
  remoteSave = false; // reset to false so web callbacks are received as false
  
}

void ConfigManager::parseAdvancedConf()
{
  if (!strlen(advancedConfig))
    return;

  size_t size = 512;
  DynamicJsonDocument doc(size);
  deserializeJson(doc, (const char *)advancedConfig);

  if (doc.containsKey(F("dmode")))
  {
    Log::setLogLevel(doc["dmode"]);
  }

  if (doc.containsKey(F("flipOled")))
  {
    advancedConf.flipOled = doc["flipOled"];
  }

  if (doc.containsKey(F("dnOled")))
  {
    advancedConf.dnOled = doc["dnOled"];
  }

  if (doc.containsKey(F("lowPower")))
  {
    advancedConf.lowPower = doc["lowPower"];
  }
}

void ConfigManager::parseModemStartup()
{
  if (modemStartup[0] == '\0')
    return; // no modem configured yet
  
  size_t size = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(16) + JSON_ARRAY_SIZE(8) + JSON_ARRAY_SIZE(8) + 64;
  DynamicJsonDocument doc(size);
  DeserializationError error = deserializeJson(doc, (const char *)modemStartup);

  if (error.code() != DeserializationError::Ok || !doc.containsKey("mode"))
  {
    Log::console(PSTR("ERROR: Your modem config is invalid. Resetting to default"));
    modemStartup[0] = '\0';
    saveConfig();
    return;
  }

  ModemInfo &m = status.modeminfo;
  m.modem_mode = doc["mode"].as<String>();
  strcpy(m.satellite, doc["sat"].as<char *>());
  m.NORAD = doc["NORAD"];

  if (m.modem_mode == "LoRa")
  {
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
  }
  else
  {
    m.frequency = doc["freq"];
    m.bw = doc["bw"];
    m.bitrate = doc["br"];
    m.freqDev = doc["fd"];
    m.power = doc["pwr"];
    m.preambleLength = doc["pl"];
    m.OOK = doc["ook"];
    m.len = doc["len"];
    m.swSize = doc["fsw"].size();
    for (int i = 0; i < 8; i++)
    {
      if (i < m.swSize)
        m.fsw[i] = doc["fsw"][i];
      else
        m.fsw[i] = 0;
    }
    m.enc= doc["enc"];
  }

  // packets Filter
  uint8_t filterSize = doc["filter"].size();
  for (int i = 0; i < 8; i++)
  {
    if (i < filterSize)
      status.modeminfo.filter[i] = doc["filter"][i];
    else
      status.modeminfo.filter[i] = 0;
  }

  if (Radio::getInstance().isReady())
    Radio::getInstance().begin();
}

bool ConfigManager::parseBoardTemplate(board_t &board)
{
  size_t size = 512;
  DynamicJsonDocument doc(size);
  DeserializationError error = deserializeJson(doc, ConfigManager::getInstance().getBoardTemplate());

  if (error.code() != DeserializationError::Ok || !doc.containsKey("radio"))
  {
    Log::console(PSTR("Error: Your Board template is not valid. Unable to finish setup."));
    return false;
  }

  board.OLED__address = doc["aADDR"];
  board.OLED__SDA = doc["oSDA"];
  board.OLED__SCL = doc["oSCL"];
  if (doc.containsKey("oRST"))
    board.OLED__RST = doc["oRST"];
  else
    board.OLED__RST = UNUSED;
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
  if (doc.containsKey("RXEN"))
    board.RX_EN = doc["RXEN"];
  else
    board.RX_EN = UNUSED;
  if (doc.containsKey("TXEN"))
    board.TX_EN = doc["TXEN"];
  else
    board.TX_EN = UNUSED;

  return true;
}
