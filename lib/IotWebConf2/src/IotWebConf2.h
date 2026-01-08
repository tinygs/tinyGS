/**
 * IotWebConf2.h -- IotWebConf2 is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf2
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConf2_h
#define IotWebConf2_h

#include <Arduino.h>
#include <IotWebConf2Parameter.h>
#include <IotWebConf2Settings.h>
#include <IotWebConf2WebServerWrapper.h>

#ifdef ESP8266
# include <ESP8266WiFi.h>
# include <ESP8266WebServer.h>
#elif defined(ESP32)
# include <WiFi.h>
# include <WebServer.h>
#endif
#include <DNSServer.h> // -- For captive portal

#ifdef ESP8266
# ifndef WebServer
#  define WebServer ESP8266WebServer
# endif
#endif

// -- HTML page fragments
const char IOTWEBCONF_HTML_HEAD[] PROGMEM         = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>\n";
const char IOTWEBCONF_HTML_STYLE_INNER[] PROGMEM  = ".de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input,select{padding:5px;font-size:1em;} input{width:95%;} select{width:100%} input[type=checkbox]{width:auto;scale:1.5;margin:10px;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}\n";
const char IOTWEBCONF_HTML_SCRIPT_INNER[] PROGMEM = "function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}; function pw(id) { var x=document.getElementById(id); if(x.type==='password') {x.type='text';} else {x.type='password';} };";
const char IOTWEBCONF_HTML_HEAD_END[] PROGMEM     = "</head><body>";
const char IOTWEBCONF_HTML_BODY_INNER[] PROGMEM   = "<div style='text-align:left;display:inline-block;min-width:260px;'>\n";
const char IOTWEBCONF_HTML_FORM_START[] PROGMEM   = "<form action='' method='post'><input type='hidden' name='iotSave' value='true'>\n";
const char IOTWEBCONF_HTML_FORM_END[] PROGMEM     = "<button type='submit' style='margin-top: 10px;'>Apply</button></form>\n";
const char IOTWEBCONF_HTML_SAVED[] PROGMEM        = "<div>Configuration saved<br />Return to <a href='/'>home page</a>.</div>\n";
const char IOTWEBCONF_HTML_END[] PROGMEM          = "</div></body></html>";
const char IOTWEBCONF_HTML_UPDATE[] PROGMEM       = "<div style='padding-top:25px;'><a href='{u}'>Firmware update</a></div>\n";
const char IOTWEBCONF_HTML_CONFIG_VER[] PROGMEM   = "<div style='font-size: .6em;'>Firmware config version '{v}'</div>\n";

// -- State of the Thing
#define IOTWEBCONF_STATE_BOOT 0
#define IOTWEBCONF_STATE_NOT_CONFIGURED 1
#define IOTWEBCONF_STATE_AP_MODE 2
#define IOTWEBCONF_STATE_CONNECTING 3
#define IOTWEBCONF_STATE_ONLINE 4

// -- AP connection state
// -- No connection on AP.
#define IOTWEBCONF_AP_CONNECTION_STATE_NC 0
// -- Has connection on AP.
#define IOTWEBCONF_AP_CONNECTION_STATE_C 1
// -- All previous connection on AP was disconnected.
#define IOTWEBCONF_AP_CONNECTION_STATE_DC 2

// -- User name on login.
#define IOTWEBCONF_ADMIN_USER_NAME "admin"

namespace iotwebconf2
{

class IotWebConf2;

typedef struct WifiAuthInfo
{
  const char* ssid;
  const char* password;
} WifiAuthInfo;

/**
 * Class for providing HTML format segments.
 */
class HtmlFormatProvider
{
public:
  virtual String getHead() { return FPSTR(IOTWEBCONF_HTML_HEAD); }
  virtual String getStyle() { return "<style>" + getStyleInner() + "</style>"; }
  virtual String getScript() { return "<script>" + getScriptInner() + "</script>"; }
  virtual String getHeadExtension() { return ""; }
  virtual String getHeadEnd() { return String(FPSTR(IOTWEBCONF_HTML_HEAD_END)) + getBodyInner(); }
  virtual String getFormStart() { return FPSTR(IOTWEBCONF_HTML_FORM_START); }
  virtual String getFormEnd() { return FPSTR(IOTWEBCONF_HTML_FORM_END); }
  virtual String getFormSaved() { return FPSTR(IOTWEBCONF_HTML_SAVED); }
  virtual String getEnd() { return FPSTR(IOTWEBCONF_HTML_END); }
  virtual String getUpdate() { return FPSTR(IOTWEBCONF_HTML_UPDATE); }
  virtual String getConfigVer() { return FPSTR(IOTWEBCONF_HTML_CONFIG_VER); }
protected:
  virtual String getStyleInner() { return FPSTR(IOTWEBCONF_HTML_STYLE_INNER); }
  virtual String getScriptInner() { return FPSTR(IOTWEBCONF_HTML_SCRIPT_INNER); }
  virtual String getBodyInner() { return FPSTR(IOTWEBCONF_HTML_BODY_INNER); }
};

