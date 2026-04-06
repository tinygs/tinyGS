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
  s += "<div class='home-wrap'>";
  s += "<div class='logo-wrap'><img class='logo' src=\"" + String(LOGO_URL) + "\"></div>";

  if (cfg.getMqttServer()[0] == '\0' || cfg.getMqttUser()[0] == '\0' || cfg.getMqttPass()[0] == '\0') {
    s += F("<div class='otp-box'>");
    s += F("<div style='font-size:0.82rem;color:var(--text2);margin-bottom:0.3rem;'>Device not connected to TinyGS</div>");
    s += "<div class='otp-code'>" + String(mqttCredentials.getOTPCode()) + "</div>";
    s += F("</div>");
  }

  s += "<a class='nav-btn' href='" + String(DASHBOARD_URL) + "'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><rect x='3' y='3' width='7' height='7' rx='1'/><rect x='14' y='3' width='7' height='7' rx='1'/><rect x='3' y='14' width='7' height='7' rx='1'/><rect x='14' y='14' width='7' height='7' rx='1'/></svg>Station Dashboard</a>";
  s += "<a class='nav-btn' href='" + String(CONFIG_URL) + "'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='3'/><path d='M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-4 0v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83-2.83l.06-.06A1.65 1.65 0 004.68 15a1.65 1.65 0 00-1.51-1H3a2 2 0 010-4h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 012.83-2.83l.06.06A1.65 1.65 0 009 4.68a1.65 1.65 0 001-1.51V3a2 2 0 014 0v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 2.83l-.06.06A1.65 1.65 0 0019.4 9a1.65 1.65 0 001.51 1H21a2 2 0 010 4h-.09a1.65 1.65 0 00-1.51 1z'/></svg>Configure Parameters</a>";
  s += "<a class='nav-btn' href='" + String(UPDATE_URL) + "'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4'/><polyline points='17 8 12 3 7 8'/><line x1='12' y1='3' x2='12' y2='15'/></svg>Upload Firmware</a>";
  s += "<a class='nav-btn' href='" + String(RESTART_URL) + "'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><polyline points='23 4 23 10 17 10'/><path d='M20.49 15a9 9 0 11-2.12-9.36L23 10'/></svg>Restart Station</a>";

  if (cfg.getThingName()[0] == 'M' && cfg.getThingName()[2] == ' ' && cfg.getMqttPass()[0] == '\0') {
    s += F("<div class='otp-box'>");
    s += F("<div style='font-size:0.82rem;color:var(--text2);margin-bottom:0.3rem;'>OTP Code</div>");
    s += "<div class='otp-code'><a href='https://tinygs.com/user/addstation'>" + String(mqttCredentials.getOTPCode()) + "</a></div>";
    s += F("</div>");
    s += F("<div style='background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:0.8rem;margin:0.5rem 0;'>");
    s += F("<div style='font-size:0.8rem;color:var(--text2);margin-bottom:0.4rem;'>Default dashboard credentials</div>");
    s += F("<table class='info-table'>");
    s += F("<tr><td>User:</td><td>admin</td></tr>");
    s += "<tr><td>Password:</td><td>" + String(cfg.getApPassword()) + "</td></tr>";
    s += F("</table></div>");
  }

  s += "</div>";
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
  s += "<div class='logo-wrap'><img class='logo' src=\"" + String(LOGO_URL) + "\"></div>";
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
  s += "<div class='logo-wrap'><img class='logo' src=\"" + String(LOGO_URL) + "\"></div>";
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
// Streamed via HTTP chunked transfer to avoid allocating the full page
// (~100 KB with 460-entry TZ dropdown) in a single heap block, which
// would fragment the ESP32 heap and lose WiFi connectivity.
// =====================================================================

