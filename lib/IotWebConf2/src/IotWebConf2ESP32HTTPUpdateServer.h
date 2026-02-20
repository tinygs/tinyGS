/**
 * IotWebConfESP32HTTPUpdateServer.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 * Notes on IotWebConfESP32HTTPUpdateServer:
 * ESP32 doesn't implement a HTTPUpdateServer. However it seams, that to code
 * from ESP8266 covers nearly all the same functionality.
 * So we need to implement our own HTTPUpdateServer for ESP32, and code is
 * reused from
 * https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPUpdateServer/src/
 * version: 41de43a26381d7c9d29ce879dd5d7c027528371b
 */
#ifdef ESP32

#ifndef __HTTP_UPDATE_SERVER_H
#define __HTTP_UPDATE_SERVER_H

#include <StreamString.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define emptyString F("")

class WebServer;

class HTTPUpdateServer
{
public:
  HTTPUpdateServer(bool serial_debug = false)
  {
    _serial_output = serial_debug;
    _server = NULL;
    _username = emptyString;
    _password = emptyString;
    _authenticated = false;
  }

  void setup(WebServer* server) { setup(server, emptyString, emptyString); }

  void setup(WebServer* server, const String& path)
  {
    setup(server, path, emptyString, emptyString);
  }

  void setup(WebServer* server, const String& username, const String& password)
  {
    setup(server, "/update", username, password);
  }

  void setup(
      WebServer* server, const String& path, const String& username,
      const String& password)
  {
    _server = server;
    _username = username;
    _password = password;

    // handler for the /update form page
    _server->on(
        path.c_str(), HTTP_GET,
        [&]()
        {
          if (_username != emptyString && _password != emptyString &&
              !_server->authenticate(_username.c_str(), _password.c_str()))
            return _server->requestAuthentication();
          _server->send_P(200, PSTR("text/html"), serverIndex);
        });

    // handler for the /update form POST (once file upload finishes)
    _server->on(
        path.c_str(), HTTP_POST,
        [&]()
        {
          if (!_authenticated)
            return _server->requestAuthentication();
          if (Update.hasError())
          {
            _server->send(
                200, F("text/html"),
                String(F("Update error: ")) + _updaterError);
          }
          else
          {
            _server->client().setNoDelay(true);
            _server->send_P(200, PSTR("text/html"), successResponse);
            delay(100);
            _server->client().stop();
            ESP.restart();
          }
        },
        [&]()
        {
          // handler for the file upload, get's the sketch bytes, and writes
          // them through the Update object
          HTTPUpload& upload = _server->upload();

          if (upload.status == UPLOAD_FILE_START)
          {
            _updaterError = String();
            if (_serial_output)
              Serial.setDebugOutput(true);

            _authenticated =
                (_username == emptyString || _password == emptyString ||
                 _server->authenticate(_username.c_str(), _password.c_str()));
            if (!_authenticated)
            {
              if (_serial_output)
                Serial.printf("Unauthenticated Update\n");
              return;
            }

            ///        WiFiUDP::stopAll();
            if (_serial_output)
              Serial.printf("Update: %s\n", upload.filename.c_str());
            ///        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() -
            ///        0x1000) & 0xFFFFF000;
            ///        if(!Update.begin(maxSketchSpace)){//start with max
            ///        available size
            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            { // start with max available size
              _setUpdaterError();
            }
          }
          else if (
              _authenticated && upload.status == UPLOAD_FILE_WRITE &&
              !_updaterError.length())
          {
            if (_serial_output)
              Serial.printf(".");
            if (Update.write(upload.buf, upload.currentSize) !=
                upload.currentSize)
            {
              _setUpdaterError();
            }
          }
          else if (
              _authenticated && upload.status == UPLOAD_FILE_END &&
              !_updaterError.length())
          {
            if (Update.end(true))
            { // true to set the size to the current progress
              if (_serial_output)
                Serial.printf(
                    "Update Success: %u\nRebooting...\n", upload.totalSize);
            }
            else
            {
              _setUpdaterError();
            }
            if (_serial_output)
              Serial.setDebugOutput(false);
          }
          else if (_authenticated && upload.status == UPLOAD_FILE_ABORTED)
          {
            Update.end();
            if (_serial_output)
              Serial.println("Update was aborted");
          }
          delay(0);
        });
  }