class StandardWebRequestWrapper : public WebRequestWrapper
{
public:
  StandardWebRequestWrapper(WebServer* server) { this->_server = server; };

  const String hostHeader() const override { return this->_server->hostHeader(); };
  IPAddress localIP() override { return this->_server->client().localIP(); };
  uint16_t localPort() override { return this->_server->client().localPort(); };
  const String uri() const { return this->_server->uri(); };
  bool authenticate(const char * username, const char * password) override
    { return this->_server->authenticate(username, password); };
  void requestAuthentication() override
    { this->_server->requestAuthentication(); };
  bool hasArg(const String& name) override { return this->_server->hasArg(name); };
  String arg(const String name) override { return this->_server->arg(name); };
  void sendHeader(const String& name, const String& value, bool first = false) override
    { this->_server->sendHeader(name, value, first); };
  void setContentLength(const size_t contentLength) override
    { this->_server->setContentLength(contentLength); };
  void send(int code, const char* content_type = NULL, const String& content = String("")) override
    { this->_server->send(code, content_type, content); };
  void sendContent(const String& content) override { this->_server->sendContent(content); };
  void stop() override { this->_server->client().stop(); };

private:
  WebServer* _server;
  friend IotWebConf2;
};

class StandardWebServerWrapper : public WebServerWrapper
{
public:
  StandardWebServerWrapper(WebServer* server) { this->_server = server; };

  void handleClient() override { this->_server->handleClient(); };
  void begin() override { this->_server->begin(); };

private:
  StandardWebServerWrapper() { };
  WebServer* _server;
  friend IotWebConf2;
};

class WifiParameterGroup : public ParameterGroup
{
public:
  WifiParameterGroup(const char* id, const char* label = NULL) : ParameterGroup(id, label)
  {
    this->addItem(&this->wifiSsidParameter);
    this->addItem(&this->wifiPasswordParameter);
  }
  TextParameter wifiSsidParameter =
    TextParameter("WiFi SSID", "iwcWifiSsid", this->_wifiSsid, IOTWEBCONF_WORD_LEN);
  PasswordParameter wifiPasswordParameter =
    PasswordParameter("WiFi password", "iwcWifiPassword", this->_wifiPassword, IOTWEBCONF_WIFI_PASSWORD_LEN);
  char _wifiSsid[IOTWEBCONF_WORD_LEN];
  char _wifiPassword[IOTWEBCONF_WIFI_PASSWORD_LEN];
};

/**
 * Main class of the module.
 */
class IotWebConf2
{
public:
  /**
   * Create a new configuration handler.
   *   @thingName - Initial value for the thing name. Used in many places like AP name, can be changed by the user.
   *   @dnsServer - A created DNSServer, that can be configured for captive portal.
   *   @server - A created web server. Will be started upon connection success.
   *   @initialApPassword - Initial value for AP mode. Can be changed by the user.
   *   @configVersion - When the software is updated and the configuration is changing, this key should also be changed,
   *     so that the config portal will force the user to reenter all the configuration values.
   */
  IotWebConf2(
      const char* thingName, DNSServer* dnsServer, WebServer* server,
      const char* initialApPassword, const char* configVersion = "init") :
      IotWebConf2(thingName, dnsServer, &this->_standardWebServerWrapper, initialApPassword, configVersion)
  {
    this->_standardWebServerWrapper._server = server;
  }

  IotWebConf2(
      const char* thingName, DNSServer* dnsServer, WebServerWrapper* server,
      const char* initialApPassword, const char* configVersion = "init");

  /**
   * Provide an Arduino pin here, that has a button connected to it with the other end of the pin is connected to GND.
   * The button pin is queried at for input on boot time (init time).
   * If the button was pressed, the thing will enter AP mode with the initial password.
   * Must be called before init()!
   *   @configPin - An Arduino pin. Will be configured as INPUT_PULLUP!
   */
  void setConfigPin(int configPin);