// Send a PROGMEM string in small stack-buffered pieces; no heap needed.
static void sendPgmChunked(httpd_req_t* req, PGM_P pgm) {
  char buf[256];
  size_t total = strlen_P(pgm);
  size_t sent  = 0;
  while (sent < total) {
    size_t n = (total - sent < 255) ? (total - sent) : 255;
    memcpy_P(buf, pgm + sent, n);
    httpd_resp_send_chunk(req, buf, (ssize_t)n);
    sent += n;
  }
}

esp_err_t TinyGSWebServer::handleConfig(httpd_req_t* req) {
  if (!authenticate(req)) return sendAuthRequired(req);

  ConfigStore& cfg = ConfigStore::getInstance();
  httpd_resp_set_type(req, "text/html");

  // Helper: send an Arduino String as one chunk.
  auto sc = [&](const String& chunk) {
    if (chunk.length())
      httpd_resp_send_chunk(req, chunk.c_str(), (ssize_t)chunk.length());
  };

  // ---- HEAD (styles + scripts) ----
  {
    String s;
    s.reserve(4096);
    s += FPSTR(HTML_HEAD);
    s += F("<style>");
    s += FPSTR(HTML_STYLE_INNER);
    s += F("</style><style>");
    s += FPSTR(IOTWEBCONF_CONFIG_STYLE_INNER);
    s += F("</style><script>");
    s += FPSTR(ADVANCED_CONFIG_SCRIPT);
    s += F("</script>");
    s += FPSTR(HTML_HEAD_END);
    s += FPSTR(HTML_BODY_INNER);
    s += F("<div class='logo-wrap'><img class='logo' src=\"");
    s += LOGO_URL;
    s += F("\"></div><form action=\"");
    s += CONFIG_URL;
    s += F("\" method=\"post\">");
    s.replace("{v}", FPSTR(TITLE_TEXT));
    sc(s);
  }

  // ---- Station identity ----
  {
    String s;
    s.reserve(512);
    s += F("<fieldset><legend>Station</legend>");
    s += F("<div><label for='thing_name'>GroundStation Name (will be seen on the map)</label>");
    s += F("<input type='text' id='thing_name' name='thing_name' maxlength='20' value='");
    s += cfg.getThingName();
    s += F("'></div>");
    s += F("<div><label for='ap_pwd'>Password for this dashboard (user is <b>admin</b>)</label>");
    s += F("<input type='password' id='ap_pwd' name='ap_pwd' maxlength='31' value='");
    s += cfg.getApPassword();
    s += F("'></div></fieldset>");
    sc(s);
  }

  // ---- WiFi ----
  {
    String s;
    s.reserve(512);
    s += F("<fieldset><legend>WiFi</legend>");
    s += F("<div><label for='wifi_ssid'>WiFi SSID</label>");
    s += F("<input type='text' id='wifi_ssid' name='wifi_ssid' maxlength='32' value='");
    s += cfg.getWifiSSID();
    s += F("'></div>");
    s += F("<div><label for='wifi_pass'>WiFi Password</label>");
    s += F("<input type='password' id='wifi_pass' name='wifi_pass' maxlength='64' value='");
    s += cfg.getWifiPassword();
    s += F("'></div></fieldset>");
    sc(s);
  }

  // ---- Location: lat/lng ----
  {
    String s;
    s.reserve(512);
    s += F("<fieldset><legend>Location</legend>");
    s += F("<div><label for='lat'>Latitude (3 decimals, will be public)</label>");
    s += F("<input type='number' id='lat' name='lat' min='-180' max='180' step='0.001' value='");
    s += cfg.getLatitudeStr();
    s += F("'></div>");
    s += F("<div><label for='lng'>Longitude (3 decimals, will be public)</label>");
    s += F("<input type='number' id='lng' name='lng' min='-180' max='180' step='0.001' value='");
    s += cfg.getLongitudeStr();
    s += F("'></div>");
    sc(s);
  }

  // ---- Timezone selector — streamed in ~2 KB batches to avoid large alloc ----
  {
    static const char TZ_OPEN[] = "<div><label for='tz'>Time Zone</label><select id='tz' name='tz'>";
    httpd_resp_send_chunk(req, TZ_OPEN, strlen(TZ_OPEN));

    const size_t tzCount = sizeof(TZ_VALUES) / TZ_LENGTH;
    String batch;
    batch.reserve(2048);
    for (size_t i = 0; i < tzCount; i++) {
      char tzVal[TZ_LENGTH];
      char tzName[TZ_NAME_LENGTH];
      strncpy_P(tzVal,  TZ_VALUES[i], TZ_LENGTH);
      strncpy_P(tzName, TZ_NAMES[i],  TZ_NAME_LENGTH);
      tzVal[TZ_LENGTH - 1]   = '\0';
      tzName[TZ_NAME_LENGTH - 1] = '\0';
      bool selected = strcmp(tzVal, cfg.getTZRaw()) == 0;
      batch += F("<option value='");
      batch += tzVal;
      batch += '\'';
      if (selected) batch += F(" selected");
      batch += '>';
      batch += tzName;
      batch += F("</option>");
      if (batch.length() > 1800 || i == tzCount - 1) {
        sc(batch);
        batch = "";
      }
    }
    static const char TZ_CLOSE[] = "</select></div></fieldset>";
    httpd_resp_send_chunk(req, TZ_CLOSE, strlen(TZ_CLOSE));
  }

  // ---- MQTT ----
  {
    String s;
    s.reserve(1024);
    s += F("<fieldset><legend>MQTT credentials (<a href='https://t.me/+VlqGIoyJ8SmgJuWe' target='_blank'>Join this group</a>)<br>Then open a private chat with <a href='https://t.me/tinygs_personal_bot' target='_blank'>@tinygs_personal_bot</a> and ask &#47;mqtt</legend>");
    s += F("<div><label for='mqtt_server'>Server address</label>");
    s += F("<input type='text' id='mqtt_server' name='mqtt_server' maxlength='30' value='");
    s += cfg.getMqttServer();
    s += F("'></div>");
    s += F("<div><label for='mqtt_port'>Server Port</label>");
    s += F("<input type='number' id='mqtt_port' name='mqtt_port' min='0' max='65536' step='1' value='");
    s += cfg.getMqttPort();
    s += F("'></div>");
    s += F("<div><label for='mqtt_user'>MQTT Username</label>");
    s += F("<input type='text' id='mqtt_user' name='mqtt_user' maxlength='30' value='");
    s += cfg.getMqttUser();
    s += F("'></div>");
    s += F("<div><label for='mqtt_pass'>MQTT Password</label>");
    s += F("<input type='text' id='mqtt_pass' name='mqtt_pass' maxlength='30' value='");
    s += cfg.getMqttPass();
    s += F("'></div></fieldset>");
    sc(s);
  }

  // ---- Board config ----
  {
    String s;
    s.reserve(1024);
    s += F("<fieldset id='Board config'><legend>Board config</legend>");
    s += F("<div><label for='board'>Board type</label>");
    s += F("<select id='board' name='board' onchange=\"document.getElementById('tpl_dirty').value='0'\">");
    for (uint8_t i = 0; i < cfg.getBoardCount(); i++) {
      bool selected = (cfg.getBoard() == i);
      s += F("<option value='");
      s += i;
      s += '\'';
      if (selected) s += F(" selected");
      s += '>';
      s += cfg.getBoardName(i);
      s += F("</option>");
    }
    s += F("</select></div>");
    s += F("<div><label for='oled_bright'>OLED Bright</label>");
    s += F("<input type='number' id='oled_bright' name='oled_bright' min='0' max='100' step='1' value='");
    s += cfg.getOledBright();
    s += F("'></div>");

    auto addCheckbox = [&](const char* id, const char* lbl, bool checked) {
      s += F("<div><label for='");
      s += id;
      s += F("'>");
      s += lbl;
      s += F("</label><input type='checkbox' id='");
      s += id;
      s += F("' name='");
      s += id;
      s += '\'';
      if (checked) s += F(" checked");
      s += F("></div>");
    };
    addCheckbox("tx",           "Enable TX (HAM licence / no preamp)",              cfg.getAllowTx());
    addCheckbox("remote_tune",  "Allow Automatic Tuning",                           cfg.getRemoteTune());
    addCheckbox("telemetry3rd", "Allow sending telemetry to third party",           cfg.getTelemetry3rd());
    addCheckbox("test",         "Test mode",                                        cfg.getTestMode());
    addCheckbox("auto_update",  "Automatic Firmware Update",                        cfg.getAutoUpdate());
    addCheckbox("disable_oled", "Disable OLED display",                             cfg.getDisableOled());
    addCheckbox("disable_radio","Disable Radio (dev mode, no LoRa module)",         cfg.getDisableRadio());
    addCheckbox("always_ap",    "Always keep WiFi AP active (same channel/BW as STA)", cfg.getAlwaysAP());
    s += F("</fieldset>");
    sc(s);
  }

  // ---- Network Interface ----
  {
    board_t brd;
    cfg.getBoardConfig(brd);
    ConnectionManager& cm2 = ConnectionManager::getInstance();
    String s;
    s.reserve(512);
    if (brd.ethEN) {
      bool failed = cm2.isEthFailed();
      s += F("<fieldset><legend>Network Interface</legend>");
      if (failed)
        s += F("<div style='color:#c0392b;font-size:0.88em;margin-bottom:6px'>&#9888; Ethernet hardware init failed &mdash; running as WiFi Only.</div>");
      s += failed
           ? F("<div><label for='iface_mode'>Interface Mode (saved for next boot)</label>")
           : F("<div><label for='iface_mode'>Interface Mode</label>");
      s += F("<select id='iface_mode' name='iface_mode'>");
      InterfaceMode curMode = cfg.getInterfaceMode();
      s += String("<option value='0'") + (curMode == InterfaceMode::WIFI_ONLY ? " selected" : "") + ">WiFi Only</option>";
      s += String("<option value='1'") + (curMode == InterfaceMode::ETH_ONLY  ? " selected" : "") + ">Ethernet Only</option>";
      s += String("<option value='2'") + (curMode == InterfaceMode::BOTH      ? " selected" : "") + ">Both (failover)</option>";
      s += F("</select></div></fieldset>");
    } else {
      // No Ethernet in board template — force WiFi Only silently.
      s += F("<input type='hidden' name='iface_mode' value='0'>");
    }
    sc(s);
  }

  // ---- Advanced Config ----
  {
    String s;
    s.reserve(1024);
    s += F("<fieldset><legend>Advanced Config (do not modify unless you know what you are doing)</legend>");
    s += F("<div><label for='board_template'>Board Template (requires manual restart)</label>");
    s += F("<div style='display:flex;align-items:flex-start;gap:6px'>");
    s += F("<textarea id='board_template' name='board_template' maxlength='511' rows='4'"
           " style='flex:1;font-family:monospace;font-size:0.85em;resize:vertical'"
           " oninput=\"document.getElementById('tpl_dirty').value='1'\">");
    s += cfg.getBoardTemplate();
    s += F("</textarea>");
    s += F("<button type='button' onclick='btWzOpen()' style='white-space:nowrap;padding:3px 10px;"
           "width:auto;background:#555;color:#fff;font-size:0.82em;border-radius:4px;"
           "border:none;cursor:pointer;flex-shrink:0'>&#128295; Wizard</button>");
    s += F("</div></div>");
    // tpl_dirty: 1 if a custom template is stored; 0 when using a preset board.
    s += String("<input type='hidden' id='tpl_dirty' name='tpl_dirty' value='")
         + (cfg.getBoardTemplate()[0] ? "1" : "0") + "'>";
    s += F("<div><label for='modem_startup'>Modem startup</label>");
    s += F("<input type='text' id='modem_startup' name='modem_startup' maxlength='255' placeholder='' value='");
    s += cfg.getModemStartup();
    s += F("'></div>");
    s += F("<div><label for='advanced_config'>Advanced parameters</label>");
    s += F("<input type='text' id='advanced_config' name='advanced_config' maxlength='255' placeholder='' value='");
    s += cfg.getAdvancedConfig();
    s += F("'></div></fieldset>");
    s += F("<br/><button type='submit'>Save</button></form>");
    sc(s);
  }

  // ---- Board Wizard HTML (large PROGMEM block — sent without heap copy) ----
  sendPgmChunked(req, (PGM_P)BOARD_WIZARD_HTML);

  // ---- Board Defaults script (one entry per chunk — no large String) ----
  {
    static const char BD_OPEN[] = "<script>var boardDefaults=[";
    httpd_resp_send_chunk(req, BD_OPEN, strlen(BD_OPEN));
    bool first = true;
    for (uint8_t i = 0; i < cfg.getBoardCount(); i++) {
      const board_t* b = cfg.getBoardDef(i);
      if (!b) continue;
      char buf[600];
      size_t off = 0;
      if (!first) { buf[0] = ','; off = 1; }
      first = false;
      snprintf(buf + off, sizeof(buf) - off,
        "{aADDR:%u,oSDA:%u,oSCL:%u,oRST:%u,pBut:%u,led:%u,"
        "radio:%u,lNSS:%u,lDIO0:%u,lDIO1:%u,lBUSSY:%u,lRST:%u,"
        "lMISO:%u,lMOSI:%u,lSCK:%u,lTCXOV:%.4f,RXEN:%u,TXEN:%u,lSPI:%u,"
        "ethEN:%s,ethPHY:%u,ethSPI:%u,ethCS:%u,ethINT:%u,ethRST:%u,"
        "ethMISO:%u,ethMOSI:%u,ethSCK:%u,"
        "ethMDC:%u,ethMDIO:%u,ethPhyAddr:%d,ethPhyType:%u,ethRefClk:%u,"
        "ethClkExt:%s,ethPhyRST:%u,ethOscEN:%u}",
        b->OLED__address, b->OLED__SDA, b->OLED__SCL, b->OLED__RST,
        b->PROG__BUTTON, b->BOARD_LED,
        b->L_radio, b->L_NSS, b->L_DI00, b->L_DI01, b->L_BUSSY, b->L_RST,
        b->L_MISO, b->L_MOSI, b->L_SCK, b->L_TCXO_V, b->RX_EN, b->TX_EN, b->L_SPI,
        b->ethEN ? "true" : "false", b->ethPHY, b->ethSPI, b->ethCS, b->ethINT, b->ethRST,
        b->ethMISO, b->ethMOSI, b->ethSCK,
        b->ethMDC, b->ethMDIO, b->ethPhyAddr, b->ethPhyType, b->ethRefClk,
        b->ethClkExt ? "true" : "false", b->ethPhyRST, b->ethOscEN);
      httpd_resp_send_chunk(req, buf, (ssize_t)strlen(buf));
    }
    static const char BD_CLOSE[] = "];</script>";
    httpd_resp_send_chunk(req, BD_CLOSE, strlen(BD_CLOSE));
  }

  // ---- Footer ----
  {
    String s;
    s += F("<br /><button onclick=\"window.location.href='");
    s += ROOT_URL;
    s += F("';\">Go Back</button><br /><br />");
    s += FPSTR(HTML_END);
    sc(s);
  }

  // Terminate HTTP chunked response.
  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
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
  s += "<div class='logo-wrap'><img class='logo' src=\"" + String(LOGO_URL) + "\"></div>";
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
