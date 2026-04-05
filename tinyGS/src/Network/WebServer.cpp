/*
  WebServer.cpp - Native ESP-IDF HTTP server replacing IotWebConf2 web UI

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

#include "WebServer.h"
#include "../Logger/Logger.h"
#include "../Radio/Radio.h"
#include "../Mqtt/MQTT_credentials.h"
#include <Update.h>
#include <mbedtls/base64.h>

TinyGSWebServer::TinyGSWebServer() {}

// ---- INetworkAware ----
void TinyGSWebServer::onConnected(IPAddress ip, ActiveInterface iface) {
  begin();
}

void TinyGSWebServer::onDisconnected() {
  // keep server running in AP mode too
}

void TinyGSWebServer::onInterfaceChanged(ActiveInterface iface, IPAddress ip) {
  // server keeps running, no action needed
}

// ---- Start / Stop ----
void TinyGSWebServer::begin() {
  if (_server) return; // already running

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;
  config.stack_size = 8192;
  config.uri_match_fn = httpd_uri_match_wildcard;

  if (httpd_start(&_server, &config) == ESP_OK) {
    registerHandlers();
    LOG_CONSOLE(PSTR("Web server started on port %d"), config.server_port);
  } else {
    LOG_ERROR(PSTR("Failed to start web server"));
  }
}

void TinyGSWebServer::stop() {
  if (_server) {
    httpd_stop(_server);
    _server = nullptr;
  }
}

void TinyGSWebServer::registerHandlers() {
  // GET handlers
  httpd_uri_t root_get       = { ROOT_URL,             HTTP_GET,  handleRoot,            this };
  httpd_uri_t logo_get       = { LOGO_URL,             HTTP_GET,  handleLogo,            this };
  httpd_uri_t config_get     = { CONFIG_URL,            HTTP_GET,  handleConfig,          this };
  httpd_uri_t config_post    = { CONFIG_URL,            HTTP_POST, handleConfigPost,      this };
  httpd_uri_t dashboard_get  = { DASHBOARD_URL,         HTTP_GET,  handleDashboard,       this };
  httpd_uri_t console_get    = { REFRESH_CONSOLE_URL,   HTTP_GET,  handleRefreshConsole,  this };
  httpd_uri_t worldmap_get   = { REFRESH_WORLDMAP_URL,  HTTP_GET,  handleRefreshWorldmap, this };
  httpd_uri_t restart_get    = { RESTART_URL,           HTTP_GET,  handleRestart,         this };
  httpd_uri_t firmware_get   = { UPDATE_URL,            HTTP_GET,  handleFirmware,        this };
  httpd_uri_t firmware_post  = { UPDATE_URL,            HTTP_POST, handleFirmwarePost,    this };

  httpd_register_uri_handler(_server, &root_get);
  httpd_register_uri_handler(_server, &logo_get);
  httpd_register_uri_handler(_server, &config_get);
  httpd_register_uri_handler(_server, &config_post);
  httpd_register_uri_handler(_server, &dashboard_get);
  httpd_register_uri_handler(_server, &console_get);
  httpd_register_uri_handler(_server, &worldmap_get);
  httpd_register_uri_handler(_server, &restart_get);
  httpd_register_uri_handler(_server, &firmware_get);
  httpd_register_uri_handler(_server, &firmware_post);
}

// ---- Authentication ----
bool TinyGSWebServer::authenticate(httpd_req_t* req) {
  // In AP mode, skip authentication
  ConnectionManager& cm = ConnectionManager::getInstance();
  if (cm.isApMode()) return true;

  char authBuf[128] = {0};
  if (httpd_req_get_hdr_value_str(req, "Authorization", authBuf, sizeof(authBuf)) != ESP_OK) {
    return false;
  }

  // Parse "Basic <base64>"
  if (strncmp(authBuf, "Basic ", 6) != 0) return false;

  const char* b64 = authBuf + 6;
  unsigned char decoded[96];
  size_t decodedLen = 0;
  if (mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &decodedLen,
                            (const unsigned char*)b64, strlen(b64)) != 0) {
    return false;
  }
  decoded[decodedLen] = '\0';

  // Expected: "admin:<password>"
  ConfigStore& cfg = ConfigStore::getInstance();
  String expected = String(ADMIN_USER) + ":" + String(cfg.getApPassword());
  return expected.equals((const char*)decoded);
}

esp_err_t TinyGSWebServer::sendAuthRequired(httpd_req_t* req) {
  httpd_resp_set_status(req, "401 Unauthorized");
  httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"TinyGS\"");
  httpd_resp_sendstr(req, "Authentication required");
  return ESP_OK;
}

// =====================================================================
// ---- GET / (Root page) ----
// =====================================================================
esp_err_t TinyGSWebServer::handleRoot(httpd_req_t* req) {
  String s = buildRootPage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());
  return ESP_OK;
}

String TinyGSWebServer::buildRootPage() {
  ConfigStore& cfg = ConfigStore::getInstance();

  String s = String(FPSTR(HTML_HEAD));
  s += "<style>" + String(FPSTR(HTML_STYLE_INNER)) + "</style>";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(HTML_BODY_INNER);
  s += "<div class='logo-wrap'><img class='logo' src=\"" + String(LOGO_URL) + "\"></div>";

  if (cfg.getMqttServer()[0] == '\0' || cfg.getMqttUser()[0] == '\0' || cfg.getMqttPass()[0] == '\0') {
    s += F("<div>Device is not connected to tinyGS:</div>");
    s += F("<table style=\"width:75%;\">");
    s += "<tr><td style=\"text-align:left;\">OTP code:</td><td style=\"text-align:left;\"><b>" + String(mqttCredentials.getOTPCode()) + "</b></td></tr>";
    s += "</table><br />";
  }

  s += "<button onclick=\"window.location.href='" + String(DASHBOARD_URL) + "';\">Station dashboard</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(CONFIG_URL) + "';\">Configure parameters</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(UPDATE_URL) + "';\">Upload new version</button><br /><br />";
  s += "<button onclick=\"window.location.href='" + String(RESTART_URL) + "';\">Restart Station</button><br /><br />";

  if (cfg.getThingName()[0] == 'M' && cfg.getThingName()[2] == ' ' && cfg.getMqttPass()[0] == '\0') {
    s += F("<table style=\"width:75%;\">");
    s += "<tr><td style=\"text-align:left;\">OTP code:</td><td style=\"text-align:left;\"><b><a href=\"https://tinygs.com/user/addstation\">" + String(mqttCredentials.getOTPCode()) + "</a></b></td></tr>";
    s += "</table><br />";
    s += F("<div>Default local dashboard credentials:</div>");
    s += F("<table style=\"width:75%;\">");
    s += F("<tr><td style=\"text-align:left;\">user:</td><td style=\"text-align:left;\"><b>admin</b></td></tr>");
    s += "<tr><td style=\"text-align:left;\">password:</td><td style=\"text-align:left;\"><b>" + String(cfg.getApPassword()) + "</b></td></tr>";
    s += "</table>";
  }

  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));
  return s;
}

// =====================================================================
// ---- GET /logo.png ----
// =====================================================================
esp_err_t TinyGSWebServer::handleLogo(httpd_req_t* req) {
  httpd_resp_set_type(req, "image/png");
  httpd_resp_send(req, LOGO_PNG, sizeof(LOGO_PNG));
  return ESP_OK;
}

// =====================================================================
// ---- GET /dashboard ----
// =====================================================================
esp_err_t TinyGSWebServer::handleDashboard(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  String s = buildDashboardPage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());
  return ESP_OK;
}

String TinyGSWebServer::buildWorldMapSVG() {
  String svg = "<div class='map-wrap'><svg viewBox=\"0 0 262 134\" xmlns=\"http://www.w3.org/2000/svg\">";
  svg += "<style>.mb{fill:var(--map-bg)}.md{stroke:var(--map-border);fill:none}.ml{fill:var(--map-land)}</style>";
  svg += "<rect class=\"mb\" x=\"0\" y=\"0\" width=\"262\" height=\"134\" rx=\"4\"/>";
  svg += "<rect class=\"md\" x=\"1\" y=\"1\" width=\"260\" height=\"132\" stroke-width=\"1\" rx=\"3\"/>";

  uint ix = 0;
  uint sx;
  for (uint y = 0; y < earth_height; y++) {
    uint n = 0;
    for (uint x = 0; x < earth_width / 8; x++) {
      for (uint i = 0; i < 8; i++) {
        if ((earth_bits[ix] >> i) & 1) {
          if (n == 0) sx = (x * 8) + i;
          n++;
        }
        if (!((earth_bits[ix] >> i) & 1) || ((x == earth_width / 8 - 1) && (i == 7))) {
          if (n > 0) {
            svg += "<rect class=\"ml\" x=\"" + String(sx * 2 + 3) + "\" y=\"" + String(y * 2 + 3) + "\" width=\"" + String(n * 2) + "\" height=\"2\"/>"; 
            n = 0;
          }
        }
      }
      ix++;
    }
  }

  svg += "<circle id=\"wmsatpos\" cx=\"" + String(status.satPos[0] * 2 + 3) + "\" cy=\"" + String(status.satPos[1] * 2 + 3) + "\" stroke=\"#3b82f6\" fill=\"none\" stroke-width=\"1.5\">";
  svg += "  <animate attributeName=\"r\" values=\"2;5;8\" dur=\"1s\" repeatCount=\"indefinite\" />";
  svg += "  <animate attributeName=\"opacity\" values=\"1;0.4;0\" dur=\"1s\" repeatCount=\"indefinite\" />";
  svg += "</circle>";
  svg += "<circle cx=\"" + String(status.satPos[0] * 2 + 3) + "\" cy=\"" + String(status.satPos[1] * 2 + 3) + "\" r=\"2\" fill=\"#3b82f6\"/>";
  svg += "</svg></div>";
  return svg;
}

String TinyGSWebServer::buildDashboardPage() {
  ConfigStore& cfg = ConfigStore::getInstance();

  String s = String(FPSTR(HTML_HEAD));
  s += "<style>" + String(FPSTR(HTML_STYLE_INNER)) + "</style>";
  s += "<style>" + String(FPSTR(IOTWEBCONF_DASHBOARD_STYLE_INNER)) + "</style>";
  s += "<script>" + String(FPSTR(IOTWEBCONF_CONSOLE_SCRIPT)) + "</script>";
  s += "<script>" + String(FPSTR(IOTWEBCONF_WORLDMAP_SCRIPT)) + "</script>";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(IOTWEBCONF_DASHBOARD_BODY_INNER);
  s += "<div style='text-align:center;margin-bottom:0.75rem;'><img class='logo' src=\"" + String(LOGO_URL) + "\" style='max-width:160px;'></div>";

  s += buildWorldMapSVG();

  if (cfg.getMqttServer()[0] == '\0' || cfg.getMqttUser()[0] == '\0' || cfg.getMqttPass()[0] == '\0') {
    s += F("<div style='background:rgba(239,68,68,0.08);border:1px solid var(--danger);border-radius:var(--radius);padding:0.8rem;margin:0.5rem auto;max-width:560px;text-align:center;'>");
    s += F("<span style='font-size:0.85rem;'>Device not connected to TinyGS &mdash; OTP: </span>");
    s += "<a href='https://tinygs.com/user/addstation' style='font-weight:700;font-size:1.1rem;'>" + String(mqttCredentials.getOTPCode()) + "</a>";
    s += F("</div>");
  }

  // Ground Station Status
  s += F("<div class='cards'><div class='card'><h3>Ground Station</h3><table id=\"gsstatus\">");
  s += "<tr><td>Name </td><td>" + String(cfg.getThingName()) + "</td></tr>";
  s += "<tr><td>Version </td><td>" + String(status.version) + "</td></tr>";
  s += "<tr><td>MQTT</td><td>" + String(status.mqtt_connected ? "<span class='G'>CONNECTED</span>" : "<span class='R'>NOT CONNECTED</span>") + "</td></tr>";

  ConnectionManager& cm = ConnectionManager::getInstance();
  if (cm.isConnected()) {
    if (cm.getActiveInterface() == ActiveInterface::WIFI) {
      s += "<tr><td>WiFi</td><td>" + String(cm.getNetworkRSSI()) + "</td></tr>";
    } else {
      s += "<tr><td>Network</td><td><span class='G'>" + cm.getActiveInterfaceName() + "</span></td></tr>";
    }
  } else {
    s += "<tr><td>WiFi</td><td><span class='R'>NOT CONNECTED</span></td></tr>";
  }

  s += "<tr><td>Radio </td><td>" + String(Radio::getInstance().isReady() ? "<span class='G'>READY</span>" : "<span class='R'>NOT READY</span>") + "</td></tr>";
  s += "<tr><td>Noise Floor</td><td>" + String(status.modeminfo.currentRssi) + "</td></tr>";
  s += F("</table></div>");

  // Modem Configuration
  s += F("<div class='card'><h3>Modem Config</h3><table id=\"modemconfig\">");
  s += "<tr><td>Modulation </td><td>" + String(status.modeminfo.modem_mode) + "</td></tr>";
  s += "<tr><td>Frequency </td><td>" + String(status.modeminfo.frequency) + "</td></tr>";
  s += "<tr><td>Freq. Offset </td><td>" + String(status.modeminfo.freqOffset) + "</td></tr>";

  if (strcmp(status.modeminfo.modem_mode, "LoRa") == 0) {
    s += "<tr><td>Spreading Factor </td><td>" + String(status.modeminfo.sf) + "</td></tr>";
    s += "<tr><td>Coding Rate </td><td>" + String(status.modeminfo.cr) + "</td></tr>";
  } else {
    s += "<tr><td>Bitrate </td><td>" + String(status.modeminfo.bitrate) + "</td></tr>";
    s += "<tr><td>Frequency dev </td><td>" + String(status.modeminfo.freqDev) + "</td></tr>";
  }
  s += "<tr><td>Bandwidth </td><td>" + String(status.modeminfo.bw) + "</td></tr>";

  // Time strings
  char timeStr[10];
  time_t currentTime = time(NULL);
  if (currentTime > 0) {
    struct tm* timeinfo = gmtime(&currentTime);
    snprintf_P(timeStr, sizeof(timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  } else {
    timeStr[0] = '\0';
  }

  // Satellite Tracking
  s += F("</table></div><div class='card'><h3>Satellite Tracking</h3><table id=\"satdata\">");
  s += "<tr><td>Listening to </td><td>" + String(status.modeminfo.satellite) + "</td></tr>";
  if (status.modeminfo.tle[0] != 0) {
    s += "<tr><td>Lat / Lon </td><td>" + String(status.tle.dSatLAT) + "° / " + String(status.tle.dSatLON) + "° </td></tr>";
    s += "<tr><td>Az  / El  </td><td>" + String(status.tle.dSatAZ) + "° / " + String(status.tle.dSatEL) + "° </td></tr>";
    s += "<tr><td>Doppler </td><td>" + String(status.tle.new_freqDoppler) + " Hz </td></tr>";
  } else {
    s += "<tr><td>Lat / Lon </td><td> - / - </td></tr>";
    s += "<tr><td>Az  / El  </td><td> - / - </td></tr>";
    s += "<tr><td>Doppler </td><td>  -  </td></tr>";
  }

  s += "<tr><td>UTC Time </td><td>" + String(timeStr) + "</td></tr>";
  if (currentTime > 0) {
    struct tm* timeinfo = localtime(&currentTime);
    snprintf_P(timeStr, sizeof(timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  } else {
    timeStr[0] = '\0';
  }
  s += "<tr><td>Local Time </td><td>" + String(timeStr) + "</td></tr>";
  s += F("</table></div>");

  // Last Packet
  s += F("<div class='card'><h3>Last Packet</h3><table id=\"lastpacket\">");
  s += "<tr><td>Received at </td><td>" + String(status.lastPacketInfo.time) + "</td></tr>";
  s += "<tr><td>Signal RSSI </td><td>" + String(status.lastPacketInfo.rssi) + "</td></tr>";
  s += "<tr><td>Signal SNR </td><td>" + String(status.lastPacketInfo.snr) + "</td></tr>";
  s += "<tr><td>Freq. Error</td><td>" + String(status.lastPacketInfo.frequencyerror) + "</td></tr>";
  s += "<tr><td colspan=\"2\" style=\"text-align:center;\">" + String(status.lastPacketInfo.crc_error ? "CRC ERROR!" : "") + "</td></tr>";
  s += F("</table></div></div>"); // close last card + .cards grid

  // Console
  s += FPSTR(IOTWEBCONF_CONSOLE_BODY_INNER);
  s += "<br /><button style='max-width: 1080px;' onclick=\"window.location.href='" + String(ROOT_URL) + "';\">Go Back</button><br /><br />";
  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));
  return s;
}

// =====================================================================
// ---- GET /cs (Console refresh - AJAX) ----
// =====================================================================
esp_err_t TinyGSWebServer::handleRefreshConsole(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  TinyGSWebServer* self = (TinyGSWebServer*)req->user_ctx;

  // Parse query string
  char queryBuf[256] = {0};
  httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf));

  char c1Val[128] = {0};
  char c2Val[16] = {0};
  httpd_query_key_value(queryBuf, "c1", c1Val, sizeof(c1Val));
  httpd_query_key_value(queryBuf, "c2", c2Val, sizeof(c2Val));

  // Process command
  String svalue(c1Val);
  if (svalue.length()) {
    // URL-decode the command  
    svalue.replace("+", " ");
    // Simple percent-decoding for common chars
    svalue.replace("%20", " ");
    svalue.replace("%21", "!");

    LOG_CONSOLE_ASYNC(PSTR("COMMAND: %s"), svalue.c_str());

    if (strcmp(svalue.c_str(), "!p") == 0) {
      ConfigStore& cfg = ConfigStore::getInstance();
      if (!cfg.getAllowTx()) {
        LOG_CONSOLE_ASYNC(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
      } else {
        static long lastTestPacketTime = 0;
        if (millis() - lastTestPacketTime < 20 * 1000) {
          LOG_CONSOLE_ASYNC(PSTR("Please wait a few seconds to send another test packet."));
        } else {
          Radio& radio = Radio::getInstance();
          radio.sendTestPacket();
          lastTestPacketTime = millis();
          LOG_CONSOLE_ASYNC(PSTR("Sending test packet to nearby stations!"));
        }
      }
    } else if (strcmp(svalue.c_str(), "!w") == 0) {
      LOG_CONSOLE_ASYNC(PSTR("Getting weblogin"));
      self->_askForWeblogin = true;
    } else if (strcmp(svalue.c_str(), "!e") == 0) {
      ConfigStore::getInstance().resetAllConfig();
      ESP.restart();
    } else if (strcmp(svalue.c_str(), "!o") == 0) {
      LOG_CONSOLE_ASYNC("OTP Code: %s", mqttCredentials.getOTPCode());
    } else {
      LOG_CONSOLE_ASYNC(PSTR("Command still not supported in web serial console!"));
    }
  }

  uint32_t counter = 0;
  if (strlen(c2Val)) {
    counter = atoi(c2Val);
  }

  // Build response
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Expires", "-1");

  String response = String((uint8_t)Log::getLogIdx()) + "\n";

  if (counter != Log::getLogIdx()) {
    if (!counter) counter = Log::getLogIdx();

    do {
      char* tmp;
      size_t len;
      Log::getLog(counter, &tmp, &len);
      if (len) {
        char stemp[len + 1];
        memcpy(stemp, tmp, len);
        stemp[len - 1] = '\n';
        stemp[len] = '\0';
        response += stemp;
      }
      counter++;
      counter &= 0xFF;
      if (!counter) counter++;
    } while (counter != Log::getLogIdx());
  }

  httpd_resp_sendstr(req, response.c_str());
  return ESP_OK;
}

// =====================================================================
// ---- GET /wm (Worldmap refresh - AJAX) ----
// =====================================================================
esp_err_t TinyGSWebServer::handleRefreshWorldmap(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  ConfigStore& cfg = ConfigStore::getInstance();
  ConnectionManager& cm = ConnectionManager::getInstance();

  String data;
  // satellite position
  data += String(status.satPos[0] * 2 + 3) + ",";
  data += String(status.satPos[1] * 2 + 3) + ",";

  // modem config
  data += String(status.modeminfo.modem_mode) + ",";
  data += String(status.modeminfo.frequency) + ",";
  data += String(status.modeminfo.freqOffset) + ",";
  if (strcmp(status.modeminfo.modem_mode, "LoRa") == 0) {
    data += String(status.modeminfo.sf) + ",";
    data += String(status.modeminfo.cr) + ",";
  } else {
    data += String(status.modeminfo.bitrate) + ",";
    data += String(status.modeminfo.freqDev) + ",";
  }
  data += String(status.modeminfo.bw) + ",";

  // ground station status
  data += String(cfg.getThingName()) + ",";
  data += String(status.version) + ",";
  data += String(status.mqtt_connected ? "<span class='G'>CONNECTED</span>" : "<span class='R'>NOT CONNECTED</span>") + ",";

  if (cm.isConnected()) {
    if (cm.getActiveInterface() == ActiveInterface::WIFI) {
      data += "<span class='G'>" + cm.getActiveInterfaceName() + " (" + String(cm.getNetworkRSSI()) + " dBm)</span>,";
    } else {
      data += "<span class='G'>" + cm.getActiveInterfaceName() + "</span>,";
    }
  } else {
    data += "<span class='R'>NOT CONNECTED</span>,";
  }

  data += String(Radio::getInstance().isReady() ? "<span class='G'>READY</span>" : "<span class='R'>NOT READY</span>") + ",";
  if (status.radio_ready) Radio::getInstance().currentRssi();
  data += String(status.modeminfo.currentRssi) + ",";

  // satellite info
  char timeStr[10];
  time_t currentTime = time(NULL);

  data += String(status.modeminfo.satellite) + ",";
  if (status.modeminfo.tle[0] != 0) {
    data += String(status.tle.dSatLAT) + "° / " + String(status.tle.dSatLON) + "° ,";
    data += String(status.tle.dSatAZ) + "° / " + String(status.tle.dSatEL) + "° ,";
    data += String(status.tle.new_freqDoppler) + " Hz,";
  } else {
    data += " - / - ,";
    data += " - / - ,";
    data += " - ,";
  }

  if (currentTime > 0) {
    struct tm* timeinfo = gmtime(&currentTime);
    snprintf_P(timeStr, sizeof(timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  } else {
    timeStr[0] = '\0';
  }
  data += String(timeStr) + ",";

  if (currentTime > 0) {
    struct tm* timeinfo = localtime(&currentTime);
    snprintf_P(timeStr, sizeof(timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  } else {
    timeStr[0] = '\0';
  }
  data += String(timeStr) + ",";

  // last packet
  data += String(status.lastPacketInfo.time) + ",";
  data += String(status.lastPacketInfo.rssi) + ",";
  data += String(status.lastPacketInfo.snr) + ",";
  data += String(status.lastPacketInfo.frequencyerror) + ",";
  data += String(status.lastPacketInfo.crc_error ? "CRC ERROR!" : "");

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
  httpd_resp_sendstr(req, (data + "\n").c_str());
  return ESP_OK;
}

// =====================================================================
// ---- GET /restart ----
// =====================================================================
esp_err_t TinyGSWebServer::handleRestart(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  String s = buildRestartPage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());

  delay(100);
  ESP.restart();
  return ESP_OK;
}

String TinyGSWebServer::buildRestartPage() {
  String s = String(FPSTR(HTML_HEAD));
  s += "<style>" + String(FPSTR(HTML_STYLE_INNER)) + "</style>";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(HTML_BODY_INNER);
  s += "<div><img src=\"" + String(LOGO_URL) + "\"></div><br/>";
  s += "<strong>Configuration saved.</strong><br/><br/>";
  s += "Ground Station is restarting&hellip;<br/><br/>";
  s += "<span id='msg'>Waiting for the board to come back online&hellip;</span><br/><br/>";
  s += "<small id='cnt'></small>";
  s += "<script>"
       "var t=12;"
       "function tick(){"
       "if(t>0){"
       "document.getElementById('cnt').textContent='Reloading in '+t+' s...';"
       "t--;setTimeout(tick,1000);"
       "}else{"
       "fetch('/').then(function(r){"
       "if(r.ok)window.location.replace('/');"
       "else retry();"
       "}).catch(retry);"
       "}}"
       "function retry(){"
       "document.getElementById('cnt').textContent='Not ready yet, retrying...';"
       "setTimeout(function(){fetch('/').then(function(r){if(r.ok)window.location.replace('/');else retry();}).catch(retry);},2000);"
       "}"
       "setTimeout(tick,1000);"
       "</script>";
  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));
  return s;
}

// =====================================================================
// ---- GET /config ---- (Configuration form)
// =====================================================================
esp_err_t TinyGSWebServer::handleConfig(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  String s = buildConfigPage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());
  return ESP_OK;
}

String TinyGSWebServer::buildConfigPage() {
  ConfigStore& cfg = ConfigStore::getInstance();

  String s = String(FPSTR(HTML_HEAD));
  s += "<style>" + String(FPSTR(HTML_STYLE_INNER)) + "</style>";
  s += "<style>" + String(FPSTR(IOTWEBCONF_CONFIG_STYLE_INNER)) + "</style>";
  s += "<script>" + String(FPSTR(ADVANCED_CONFIG_SCRIPT)) + "</script>";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(HTML_BODY_INNER);
  s += "<div><img src=\"" + String(LOGO_URL) + "\"></div><br/>";

  s += "<form action=\"" + String(CONFIG_URL) + "\" method=\"post\">";

  // ---- Station identity ----
  s += F("<fieldset><legend>Station</legend>");
  s += F("<div><label for='thing_name'>GroundStation Name (will be seen on the map)</label>");
  s += "<input type='text' id='thing_name' name='thing_name' maxlength='20' value='" + String(cfg.getThingName()) + "'></div>";
  s += F("<div><label for='ap_pwd'>Password for this dashboard (user is <b>admin</b>)</label>");
  s += "<input type='password' id='ap_pwd' name='ap_pwd' maxlength='31' value='" + String(cfg.getApPassword()) + "'></div>";
  s += F("</fieldset>");

  // ---- WiFi ----
  s += F("<fieldset><legend>WiFi</legend>");
  s += F("<div><label for='wifi_ssid'>WiFi SSID</label>");
  s += "<input type='text' id='wifi_ssid' name='wifi_ssid' maxlength='32' value='" + String(cfg.getWifiSSID()) + "'></div>";
  s += F("<div><label for='wifi_pass'>WiFi Password</label>");
  s += "<input type='password' id='wifi_pass' name='wifi_pass' maxlength='64' value='" + String(cfg.getWifiPassword()) + "'></div>";
  s += F("</fieldset>");

  // ---- Location ----
  s += F("<fieldset><legend>Location</legend>");
  s += F("<div><label for='lat'>Latitude (3 decimals, will be public)</label>");
  s += "<input type='number' id='lat' name='lat' min='-180' max='180' step='0.001' value='" + String(cfg.getLatitudeStr()) + "'></div>";
  s += F("<div><label for='lng'>Longitude (3 decimals, will be public)</label>");
  s += "<input type='number' id='lng' name='lng' min='-180' max='180' step='0.001' value='" + String(cfg.getLongitudeStr()) + "'></div>";

  // Timezone selector
  s += F("<div><label for='tz'>Time Zone</label>");
  s += F("<select id='tz' name='tz'>");
  size_t tzCount = sizeof(TZ_VALUES) / TZ_LENGTH;
  for (size_t i = 0; i < tzCount; i++) {
    char tzVal[TZ_LENGTH];
    char tzName[TZ_NAME_LENGTH];
    strncpy_P(tzVal, TZ_VALUES[i], TZ_LENGTH);
    strncpy_P(tzName, TZ_NAMES[i], TZ_NAME_LENGTH);
    bool selected = strcmp(tzVal, cfg.getTZRaw()) == 0;
    s += "<option value='" + String(tzVal) + "'" + (selected ? " selected" : "") + ">" + String(tzName) + "</option>";
  }
  s += F("</select></div>");
  s += F("</fieldset>");

  // ---- MQTT ----
  s += F("<fieldset><legend>MQTT credentials</legend>");
  s += F("<div><label for='mqtt_server'>Server address</label>");
  s += "<input type='text' id='mqtt_server' name='mqtt_server' maxlength='30' value='" + String(cfg.getMqttServer()) + "'></div>";
  s += F("<div><label for='mqtt_port'>Server Port</label>");
  s += "<input type='number' id='mqtt_port' name='mqtt_port' min='0' max='65536' step='1' value='" + String(cfg.getMqttPort()) + "'></div>";
  s += F("<div><label for='mqtt_user'>MQTT Username</label>");
  s += "<input type='text' id='mqtt_user' name='mqtt_user' maxlength='30' value='" + String(cfg.getMqttUser()) + "'></div>";
  s += F("<div><label for='mqtt_pass'>MQTT Password</label>");
  s += "<input type='text' id='mqtt_pass' name='mqtt_pass' maxlength='30' value='" + String(cfg.getMqttPass()) + "'></div>";
  s += F("</fieldset>");

  // ---- Board config ----
  s += F("<fieldset id='Board config'><legend>Board config</legend>");
  s += F("<div><label for='board'>Board type</label>");
  s += F("<select id='board' name='board' onchange=\"document.getElementById('tpl_dirty').value='0'\">");
  for (uint8_t i = 0; i < cfg.getBoardCount(); i++) {
    bool selected = (cfg.getBoard() == i);
    s += "<option value='" + String(i) + "'" + (selected ? " selected" : "") + ">" + cfg.getBoardName(i) + "</option>";
  }
  s += F("</select></div>");

  s += F("<div><label for='oled_bright'>OLED Bright</label>");
  s += "<input type='number' id='oled_bright' name='oled_bright' min='0' max='100' step='1' value='" + String(cfg.getOledBright()) + "'></div>";

  auto addCheckbox = [&](const char* id, const char* label, bool checked) {
    s += "<div><label for='" + String(id) + "'>" + String(label) + "</label>";
    s += "<input type='checkbox' id='" + String(id) + "' name='" + String(id) + "'" + (checked ? " checked" : "") + "></div>";
  };

  addCheckbox("tx", "Enable TX (HAM licence / no preamp)", cfg.getAllowTx());
  addCheckbox("remote_tune", "Allow Automatic Tuning", cfg.getRemoteTune());
  addCheckbox("telemetry3rd", "Allow sending telemetry to third party", cfg.getTelemetry3rd());
  addCheckbox("test", "Test mode", cfg.getTestMode());
  addCheckbox("auto_update", "Automatic Firmware Update", cfg.getAutoUpdate());
  addCheckbox("disable_oled", "Disable OLED display", cfg.getDisableOled());
  addCheckbox("disable_radio", "Disable Radio (dev mode, no LoRa module)", cfg.getDisableRadio());
  addCheckbox("always_ap", "Always keep WiFi AP active (same channel/BW as STA)", cfg.getAlwaysAP());
  s += F("</fieldset>");

  // ---- Network Interface ----
  {
    board_t brd;
    cfg.getBoardConfig(brd);
    ConnectionManager& cm2 = ConnectionManager::getInstance();
    bool ethEffective = brd.ethEN && !cm2.isEthFailed();
    if (ethEffective) {
      // Ethernet hardware present and working: show the selector.
      s += F("<fieldset><legend>Network Interface</legend>");
      s += F("<div><label for='iface_mode'>Interface Mode</label>");
      s += F("<select id='iface_mode' name='iface_mode'>");
      InterfaceMode curMode = cfg.getInterfaceMode();
      s += String("<option value='0'") + (curMode == InterfaceMode::WIFI_ONLY ? " selected" : "") + ">WiFi Only</option>";
      s += String("<option value='1'") + (curMode == InterfaceMode::ETH_ONLY ? " selected" : "") + ">Ethernet Only</option>";
      s += String("<option value='2'") + (curMode == InterfaceMode::BOTH ? " selected" : "") + ">Both (failover)</option>";
      s += F("</select></div>");
      s += F("</fieldset>");
    } else if (brd.ethEN && cm2.isEthFailed()) {
      // Board template enables Ethernet but hardware init failed at runtime.
      // Show the selector so the user can change the setting, but pre-select
      // WiFi Only to reflect the forced runtime behaviour.
      s += F("<fieldset><legend>Network Interface</legend>");
      s += F("<div style='color:#c0392b;font-size:0.88em;margin-bottom:6px'>&#9888; Ethernet hardware init failed &mdash; running as WiFi Only.</div>");
      s += F("<div><label for='iface_mode'>Interface Mode (saved for next boot)</label>");
      s += F("<select id='iface_mode' name='iface_mode'>");
      InterfaceMode curMode = cfg.getInterfaceMode();
      s += String("<option value='0'") + (curMode == InterfaceMode::WIFI_ONLY ? " selected" : "") + ">WiFi Only</option>";
      s += String("<option value='1'") + (curMode == InterfaceMode::ETH_ONLY ? " selected" : "") + ">Ethernet Only</option>";
      s += String("<option value='2'") + (curMode == InterfaceMode::BOTH ? " selected" : "") + ">Both (failover)</option>";
      s += F("</select></div>");
      s += F("</fieldset>");
    } else {
      // No Ethernet in board template — force WiFi Only silently
      s += F("<input type='hidden' name='iface_mode' value='0'>");
    }
  }

  // ---- Advanced ----
  s += F("<fieldset><legend>Advanced Config (do not modify unless you know what you are doing)</legend>");
  s += F("<div><label for='board_template'>Board Template (requires manual restart)</label>");
  s += F("<div style='display:flex;align-items:flex-start;gap:6px'>");
  s += "<textarea id='board_template' name='board_template' maxlength='511' rows='4' style='flex:1;font-family:monospace;font-size:0.85em;resize:vertical' oninput=\"document.getElementById('tpl_dirty').value='1'\">" + String(cfg.getBoardTemplate()) + "</textarea>";
  s += F("<button type='button' onclick='btWzOpen()' style='white-space:nowrap;padding:3px 10px;width:auto;background:#555;color:#fff;font-size:0.82em;border-radius:4px;border:none;cursor:pointer;flex-shrink:0'>&#128295; Wizard</button>");
  s += F("</div></div>");
  // tpl_dirty: starts 1 if a custom template is already stored, 0 otherwise.
  // Changing the board dropdown resets it to 0; editing the textarea or applying
  // the Wizard sets it to 1.  The server only saves the template when it is 1.
  s += String("<input type='hidden' id='tpl_dirty' name='tpl_dirty' value='") + (cfg.getBoardTemplate()[0] ? "1" : "0") + "'>";
  s += F("<div><label for='modem_startup'>Modem startup</label>");
  s += "<input type='text' id='modem_startup' name='modem_startup' maxlength='255' placeholder='' value='" + String(cfg.getModemStartup()) + "'></div>";
  s += F("<div><label for='advanced_config'>Advanced parameters</label>");
  s += "<input type='text' id='advanced_config' name='advanced_config' maxlength='255' placeholder='' value='" + String(cfg.getAdvancedConfig()) + "'></div>";
  s += F("</fieldset>");

  s += F("<br/><button type='submit'>Save</button>");
  s += F("</form>");
  s += FPSTR(BOARD_WIZARD_HTML);
  // Inject board defaults so the Wizard can pre-fill when the template textarea is empty.
  {
    String bds = F("<script>var boardDefaults=[");
    for (uint8_t i = 0; i < cfg.getBoardCount(); i++) {
      const board_t* b = cfg.getBoardDef(i);
      if (!b) continue;
      if (i) bds += ',';
      char buf[512];
      snprintf(buf, sizeof(buf),
        "{aADDR:%u,oSDA:%u,oSCL:%u,oRST:%u,pBut:%u,led:%u,"
        "radio:%u,lNSS:%u,lDIO0:%u,lDIO1:%u,lBUSSY:%u,lRST:%u,"
        "lMISO:%u,lMOSI:%u,lSCK:%u,lTCXOV:%.4f,RXEN:%u,TXEN:%u,lSPI:%u,"
        "ethEN:%s,ethPHY:%u,ethSPI:%u,ethCS:%u,ethINT:%u,ethRST:%u,"
        "ethMISO:%u,ethMOSI:%u,ethSCK:%u,"
        "ethMDC:%u,ethMDIO:%u,ethPhyAddr:%d,ethPhyType:%u,ethRefClk:%u,ethClkExt:%s,ethPhyRST:%u,ethOscEN:%u}",
        b->OLED__address, b->OLED__SDA, b->OLED__SCL, b->OLED__RST,
        b->PROG__BUTTON, b->BOARD_LED,
        b->L_radio, b->L_NSS, b->L_DI00, b->L_DI01, b->L_BUSSY, b->L_RST,
        b->L_MISO, b->L_MOSI, b->L_SCK, b->L_TCXO_V, b->RX_EN, b->TX_EN, b->L_SPI,
        b->ethEN ? "true" : "false", b->ethPHY, b->ethSPI, b->ethCS, b->ethINT, b->ethRST,
        b->ethMISO, b->ethMOSI, b->ethSCK,
        b->ethMDC, b->ethMDIO, b->ethPhyAddr, b->ethPhyType, b->ethRefClk,
        b->ethClkExt ? "true" : "false", b->ethPhyRST, b->ethOscEN);
      bds += buf;
    }
    bds += F("];</script>");
    s += bds;
  }
  s += "<br /><button onclick=\"window.location.href='" + String(ROOT_URL) + "';\">Go Back</button><br /><br />";
  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));
  return s;
}

// =====================================================================
// ---- POST /config ---- (Save configuration)
// =====================================================================
esp_err_t TinyGSWebServer::handleConfigPost(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  TinyGSWebServer* self = (TinyGSWebServer*)req->user_ctx;

  // Read POST body
  int contentLen = req->content_len;
  if (contentLen > 2048) contentLen = 2048;
  char* buf = (char*)malloc(contentLen + 1);
  if (!buf) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  int received = 0;
  while (received < contentLen) {
    int ret = httpd_req_recv(req, buf + received, contentLen - received);
    if (ret <= 0) { free(buf); httpd_resp_send_500(req); return ESP_FAIL; }
    received += ret;
  }
  buf[contentLen] = '\0';

  // Helper lambda: extract form value
  String body(buf);
  free(buf);

  // Full percent-decode of an application/x-www-form-urlencoded value.
  auto urlDecode = [](const String& src) -> String {
    String out;
    out.reserve(src.length());
    for (int i = 0; i < (int)src.length(); i++) {
      char c = src[i];
      if (c == '+') {
        out += ' ';
      } else if (c == '%' && i + 2 < (int)src.length()) {
        char hi = src[i + 1];
        char lo = src[i + 2];
        auto hexVal = [](char h) -> int {
          if (h >= '0' && h <= '9') return h - '0';
          if (h >= 'A' && h <= 'F') return h - 'A' + 10;
          if (h >= 'a' && h <= 'f') return h - 'a' + 10;
          return -1;
        };
        int h = hexVal(hi), l = hexVal(lo);
        if (h >= 0 && l >= 0) {
          out += (char)((h << 4) | l);
          i += 2;
        } else {
          out += c;
        }
      } else {
        out += c;
      }
    }
    return out;
  };

  auto getFormVal = [&body, &urlDecode](const char* key) -> String {
    String search = String(key) + "=";
    int start = body.indexOf(search);
    if (start < 0) return "";
    start += search.length();
    int end = body.indexOf('&', start);
    if (end < 0) end = body.length();
    return urlDecode(body.substring(start, end));
  };

  ConfigStore& cfg = ConfigStore::getInstance();

  // Validate station name
  String name = getFormVal("thing_name");
  if (name.length() >= 3) {
    cfg.setThingName(name.c_str());
  }

  cfg.setApPassword(getFormVal("ap_pwd").c_str());
  String wifiSsid = getFormVal("wifi_ssid");
  String wifiPass = getFormVal("wifi_pass");
  LOG_CONSOLE(PSTR("[Config] Saving WiFi SSID: '%s'  password: '%s'"), wifiSsid.c_str(), wifiPass.c_str());
  cfg.setWifiSSID(wifiSsid.c_str());
  cfg.setWifiPassword(wifiPass.c_str());

  String lat = getFormVal("lat");
  if (lat.length()) cfg.setLat(lat.c_str());
  String lon = getFormVal("lng");
  if (lon.length()) cfg.setLon(lon.c_str());

  String tz = getFormVal("tz");
  if (tz.length()) cfg.setTZ(tz.c_str());

  cfg.setMqttServer(getFormVal("mqtt_server").c_str());
  cfg.setMqttPort(getFormVal("mqtt_port").c_str());
  cfg.setMqttUser(getFormVal("mqtt_user").c_str());
  cfg.setMqttPass(getFormVal("mqtt_pass").c_str());

  cfg.setBoard(getFormVal("board").c_str());
  cfg.setOledBright(getFormVal("oled_bright").c_str());

  // Checkboxes: present in body = checked, absent = unchecked
  cfg.setAllowTx(body.indexOf("tx=") >= 0);
  cfg.setDisableOled(body.indexOf("disable_oled=") >= 0);
  cfg.setDisableRadio(body.indexOf("disable_radio=") >= 0);
  cfg.setAlwaysAP(body.indexOf("always_ap=") >= 0);
  // For the others, we need setters
  // (the ConfigStore already has raw buffer access, add simple setters if needed)

  // Interface mode
  String ifaceStr = getFormVal("iface_mode");
  if (ifaceStr.length()) {
    cfg.setInterfaceMode((InterfaceMode)atoi(ifaceStr.c_str()));
  }

  // Advanced — only persist the board template when the user explicitly edited
  // it (tpl_dirty=1).  Merely changing the board dropdown must not make the
  // firmware think a custom template exists.
  bool tplDirty = getFormVal("tpl_dirty") == "1";
  cfg.setBoardTemplate(tplDirty ? getFormVal("board_template").c_str() : "");
  cfg.setModemStartup(getFormVal("modem_startup").c_str());
  cfg.setAdvancedConfig(getFormVal("advanced_config").c_str());

  cfg.save();

  // Respond with the "restarting" page (200 OK) rather than a 303 redirect.
  // A redirect would cause the browser to immediately open a new connection
  // to GET /, which races with ESP.restart() and results in a hanging request.
  // The restart page shows a message and auto-reloads after 10 s — enough
  // time for the board to restart and be ready to serve again.
  String restartHtml = buildRestartPage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_sendstr(req, restartHtml.c_str());

  // Notify callback (calls ESP.restart() after a short delay)
  if (self->_onConfigSaved) {
    self->_onConfigSaved();
  }

  return ESP_OK;
}

// =====================================================================
// ---- GET /firmware ---- (Upload page)
// =====================================================================
esp_err_t TinyGSWebServer::handleFirmware(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  String s = buildFirmwarePage();
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());
  return ESP_OK;
}

String TinyGSWebServer::buildFirmwarePage() {
  String s = String(FPSTR(HTML_HEAD));
  s += "<style>" + String(FPSTR(HTML_STYLE_INNER)) + "</style>";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(HTML_BODY_INNER);
  s += "<div><img src=\"" + String(LOGO_URL) + "\"></div><br/>";
  s += F("<h3>Firmware Update</h3>");
  s += F("<form method='POST' action='/firmware' enctype='multipart/form-data'>");
  s += F("<input type='file' name='update' accept='.bin'><br/><br/>");
  s += F("<button type='submit'>Upload</button>");
  s += F("</form><br/>");
  s += "<button onclick=\"window.location.href='" + String(ROOT_URL) + "';\">Go Back</button><br /><br />";
  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));
  return s;
}

// =====================================================================
// ---- POST /firmware ---- (OTA upload)
// =====================================================================
esp_err_t TinyGSWebServer::handleFirmwarePost(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  int contentLen = req->content_len;
  char* buf = (char*)malloc(1024);
  if (!buf) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  bool updateStarted = false;
  bool updateError = false;
  int remaining = contentLen;
  bool headerSkipped = false;

  while (remaining > 0) {
    int toRead = remaining > 1024 ? 1024 : remaining;
    int received = httpd_req_recv(req, buf, toRead);
    if (received <= 0) {
      updateError = true;
      break;
    }
    remaining -= received;

    // Skip multipart headers on first chunk
    if (!headerSkipped) {
      // Find the double CRLF that ends the multipart header
      char* bodyStart = strstr(buf, "\r\n\r\n");
      if (bodyStart) {
        bodyStart += 4;
        int headerLen = bodyStart - buf;
        int dataLen = received - headerLen;

        if (!Update.begin(contentLen - headerLen)) {
          updateError = true;
          break;
        }
        updateStarted = true;
        if (Update.write((uint8_t*)bodyStart, dataLen) != (size_t)dataLen) {
          updateError = true;
          break;
        }
        headerSkipped = true;
      }
      continue;
    }

    if (updateStarted) {
      if (Update.write((uint8_t*)buf, received) != (size_t)received) {
        updateError = true;
        break;
      }
    }
  }

  free(buf);

  if (updateStarted && !updateError) {
    if (!Update.end(true)) {
      updateError = true;
    }
  }

  if (updateError) {
    Update.abort();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, "<h3>Update FAILED</h3><a href='/'>Go Back</a>");
    return ESP_OK;
  }

  String s = String(FPSTR(HTML_HEAD));
  s += "<meta http-equiv=\"refresh\" content=\"10; url=/\">";
  s += FPSTR(HTML_HEAD_END);
  s += FPSTR(HTML_BODY_INNER);
  s += F("<h3>Update successful! Rebooting...</h3>");
  s += FPSTR(HTML_END);
  s.replace("{v}", FPSTR(TITLE_TEXT));

  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, s.c_str());

  delay(100);
  ESP.restart();
  return ESP_OK;
}