  /**
   * Provide an Arduino pin for status indicator (LOW = on). Blink codes:
   *   - Rapid blinks - The thing is in AP mode with default password.
   *   - Rapid blinks, but mostly on - AP mode, waiting for configuration changes.
   *   - Normal blinks - Connecting to WiFi.
   *   - Mostly off with rare rapid blinks - WiFi is connected performing normal operation.
   * User can also apply custom blinks. See blink() method!
   * Must be called before init()!
   *   @statusPin - An Arduino pin. Will be configured as OUTPUT!
   *   @statusOnLevel - Logic level of the On state of the status pin. Default is LOW.
   */
  void setStatusPin(int statusPin, int statusOnLevel = LOW);

  /**
   * Add an UpdateServer instance to the system. The firmware update link will appear on the config portal.
   * The UpdateServer will be added to the WebServer with the path provided here (or with "firmware",
   * if none was provided).
   * Login user will be IOTWEBCONF_ADMIN_USER_NAME, password is the password provided in the config portal.
   * Should be called before init()!
   *   @updateServer - An uninitialized UpdateServer instance.
   *   @updatePath - (Optional) The path to set up the UpdateServer with. Will be also used in the config portal.
   */
  void setupUpdateServer(
    std::function<void(const char* _updatePath)> setup,
    std::function<void(const char* userName, char* password)> updateCredentials,
    const char* updatePath = "/firmware")
  {
    this->_updateServerSetupFunction = setup;
    this->_updateServerUpdateCredentialsFunction = updateCredentials;
    this->_updatePath = updatePath;
  }

  /**
   * Start up the IotWebConf2 module.
   * Loads all configuration from the EEPROM, and initialize the system.
   * Will return false, if no configuration (with specified config version) was found in the EEPROM.
   */
  bool init();

  /**
   * IotWebConf2 is a non-blocking, state controlled system. Therefor it should be
   * regularly triggered from the user code.
   * So call this method any time you can.
   */
  void doLoop();

  /**
   * Each WebServer URL handler method should start with calling this method.
   * If this method return true, the request was already served by it.
   */
  bool handleCaptivePortal(WebRequestWrapper* webRequestWrapper);
  bool handleCaptivePortal()
  {
    StandardWebRequestWrapper webRequestWrapper = StandardWebRequestWrapper(this->_standardWebServerWrapper._server);
    return handleCaptivePortal(&webRequestWrapper);
  }

  /**
   * Config URL web request handler. Call this method to handle config request.
   */
  void handleConfig(WebRequestWrapper* webRequestWrapper);
  void handleConfig()
  {
    StandardWebRequestWrapper webRequestWrapper = StandardWebRequestWrapper(this->_standardWebServerWrapper._server);
    handleConfig(&webRequestWrapper);
  }

  /**
   * URL-not-found web request handler. Used for handling captive portal request.
   */
  void handleNotFound(WebRequestWrapper* webRequestWrapper);
  void handleNotFound()
  {
    StandardWebRequestWrapper webRequestWrapper = StandardWebRequestWrapper(this->_standardWebServerWrapper._server);
    handleNotFound(&webRequestWrapper);
  }

  /**
   * Specify a callback method, that will be called upon WiFi connection success.
   * Should be called before init()!
   */
  void setWifiConnectionCallback(std::function<void()> func);

  /**
   * Specify a callback method, that will be called upon first config is saved.
   * Should be called before init()!
   */
  void setConfiguredCallback(std::function<void()> func);

  /**
   * Specify a callback method, that will be called when settings is being changed.
   * This is very handy if you have other routines, that are modifying the "EEPROM"
   * parallel to IotWebConf2, now this is the time to disable these routines.
   * Should be called before init()!
   */
  void setConfigSavingCallback(std::function<void(int size)> func);

  /**
   * Specify a callback method, that will be called when settings have been changed.
   * All pending EEPROM manipulations are done by the time this method is called.
   * Should be called before init()!
   */
  void setConfigSavedCallback(std::function<void()> func);

  /**
   * Specify a callback method, that will be called when form validation is required.
   * If the method will return false, the configuration will not be saved.
   * Should be called before init()!
   */
  void setFormValidator(std::function<bool(WebRequestWrapper* webRequestWrapper)> func);

  /**
   * Specify your custom Access Point connection handler. Please use IotWebConf2::connectAp() as
   * reference when implementing your custom solution.
   */
  void setApConnectionHandler(
      std::function<bool(const char* apName, const char* password)> func)
  {
    _apConnectionHandler = func;
  }

