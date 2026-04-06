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
const char HTML_HEAD[] PROGMEM           =
    "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><meta charset=\"UTF-8\"/><title>{v}</title>"
    "<script>(function(){if(localStorage.getItem('tgs-theme')==='dark')"
    "document.documentElement.classList.add('dark');})();"
    "document.addEventListener('DOMContentLoaded',function(){"
    "var b=document.createElement('button');"
    "b.id='tt';b.title='Toggle theme';"
    "var dk=document.documentElement.classList.contains('dark');"
    "b.innerHTML=dk?'&#9728;':'&#127769;';"
    "b.onclick=function(){var d=document.documentElement.classList.toggle('dark');"
    "b.innerHTML=d?'&#9728;':'&#127769;';localStorage.setItem('tgs-theme',d?'dark':'light');};"
    "document.body.appendChild(b);"
    "});</script>\n";
const char HTML_STYLE_INNER[] PROGMEM    =
    ":root{--bg:#f8fafc;--surface:#fff;--surface2:#f1f5f9;--border:#cbd5e1;--"
    "accent:#3b82f6;--accent2:#0891b2;--accent-glow:rgba(59,130,246,0.12);--"
    "text:#0f172a;--text2:#475569;--danger:#dc2626;--success:#16a34a;--warning:"
    "#d97706;--radius:0.625rem;--font:system-ui,-apple-system,"
    "BlinkMacSystemFont,'Segoe UI',sans-serif;--map-bg:#dbeafe;--map-land:#1d4ed8;--map-border:#93c5fd}"
    "html.dark{--bg:#0b0e17;--surface:#131827;--surface2:#1a2035;--border:#"
    "2a3050;--accent2:#06b6d4;--accent-glow:rgba(59,130,246,0.15);--text:#"
    "e2e8f0;--text2:#94a3b8;--danger:#ef4444;--success:#22c55e;--warning:#"
    "f59e0b;--map-bg:#0b0e17;--map-land:#1e3a5f;--map-border:#2a3050}"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:var(--bg);color:var(--text);font-family:var(--font);font-"
    "size:0.938rem;line-height:1.6;min-height:100vh;text-align:center}"
    ".de{background-color:rgba(239,68,68,0.15);border:1px solid "
    "var(--danger);border-radius:var(--radius)}"
    ".em{font-size:0.75rem;color:var(--danger);padding:2px 0 0;min-height:0}"
    "div,input,select{padding:5px;font-size:0.938rem}"
    "input,select,textarea{background:var(--surface);color:var(--text);border:"
    "1px solid var(--border);border-radius:var(--radius);padding:0.55rem "
    "0.75rem;font-size:0.875rem;font-family:var(--font);transition:border-"
    "color .2s,box-shadow .2s;outline:none;width:100%}"
    "input:focus,select:focus,textarea:focus{border-color:var(--accent);box-"
    "shadow:0 0 0 3px var(--accent-glow)}"
    "input[type=checkbox]{width:auto;height:1.15rem;width:1.15rem;accent-color:"
    "var(--accent);margin:0 8px 0 0;cursor:pointer;vertical-align:middle}"
    "select{cursor:pointer;appearance:auto}"
    "label{display:block;color:var(--text2);font-size:0.8rem;font-weight:500;"
    "margin-bottom:3px;text-align:left;letter-spacing:0.02em}"
    "button,input[type=button],input[type=submit]{background:linear-gradient("
    "135deg,var(--accent),var(--accent2));color:#fff;border:none;border-radius:"
    "var(--radius);padding:0.7rem 1.5rem;font-size:1rem;font-weight:600;"
    "cursor:pointer;transition:opacity .2s,transform .1s;width:100%;letter-spacing:0.02em}"
    "button:hover,input[type=button]:hover{opacity:0.9;transform:translateY(-1px)}"
    "button:active,input[type=button]:active{transform:translateY(0)}"
    "fieldset{background:var(--surface);border:1px solid "
    "var(--border);border-radius:var(--radius);padding:1.25rem 1rem "
    "1rem;margin:0.75rem 0}"
    "legend{color:var(--accent2);font-weight:600;font-size:0.9rem;padding:0 "
    "0.5rem;letter-spacing:0.03em}"
    "a{color:var(--accent);text-decoration:none;transition:color "
    ".2s}a:hover{color:var(--accent2)}"
    ".G{color:var(--success);font-weight:600}.R{color:var(--danger);font-weight:600}"
    ".logo-wrap{text-align:center;margin-bottom:1rem}"
    ".logo-wrap img{max-width:160px;filter:drop-shadow(0 0 20px rgba(59,130,246,0.3));transition:filter .3s}"
    ".logo{max-width:160px;transition:filter .3s}"
    "html.dark .logo-wrap img,html.dark .logo{filter:invert(1) drop-shadow(0 0 15px rgba(59,130,246,0.45))}"
    "#tt{position:fixed;top:.75rem;right:.75rem;z-index:9999;width:auto!"
    "important;padding:.35rem .7rem;background:var(--surface2);color:var(--text2);"
    "border:1px solid var(--border);border-radius:var(--radius);font-size:1.05rem;"
    "cursor:pointer;line-height:1;transition:all .2s;}"
    "#tt:hover{transform:none;opacity:.8;}"
    ".home-wrap{max-width:420px;margin:0 auto;padding:2rem 1rem;text-align:center;}"
    ".nav-btn{display:block;width:100%;margin:0.5rem 0;padding:0.75rem 1rem;"
    "background:var(--surface);color:var(--text);border:1px solid var(--border);"
    "border-radius:var(--radius);font-size:0.95rem;font-weight:500;cursor:pointer;"
    "transition:all .2s;text-align:center;text-decoration:none;box-sizing:border-box;}"
    ".nav-btn:hover{border-color:var(--accent);background:var(--surface2);color:var(--accent);transform:translateY(-1px);}"
    ".nav-btn svg{vertical-align:middle;margin-right:0.5rem;}"
    ".otp-box{background:rgba(59,130,246,0.08);border:1px solid var(--accent);"
    "border-radius:var(--radius);padding:0.8rem;margin:1rem 0;}"
    ".otp-code{font-size:1.3rem;font-weight:700;color:var(--accent);letter-spacing:0.1em;}"
    ".info-table{width:100%;margin:0.5rem 0;text-align:left;border-collapse:collapse;}"
    ".info-table td{padding:0.3rem 0.5rem;font-size:0.85rem;}"
    ".info-table td:first-child{color:var(--text2);}"
    ".info-table td:last-child{font-weight:600;}\n";
const char HTML_HEAD_END[] PROGMEM       = "</head><body>";
const char HTML_BODY_INNER[] PROGMEM     = "<div style='text-align:left;display:inline-block;min-width:260px;max-width:580px;width:90%;padding:1.5rem 0;'>\n";
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
