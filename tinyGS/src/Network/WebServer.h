/*
  WebServer.h - Native ESP-IDF HTTP server replacing IotWebConf2 web UI

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

#ifndef TINYGS_WEBSERVER_H
#define TINYGS_WEBSERVER_H

#include <Arduino.h>
#include <esp_http_server.h>
#include "ConfigStore.h"
#include "ConnectionManager.h"
#include "INetworkAware.h"
#include "../Status.h"
#include "logos.h"
#include "html.h"
#include "../Display/graphics.h"

extern Status status;

// ---- URL paths ----
constexpr auto ROOT_URL             = "/";
constexpr auto LOGO_URL             = "/logo.png";
constexpr auto CONFIG_URL           = "/config";
constexpr auto DASHBOARD_URL        = "/dashboard";
constexpr auto UPDATE_URL           = "/firmware";
constexpr auto RESTART_URL          = "/restart";
constexpr auto REFRESH_CONSOLE_URL  = "/cs";
constexpr auto REFRESH_WORLDMAP_URL = "/wm";

constexpr auto ADMIN_USER  = "admin";

const char TITLE_TEXT[] PROGMEM = "TinyGS Configuration";

// ---- HTML fragments (replacing IotWebConf2 constants) ----
const char HTML_HEAD[] PROGMEM           = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>\n";
const char HTML_STYLE_INNER[] PROGMEM    = ".de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input,select{padding:5px;font-size:1em;} input{width:95%;} select{width:100%} input[type=checkbox]{width:auto;scale:1.5;margin:10px;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}\n";
const char HTML_HEAD_END[] PROGMEM       = "</head><body>";
const char HTML_BODY_INNER[] PROGMEM     = "<div style='text-align:left;display:inline-block;min-width:260px;'>\n";
const char HTML_END[] PROGMEM            = "</div></body></html>";

class TinyGSWebServer : public INetworkAware {
public:
  static TinyGSWebServer& getInstance() {
    static TinyGSWebServer instance;
    return instance;
  }

  void begin();
  void stop();

  bool askedWebLogin() {
    bool asked = _askForWeblogin;
    _askForWeblogin = false;
    return asked;
  }

  // Callback to be set: called when config is saved from web
  using ConfigSavedCallback = std::function<void()>;
  void setConfigSavedCallback(ConfigSavedCallback cb) { _onConfigSaved = cb; }

  // INetworkAware
  void onConnected(IPAddress ip, ActiveInterface iface) override;
  void onDisconnected() override;
  void onInterfaceChanged(ActiveInterface iface, IPAddress ip) override;

private:
  TinyGSWebServer();
  httpd_handle_t _server = nullptr;
  bool _askForWeblogin = false;
  ConfigSavedCallback _onConfigSaved = nullptr;

  void registerHandlers();

  // ---- HTTP handler callbacks (static for esp_http_server C API) ----
  static esp_err_t handleRoot(httpd_req_t* req);
  static esp_err_t handleLogo(httpd_req_t* req);
  static esp_err_t handleConfig(httpd_req_t* req);
  static esp_err_t handleConfigPost(httpd_req_t* req);
  static esp_err_t handleDashboard(httpd_req_t* req);
  static esp_err_t handleRefreshConsole(httpd_req_t* req);
  static esp_err_t handleRefreshWorldmap(httpd_req_t* req);
  static esp_err_t handleRestart(httpd_req_t* req);
  static esp_err_t handleFirmware(httpd_req_t* req);
  static esp_err_t handleFirmwarePost(httpd_req_t* req);

  // ---- Auth helper ----
  static bool authenticate(httpd_req_t* req);
  static esp_err_t sendAuthRequired(httpd_req_t* req);

  // ---- HTML builders ----
  static String buildRootPage();
  static String buildDashboardPage();
  static String buildConfigPage();
  static String buildRestartPage();
  static String buildFirmwarePage();
  static String buildWorldMapSVG();
};

#endif // TINYGS_WEBSERVER_H