  /**
   * Specify your custom WiFi connection handler. Please use IotWebConf2::connectWifi() as
   * reference when implementing your custom solution.
   * Your method will be called when IotWebConf2 trying to establish
   * connection to a WiFi network.
   */
  void setWifiConnectionHandler(
      std::function<void(const char* ssid, const char* password)> func)
  {
    _wifiConnectionHandler = func;
  }

  /**
   * With this method you can specify your custom WiFi timeout handler.
   * This handler can manage what should happen, when WiFi connection timed out.
   * By default the handler implementation returns with NULL, as seen on reference implementation
   * IotWebConf2::handleConnectWifiFailure(). This means we need to fall back to AP mode.
   * If it method returns with a (new) WiFi settings, it is used as a next try.
   * Note, that in case once you have returned with NULL, you might also want to
   * resetWifiAuthInfo(), that sets the auth info used for the next time to the
   * one set up in the admin portal.
   * Note, that this feature is provided because of the option of providing multiple
   * WiFi settings utilized by the MultipleWifiAddition class. (See IotWebConf2MultipleWifi.h)
   */
  void setWifiConnectionFailedHandler( std::function<WifiAuthInfo*()> func )
  {
    _wifiConnectionFailureHandler = func;
  }

  /**
   * Add a custom parameter group, that will be handled by the IotWebConf2 module.
   * The parameters in this group will be saved to/loaded from EEPROM automatically,
   * and will appear on the config portal.
   * Must be called before init()!
   */
  void addParameterGroup(ParameterGroup* group);

  /**
   * Add a custom parameter group, that will be handled by the IotWebConf2 module.
   * The parameters in this group will be saved to/loaded from EEPROM automatically,
   * but will NOT appear on the config portal.
   * Must be called before init()!
   */
  void addHiddenParameter(ConfigItem* parameter);

  /**
   * Add a custom parameter group, that will be handled by the IotWebConf2 module.
   * The parameters in this group will be saved to/loaded from EEPROM automatically,
   * but will NOT appear on the config portal.
   * Must be called before init()!
   */
  void addSystemParameter(ConfigItem* parameter);

  /**
   * Getter for the actually configured thing name.
   */
  char* getThingName();

  /**
   * Use this delay, to prevent blocking IotWebConf2.
   */
  void delay(unsigned long millis);

  /**
   * IotWebConf2 tries to connect to the local network for an amount of time before falling back to AP mode.
   * The default amount can be updated with this setter.
   * Should be called before init()!
   */
  void setWifiConnectionTimeoutMs(unsigned long millis);

  /**
   * Interrupts internal blinking cycle and applies new values for
   * blinking the status LED (if one configured with setStatusPin() prior init()
   * ).
   *   @repeatMs - Defines the the period of one on-off cycle in milliseconds.
   *   @dutyCyclePercent - LED on/off percent. 100 means always on, 0 means
   * always off. When called with repeatMs = 0, then internal blink cycle will
   * be continued.
   */
  void blink(unsigned long repeatMs, byte dutyCyclePercent);

  /**
   * Similar to blink, but here we define exact on and off times for more
   * precise timings.
   *   @onMs - Milliseconds for the LED turned on.
   *   @offMs -  Milliseconds for the LED turned off.
   */
  void fineBlink(unsigned long onMs, unsigned long offMs);

  /**
   * Stop custom blinking defined by blink() or fineBlink() and continues with
   * the internal blink cycle.
   */
  void stopCustomBlink();

  /**
   * Disables blinking, so allows user code to control same LED.
   */
  void disableBlink() { this->_blinkEnabled = false; }

  /**
   * Enables blinking if it has been disabled by disableBlink().
   */
  void enableBlink()  { this->_blinkEnabled = true; }

  /**
   * Returns blink enabled state modified by disableBlink() and enableBlink().
   */
  bool isBlinkEnabled()  { return this->_blinkEnabled; }

  /**
   * Return the current state, that will be a value from the IOTWEBCONF_STATE_* constants.
   */
  byte getState () { return this->_state; };

  void changeState (byte newState);

  /**
   * This method can be used to set the AP timeout directly without modifying the apTimeoutParameter.
   * Note, that apTimeoutMs value will be reset to the value of apTimeoutParameter on init and on config save.
   */
  void setApTimeoutMs(unsigned long apTimeoutMs)
  {
    this->_apTimeoutMs = apTimeoutMs;
  };

  /**
   * Returns the actual value of the AP timeout in use.
   */
  unsigned long getApTimeoutMs() { return this->_apTimeoutMs; };