  void updateCredentials(const String& username, const String& password)
  {
    _username = username;
    _password = password;
  }

protected:
  void _setUpdaterError()
  {
    if (_serial_output)
      Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    _updaterError = str.c_str();
  }

private:
  bool _serial_output;
  WebServer* _server;
  String _username;
  String _password;
  bool _authenticated;
  String _updaterError;
  const char* serverIndex PROGMEM =
      R"HTML(<!DOCTYPE html><html lang="en"><head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
<meta charset="UTF-8"/><title>TinyGS &mdash; Upload Firmware</title>
<script>(function(){if(localStorage.getItem('tgs-theme')==='dark')document.documentElement.classList.add('dark');})();
document.addEventListener('DOMContentLoaded',function(){
var b=document.createElement('button');b.id='tt';b.title='Toggle theme';
var dk=document.documentElement.classList.contains('dark');
b.innerHTML=dk?'&#9728;':'&#127769;';
b.onclick=function(){var d=document.documentElement.classList.toggle('dark');b.innerHTML=d?'&#9728;':'&#127769;';localStorage.setItem('tgs-theme',d?'dark':'light');};
document.body.appendChild(b);});
function pickFile(){document.getElementById('fw').click();}
</script>
<style>
:root{--bg:#f8fafc;--surface:#fff;--surface2:#f1f5f9;--border:#cbd5e1;--accent:#3b82f6;--accent2:#0891b2;--accent-glow:rgba(59,130,246,0.12);--text:#0f172a;--text2:#475569;--danger:#dc2626;--success:#16a34a;--radius:0.625rem;--font:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif}
html.dark{--bg:#0b0e17;--surface:#131827;--surface2:#1a2035;--border:#2a3050;--accent2:#06b6d4;--accent-glow:rgba(59,130,246,0.15);--text:#e2e8f0;--text2:#94a3b8;--danger:#ef4444;--success:#22c55e}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:var(--font);font-size:0.938rem;line-height:1.6;min-height:100vh;display:flex;align-items:center;justify-content:center}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:2rem;width:100%;max-width:420px;text-align:center;box-shadow:0 4px 24px rgba(0,0,0,0.06)}
h2{font-size:1.1rem;font-weight:700;color:var(--text);margin-bottom:0.25rem}
p{font-size:0.85rem;color:var(--text2);margin-bottom:1.5rem}
.file-wrap{display:block;border:2px dashed var(--border);border-radius:var(--radius);padding:2rem 1rem;margin-bottom:1.25rem;cursor:pointer;transition:border-color .2s}
.file-wrap:hover{border-color:var(--accent)}
.file-label{font-size:0.85rem;color:var(--text2)}
.file-label strong{display:block;font-size:1rem;color:var(--accent);margin-bottom:0.25rem}
input[type=submit]{display:block;width:100%;padding:0.75rem;background:linear-gradient(135deg,var(--accent),var(--accent2));color:#fff;border:none;border-radius:var(--radius);font-size:1rem;font-weight:600;cursor:pointer;transition:opacity .2s}
input[type=submit]:hover{opacity:0.88}
a{color:var(--accent);text-decoration:none;font-size:0.85rem}
a:hover{color:var(--accent2)}
.logo{max-width:120px;margin:0 auto 1.5rem;display:block;transition:filter .3s}
html.dark .logo{filter:invert(1) drop-shadow(0 0 12px rgba(59,130,246,0.45))}
#tt{position:fixed;top:.75rem;right:.75rem;z-index:9999;background:var(--surface2);color:var(--text2);border:1px solid var(--border);border-radius:var(--radius);font-size:1.05rem;cursor:pointer;padding:.35rem .7rem;line-height:1;transition:all .2s}
#tt:hover{opacity:.8}
#fname{font-size:0.82rem;color:var(--accent);margin-top:0.4rem;min-height:1.2em}
</style></head><body>
<div class="card">
  <img class="logo" src="/logo.png" alt="TinyGS">
  <h2>Upload Firmware</h2>
  <p>Select a <code>.bin</code> firmware file and press <strong>Flash</strong>.</p>
  <form method="POST" action="" enctype="multipart/form-data" onsubmit="document.getElementById('sb').disabled=true;">
    <div class="file-wrap" onclick="pickFile()">
      <input type="file" id="fw" name="update" accept=".bin" style="display:none" onchange="document.getElementById('fname').textContent=this.files[0]?this.files[0].name:''">
      <div class="file-label"><strong>&#128190; Choose file</strong><div id="fname"></div></div>
    </div>
    <input type="submit" id="sb" value="Flash">
  </form>
  <br><a href="/">&larr; Back to home</a>
</div>
</body></html>)HTML";
  const char* successResponse PROGMEM =
      R"HTML(<!DOCTYPE html><html lang="en"><head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
<meta charset="UTF-8"/><title>TinyGS &mdash; Update OK</title>
<meta http-equiv="refresh" content="15;URL=/">
<script>(function(){if(localStorage.getItem('tgs-theme')==='dark')document.documentElement.classList.add('dark');})();</script>
<style>
:root{--bg:#f8fafc;--surface:#fff;--surface2:#f1f5f9;--border:#cbd5e1;--accent:#3b82f6;--accent2:#0891b2;--text:#0f172a;--text2:#475569;--success:#16a34a;--radius:0.625rem;--font:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif}
html.dark{--bg:#0b0e17;--surface:#131827;--surface2:#1a2035;--border:#2a3050;--text:#e2e8f0;--text2:#94a3b8;--success:#22c55e}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:var(--font);min-height:100vh;display:flex;align-items:center;justify-content:center}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:2rem;max-width:380px;text-align:center}
.icon{font-size:3rem;margin-bottom:0.75rem}
h2{font-size:1.1rem;font-weight:700;color:var(--success);margin-bottom:0.5rem}
p{font-size:0.85rem;color:var(--text2)}
</style></head><body>
<div class="card">
  <div class="icon">&#10003;</div>
  <h2>Update successful!</h2>
  <p>The device is rebooting&hellip;<br>You will be redirected in 15 seconds.</p>
</div>
</body></html>)HTML";
};

/////////////////////////////////////////////////////////////////////////////////

#endif

#endif
