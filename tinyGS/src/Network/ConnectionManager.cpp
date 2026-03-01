/*
  ConnectionManager.cpp - Network connection supervisor using EthWiFiManager

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

#include "ConnectionManager.h"
#include "../Logger/Logger.h"
#include <esp_wifi.h>

ConnectionManager::ConnectionManager() {}

void ConnectionManager::init() {
  ConfigStore& cfg = ConfigStore::getInstance();

  // If WiFi credentials are available, proceed with connection
  if (strlen(cfg.getWifiSSID()) > 0) {
    setupEthWiFiManager();
    _state = ConnState::CONNECTING;
    _connectStartTime = millis();
  } else {
    // No credentials → AP mode
    setupAP();
  }
}

void ConnectionManager::setupEthWiFiManager() {
  ConfigStore& cfg = ConfigStore::getInstance();
  InterfaceMode mode = cfg.getInterfaceMode();

  EthWiFiManager::Config ewmCfg;

  // WiFi credentials — always passed to the config struct so enableWiFi()
  // can use them later, but we disable WiFi after begin() if ETH_ONLY.
  ewmCfg.wifi.ssid     = cfg.getWifiSSID();
  ewmCfg.wifi.password = cfg.getWifiPassword();
  // Disable WiFi at the config level for ETH_ONLY: this must be set BEFORE
  // begin() so the library sees m_wifiEnabled=false during initEthernet().
  // The disableWiFi() call after begin() remains as a safety net for any
  // dynamic mode changes.
  ewmCfg.wifi.enabled  = (mode != InterfaceMode::ETH_ONLY);

  // Configure Ethernet if the board template enables it.
  board_t board;
  bool ethHardwareAvailable = cfg.getBoardConfig(board) && board.ethEN;

  if (ethHardwareAvailable) {
    // If the board defines a hardware reset pin, pulse it now so the
    // chip is guaranteed to be out of reset before SPI probing starts.
    if (board.ethRST != UNUSED_PIN) {
      LOG_CONSOLE(PSTR("[ETH] Resetting module on GPIO %d"), board.ethRST);
      pinMode(board.ethRST, OUTPUT);
      digitalWrite(board.ethRST, LOW);
      delay(50);
      digitalWrite(board.ethRST, HIGH);
      delay(50);
    }

    ewmCfg.ethernet.enabled = true;

    // Map board ethPHY to SpiModule enum: 0=W5500, 1=DM9051, 2=KSZ8851SNL
    switch (board.ethPHY) {
      case 1:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::DM9051; break;
      case 2:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::KSZ8851SNL; break;
      default: ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::W5500; break;
    }
    // SPI bus: 2=SPI2_HOST, 3=SPI3_HOST
    ewmCfg.ethernet.spiHost = (board.ethSPI == 3) ? SPI3_HOST : SPI2_HOST;
    ewmCfg.ethernet.csPin  = (gpio_num_t)board.ethCS;
    ewmCfg.ethernet.intPin = (gpio_num_t)board.ethINT;

    // Determine SPI pins for Ethernet
    if (board.ethMISO != UNUSED_PIN && board.ethMOSI != UNUSED_PIN && board.ethSCK != UNUSED_PIN) {
      ewmCfg.ethernet.misoPin = (gpio_num_t)board.ethMISO;
      ewmCfg.ethernet.mosiPin = (gpio_num_t)board.ethMOSI;
      ewmCfg.ethernet.sckPin  = (gpio_num_t)board.ethSCK;
    } else {
      ewmCfg.ethernet.misoPin = (gpio_num_t)board.L_MISO;
      ewmCfg.ethernet.mosiPin = (gpio_num_t)board.L_MOSI;
      ewmCfg.ethernet.sckPin  = (gpio_num_t)board.L_SCK;
    }

    LOG_CONSOLE(PSTR("[ETH] Config: PHY=%d CS=%d INT=%d RST=%d MISO=%d MOSI=%d SCK=%d SPI=%d"),
      board.ethPHY, board.ethCS, board.ethINT, board.ethRST,
      ewmCfg.ethernet.misoPin, ewmCfg.ethernet.mosiPin, ewmCfg.ethernet.sckPin,
      board.ethSPI);
  } else {
    ewmCfg.ethernet.enabled = false;
    LOG_CONSOLE(PSTR("[ETH] Disabled (ethEN=false in board template)"));
  }

  // Register event handler
  _ewm.onEvent([this](EthWiFiManager::Event ev, IPAddress ip) {
    onEthEvent(ev, ip);
  });

  _ewm.begin(ewmCfg);

  // ── Enforce InterfaceMode via the library's enable/disable API ──
  switch (mode) {
    case InterfaceMode::WIFI_ONLY:
      _ewm.disableEthernet();
      LOG_CONSOLE(PSTR("[NET] Interface Mode = WIFI_ONLY (Ethernet disabled)"));
      break;
    case InterfaceMode::ETH_ONLY:
      _ewm.disableWiFi();
      LOG_CONSOLE(PSTR("[NET] Interface Mode = ETH_ONLY (WiFi disabled)"));
      break;
    case InterfaceMode::BOTH:
    default:
      LOG_CONSOLE(PSTR("[NET] Interface Mode = BOTH (failover)"));
      break;
  }
}

void ConnectionManager::setupAP() {
  ConfigStore& cfg = ConfigStore::getInstance();
  _state = ConnState::AP_MODE;
  _apStartTime = millis();

  // Start WiFi AP via EthWiFiManager
  String apName = String("TinyGS_") + cfg.getThingName();
  EthWiFiManager::ApConfig apCfg;
  apCfg.ssid     = apName.c_str();
  apCfg.password = cfg.getApPassword();
  _ewm.enableAP(apCfg);

  // Start DNS server for captive portal — use library's apLocalIP() directly
  _dnsServer.start(53, "*", _ewm.apLocalIP());

  LOG_CONSOLE(PSTR("AP mode started: %s (IP: %s)"),
               apName.c_str(), _ewm.apLocalIP().toString().c_str());
}

void ConnectionManager::onEthEvent(EthWiFiManager::Event event, IPAddress ip) {
  switch (event) {
    case EthWiFiManager::Event::WiFiGotIP:
      LOG_CONSOLE(PSTR("WiFi connected, IP: %s"), ip.toString().c_str());
      _activeInterface = ActiveInterface::WIFI;
      _localIP = ip;
      _state = ConnState::CONNECTED;
      if (ConfigStore::getInstance().getAlwaysAP()) startAlwaysAP();
      notifyConnected(ip, ActiveInterface::WIFI);
      break;

    case EthWiFiManager::Event::EthGotIP:
      LOG_CONSOLE(PSTR("Ethernet connected, IP: %s"), ip.toString().c_str());
      _activeInterface = ActiveInterface::ETHERNET;
      _localIP = ip;
      _state = ConnState::CONNECTED;
      notifyConnected(ip, ActiveInterface::ETHERNET);
      break;

    case EthWiFiManager::Event::WiFiDisconnected:
      LOG_CONSOLE(PSTR("WiFi disconnected"));
      if (_activeInterface == ActiveInterface::WIFI) {
        // Check if Ethernet is still up
        if ((uint32_t)_ewm.getEthernetIP() != 0) {
          _state = ConnState::SWITCHING_INTERFACE;
          _switchStartTime = millis();
        } else {
          _activeInterface = ActiveInterface::NONE;
          _localIP = IPAddress();
          _state = ConnState::CONNECTING;
          _connectStartTime = millis();
          notifyDisconnected();
        }
      }
      // If _activeInterface == NONE we are in the initial CONNECTING phase.
      // Do NOT reset _connectStartTime — let loop()'s 30s timeout run.
      break;

    case EthWiFiManager::Event::EthLinkDown:
      LOG_CONSOLE(PSTR("Ethernet link down"));
      if (_activeInterface == ActiveInterface::ETHERNET) {
        // In BOTH mode the library will attempt WiFi fallback automatically.
        // In ETH_ONLY the library won't reconnect WiFi (disableWiFi was called).
        if ((uint32_t)_ewm.getWiFiIP() != 0) {
          _state = ConnState::SWITCHING_INTERFACE;
          _switchStartTime = millis();
        } else {
          _activeInterface = ActiveInterface::NONE;
          _state = ConnState::DISCONNECTED;
          notifyDisconnected();
        }
      }
      break;

    case EthWiFiManager::Event::InterfaceChanged:
      LOG_CONSOLE(PSTR("Interface changed, new IP: %s"), ip.toString().c_str());
      if ((uint32_t)ip == 0) {
        // IP 0.0.0.0 means connectivity was just lost — already handled above.
        break;
      }
      {
        ActiveInterface newIface = _ewm.activeInterface() == EthWiFiManager::ActiveInterface::Ethernet
                                     ? ActiveInterface::ETHERNET
                                     : ActiveInterface::WIFI;
        _activeInterface = newIface;
        _localIP = ip;
        _state = ConnState::CONNECTED;
        notifyInterfaceChanged(newIface, ip);
      }
      break;

    default:
      break;
  }
}

void ConnectionManager::loop() {
  // EthWiFiManager is event-driven via ESP-IDF event loop - no loop() call needed

  // AP mode timeout
  if (_state == ConnState::AP_MODE && !_apModeForced) {
    _dnsServer.processNextRequest();

    if (millis() - _apStartTime > AP_TIMEOUT_MS) {
      LOG_CONSOLE(PSTR("AP timeout, attempting connection..."));
      _ewm.disableAP();
      _dnsServer.stop();
      ConfigStore& cfg = ConfigStore::getInstance();
      EthWiFiManager::WiFiConfig wifiCfg;
      wifiCfg.ssid     = cfg.getWifiSSID();
      wifiCfg.password = cfg.getWifiPassword();
      _ewm.enableWiFi(wifiCfg);
      _state = ConnState::CONNECTING;
      _connectStartTime = millis();
    }
  }

  // Connection timeout: if WiFi hasn't connected, fall back to AP mode
  if (_state == ConnState::CONNECTING) {
    if (millis() - _connectStartTime > CONNECT_TIMEOUT_MS) {
      LOG_CONSOLE(PSTR("WiFi connection timeout, starting AP mode..."));
      _ewm.disableWiFi();   // stops EthWiFiManager's internal reconnect loop
      setupAP();
    }
  }

  // Interface switching grace period
  if (_state == ConnState::SWITCHING_INTERFACE) {
    if (millis() - _switchStartTime > SWITCH_GRACE_MS) {
      // Grace period expired, check which interface is available
      if ((uint32_t)_ewm.getEthernetIP() != 0) {
        IPAddress ip = _ewm.getEthernetIP();
        _activeInterface = ActiveInterface::ETHERNET;
        _localIP = ip;
        _state = ConnState::CONNECTED;
        notifyInterfaceChanged(ActiveInterface::ETHERNET, ip);
      } else if ((uint32_t)_ewm.getWiFiIP() != 0) {
        IPAddress ip = _ewm.getWiFiIP();
        _activeInterface = ActiveInterface::WIFI;
        _localIP = ip;
        _state = ConnState::CONNECTED;
        notifyInterfaceChanged(ActiveInterface::WIFI, ip);
      } else {
        _activeInterface = ActiveInterface::NONE;
        _state = ConnState::DISCONNECTED;
        notifyDisconnected();
      }
    }
  }
}

IPAddress ConnectionManager::getLocalIP() const {
  return _localIP;
}

String ConnectionManager::getActiveInterfaceName() const {
  switch (_activeInterface) {
    case ActiveInterface::WIFI:     return "WiFi";
    case ActiveInterface::ETHERNET: return "Ethernet";
    default:                        return "None";
  }
}

int32_t ConnectionManager::getNetworkRSSI() const {
  if (_activeInterface == ActiveInterface::WIFI) {
    return _ewm.RSSI();
  }
  return 0;
}

void ConnectionManager::forceApMode(bool enable) {
  _apModeForced = enable;
  if (enable) {
    setupAP();
  }
}

void ConnectionManager::stopAP() {
  _ewm.disableAP();
  _dnsServer.stop();
}

bool ConnectionManager::connectWiFi(const char* ssid, const char* password, unsigned long timeoutMs) {
  // Stop AP if running
  stopAP();

  // Use EthWiFiManager to connect WiFi
  EthWiFiManager::WiFiConfig wifiCfg;
  wifiCfg.ssid     = ssid;
  wifiCfg.password = password;
  _ewm.enableWiFi(wifiCfg);

  // Poll for connection (blocking — used by Improv provisioning flow)
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (_ewm.status() == WL_CONNECTED) {
      // Wait for DHCP to assign a valid IP
      for (int i = 0; i < 20 && (uint32_t)_ewm.getWiFiIP() == 0; i++) {
        delay(250);
      }
      return true;
    }
    delay(250);
  }

  // Failed — disable WiFi so it doesn't keep retrying
  _ewm.disableWiFi();
  return false;
}

// ---- WiFi scan wrappers ----

int16_t ConnectionManager::scanNetworks(bool async, bool showHidden) {
  return _ewm.scanNetworks(async, showHidden);
}

int16_t ConnectionManager::scanComplete() const {
  return _ewm.scanComplete();
}

void ConnectionManager::scanDelete() {
  _ewm.scanDelete();
}

String ConnectionManager::scannedSSID(uint8_t i) const {
  return _ewm.scannedSSID(i);
}

int32_t ConnectionManager::scannedRSSI(uint8_t i) const {
  return _ewm.scannedRSSI(i);
}

wifi_auth_mode_t ConnectionManager::scannedEncryptionType(uint8_t i) const {
  return _ewm.scannedEncryptionType(i);
}

// ---- Credential management ----

void ConnectionManager::updateWiFiCredentials(const char* ssid, const char* password) {
  _ewm.setWiFiCredentials(ssid, password);
  if (_ewm.isWiFiEnabled()) {
    _ewm.reconnect();
  }
}

// ---- Observer pattern ----

void ConnectionManager::registerObserver(INetworkAware* obs) {
  _observers.push_back(obs);
}

void ConnectionManager::unregisterObserver(INetworkAware* obs) {
  _observers.erase(
    std::remove(_observers.begin(), _observers.end(), obs),
    _observers.end()
  );
}

void ConnectionManager::notifyConnected(IPAddress ip, ActiveInterface iface) {
  for (auto* obs : _observers) {
    obs->onConnected(ip, iface);
  }
}

void ConnectionManager::notifyDisconnected() {
  for (auto* obs : _observers) {
    obs->onDisconnected();
  }
}

void ConnectionManager::notifyInterfaceChanged(ActiveInterface iface, IPAddress ip) {
  for (auto* obs : _observers) {
    obs->onInterfaceChanged(iface, ip);
  }
}

void ConnectionManager::managedDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    loop();
    delay(1);
  }
}

void ConnectionManager::startAlwaysAP() {
  ConfigStore& cfg = ConfigStore::getInstance();
  String apName = String("TinyGS_") + cfg.getThingName();

  EthWiFiManager::ApConfig apCfg;
  apCfg.ssid     = apName.c_str();
  apCfg.password  = cfg.getApPassword();
  // Channel and bandwidth are auto-matched to STA by the library
  _ewm.enableAP(apCfg);

  LOG_CONSOLE(PSTR("Always-AP: %s  IP=%s"),
    apName.c_str(),
    WiFi.softAPIP().toString().c_str());
}