    /**
   * Returns the current WiFi authentication credentials. These are usually the configured ones,
   * but might be overwritten by setWifiConnectionFailedHandler().
   */
  WifiAuthInfo getWifiAuthInfo() { return _wifiAuthInfo; };

  /**
   * Resets the authentication credentials for WiFi connection to the configured one.
   * With the return value of setWifiConnectionFailedHandler() one can provide alternative connection settings,
   * that can be reset with resetWifiAuthInfo().
   */
  void resetWifiAuthInfo()
  {
    _wifiAuthInfo = {this->_wifiParameters._wifiSsid, this->_wifiParameters._wifiPassword};
  };

  /**
   * By default IotWebConf2 starts up in AP mode. Calling this method before the init will force IotWebConf2
   * to connect immediately to the configured WiFi network.
   * Note, this method only takes effect, when WiFi mode is enabled, thus when a valid WiFi connection is
   * set up, and AP mode is not forced by ConfigPin (see setConfigPin() for details).
   */
  void skipApStartup() { this->_skipApStartup = true; }

  /**
   * By default IotWebConf2 will continue startup in WiFi mode, when no configuration request arrived
   * in AP mode. With this method holding the AP mode can be forced.
   * Further more, instant AP mode can forced even when we are currently in WiFi mode.
   *   @value - When TRUE, AP mode is forced/entered.
   *     When FALSE, AP mode is released, normal operation will continue.
   */
  void forceApMode(bool value);

    /**
   * By default IotWebConf2 will set the thing password as the AP password when connecting in AP mode
   * With this method a default AP can be set to recover settings.
   * In combination with forceApMode, instan AP mode with default password can be triggered
   *   @value - When parameter is TRUE the AP mode does not ask for password
   *     When value is FALSE normal operation will continue.
   */
  void forceDefaultPassword(bool value)
  {
    this->_forceDefaultPassword = value;
  }

  /**
   * Get internal parameters, for manual handling.
   * Normally you don't need to access these parameters directly.
   * Note, that changing valueBuffer of these parameters should be followed by saveConfig()!
   */
  ParameterGroup* getSystemParameterGroup()
  {
    return &this->_systemParameters;
  };
  Parameter* getThingNameParameter()
  {
    return &this->_thingNameParameter;
  };
  Parameter* getApPasswordParameter()
  {
    return &this->_apPasswordParameter;
  };
  WifiParameterGroup* getWifiParameterGroup()
  {
    return &this->_wifiParameters;
  };
  Parameter* getWifiSsidParameter()
  {
    return &this->_wifiParameters.wifiSsidParameter;
  };
  Parameter* getWifiPasswordParameter()
  {
    return &this->_wifiParameters.wifiPasswordParameter;
  };
  Parameter* getApTimeoutParameter()
  {
    return &this->_apTimeoutParameter;
  };

  /**
   * If config parameters are modified directly, the new values can be saved by this method.
   * Note, that init() must pretend saveConfig()!
   * Also note, that saveConfig writes to EEPROM, and EEPROM can be written only some thousand times
   *  in the lifetime of an ESP8266 module.
   */
  void saveConfig();

  /**
   * Loads all configuration from the EEPROM without initializing the system.
   * Will return false, if no configuration (with specified config version) was found in the EEPROM.
   */
  bool loadConfig();

  /**
   * With this method you can override the default HTML format provider to
   * provide custom HTML segments.
   */
  void
  setHtmlFormatProvider(HtmlFormatProvider* customHtmlFormatProvider)
  {
    this->htmlFormatProvider = customHtmlFormatProvider;
  }
  HtmlFormatProvider* getHtmlFormatProvider()
  {
    return this->htmlFormatProvider;
  }

  bool isFailSafeActive() { return _failsafeTriggered; }

private:
  const char* _initialApPassword = NULL;
  const char* _configVersion;
  DNSServer* _dnsServer;
  WebServerWrapper* _webServerWrapper;
  StandardWebServerWrapper _standardWebServerWrapper = StandardWebServerWrapper();
  std::function<void(const char* _updatePath)>
    _updateServerSetupFunction = NULL;
  std::function<void(const char* userName, char* password)>
    _updateServerUpdateCredentialsFunction = NULL;
  int _configPin = -1;
  int _statusPin = -1;
  int _statusOnLevel = LOW;
  const char* _updatePath = NULL;
  bool _forceDefaultPassword = false;
  bool _skipApStartup = false;
  bool _forceApMode = false;
  ParameterGroup _allParameters = ParameterGroup("iwcAll");
  ParameterGroup _systemParameters = ParameterGroup("iwcSys", "System configuration");
  ParameterGroup _customParameterGroups = ParameterGroup("iwcCustom");
  ParameterGroup _hiddenParameters = ParameterGroup("hidden");
  WifiParameterGroup _wifiParameters = WifiParameterGroup("iwcWifi0");
  TextParameter _thingNameParameter =
    TextParameter("Thing name", "iwcThingName", this->_thingName, IOTWEBCONF_WORD_LEN);
  PasswordParameter _apPasswordParameter =
    PasswordParameter("AP password", "iwcApPassword", this->_apPassword, IOTWEBCONF_PASSWORD_LEN);
  NumberParameter _apTimeoutParameter =
    NumberParameter("Startup delay (seconds)", "iwcApTimeout", this->_apTimeoutStr, IOTWEBCONF_WORD_LEN, IOTWEBCONF_DEFAULT_AP_MODE_TIMEOUT_SECS, NULL, "min='1' max='600'");
  char _thingName[IOTWEBCONF_WORD_LEN];
  char _apPassword[IOTWEBCONF_PASSWORD_LEN];
  char _apTimeoutStr[IOTWEBCONF_WORD_LEN];
  unsigned long _apTimeoutMs;
  // TODO: Add to WifiParameterGroup
  unsigned long _wifiConnectionTimeoutMs =
      IOTWEBCONF_DEFAULT_WIFI_CONNECTION_TIMEOUT_MS;
  byte _state = IOTWEBCONF_STATE_BOOT;
  unsigned long _apStartTimeMs = 0;
  byte _apConnectionStatus = IOTWEBCONF_AP_CONNECTION_STATE_NC;
  std::function<void()> _wifiConnectionCallback = NULL;
  std::function<void()> _configuredCallback = NULL;
  std::function<void(int)> _configSavingCallback = NULL;
  std::function<void()> _configSavedCallback = NULL;
  std::function<bool(WebRequestWrapper* webRequestWrapper)> _formValidator = NULL;
  std::function<void(const char*, const char*)> _apConnectionHandler =
      &(IotWebConf2::connectAp);
  std::function<void(const char*, const char*)> _wifiConnectionHandler =
      &(IotWebConf2::connectWifi);
  std::function<WifiAuthInfo*()> _wifiConnectionFailureHandler =
      &(IotWebConf2::handleConnectWifiFailure);
  unsigned long _internalBlinkOnMs = 500;
  unsigned long _internalBlinkOffMs = 500;
  unsigned long _blinkOnMs = 500;
  unsigned long _blinkOffMs = 500;
  bool _blinkEnabled = true;
  bool _blinkStateOn = false;
  unsigned long _lastBlinkTime = 0;
  unsigned long _wifiConnectionStart = 0;
  // TODO: authinfo
  WifiAuthInfo _wifiAuthInfo;
  HtmlFormatProvider htmlFormatProviderInstance;
  HtmlFormatProvider* htmlFormatProvider = &htmlFormatProviderInstance;

  // Failsafe
  uint8_t _bootCount = 0;
  bool _failsafeTriggered = false;
  void loadFailSafeCounter();
  void resetFailSafeCounter();

  int initConfig();
  bool testConfigVersion();
  void saveConfigVersion();
  void readEepromValue(int start, byte* valueBuffer, int length);
  void writeEepromValue(int start, byte* valueBuffer, int length);

  bool validateForm(WebRequestWrapper* webRequestWrapper);

//   void changeState(byte newState);
  void stateChanged(byte oldState, byte newState);
  bool mustUseDefaultPassword()
  {
    return this->_forceDefaultPassword || (this->_apPassword[0] == '\0');
  }
  bool mustStayInApMode()
  {
    return this->_forceDefaultPassword || (this->_apPassword[0] == '\0') ||
      (this->_wifiParameters._wifiSsid[0] == '\0') || this->_forceApMode;
  }
  bool isIp(String str);
  String toStringIp(IPAddress ip);
  void doBlink();
  void blinkInternal(unsigned long repeatMs, byte dutyCyclePercent);

  void checkApTimeout();
  void checkConnection();
  bool checkWifiConnection();
  void setupAp();
  void stopAp();

  static bool connectAp(const char* apName, const char* password);
  static void connectWifi(const char* ssid, const char* password);
  static WifiAuthInfo* handleConnectWifiFailure();
};

} // end namespace

using iotwebconf2::IotWebConf2;

#endif

