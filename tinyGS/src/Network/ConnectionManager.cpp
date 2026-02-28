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

ConnectionManager::ConnectionManager() {}

void ConnectionManager::init() {
  ConfigStore& cfg = ConfigStore::getInstance();

  // If WiFi credentials are available, proceed with connection
  if (strlen(cfg.getWifiSSID()) > 0) {
    setupEthWiFiManager();
    _state = ConnState::CONNECTING;
  } else {
    // No credentials → AP mode
    setupAP();
  }
}

void ConnectionManager::setupEthWiFiManager() {
  ConfigStore& cfg = ConfigStore::getInstance();

  EthWiFiManager::Config ewmCfg;
  ewmCfg.wifi.ssid     = cfg.getWifiSSID();
  ewmCfg.wifi.password = cfg.getWifiPassword();

  // Configure Ethernet if board supports it and mode allows it
  board_t board;
  if (cfg.getBoardConfig(board) && board.ethEN &&
      cfg.getInterfaceMode() != InterfaceMode::WIFI_ONLY) {

    ewmCfg.ethernet.enabled = true;

    // Map board ethPHY to SpiModule enum: 0=W5500, 1=DM9051, 2=KSZ8851SNL
    switch (board.ethPHY) {
      case 1:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::DM9051; break;
      case 2:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::KSZ8851SNL; break;
      default: ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::W5500; break;
    }
    ewmCfg.ethernet.csPin  = (gpio_num_t)board.ethCS;
    ewmCfg.ethernet.intPin = (gpio_num_t)board.ethINT;

    // Determine SPI bus for Ethernet
    if (board.ethMISO != UNUSED_PIN && board.ethMOSI != UNUSED_PIN && board.ethSCK != UNUSED_PIN) {
      // Dedicated SPI bus for Ethernet
      ewmCfg.ethernet.misoPin = (gpio_num_t)board.ethMISO;
      ewmCfg.ethernet.mosiPin = (gpio_num_t)board.ethMOSI;
      ewmCfg.ethernet.sckPin  = (gpio_num_t)board.ethSCK;
    } else {
      // Shared SPI bus with Radio (Radio must init first)
      ewmCfg.ethernet.misoPin = (gpio_num_t)board.L_MISO;
      ewmCfg.ethernet.mosiPin = (gpio_num_t)board.L_MOSI;
      ewmCfg.ethernet.sckPin  = (gpio_num_t)board.L_SCK;
    }
  } else {
    ewmCfg.ethernet.enabled = false;
  }

  // Register event handler
  _ewm.onEvent([this](EthWiFiManager::Event ev, IPAddress ip) {
    onEthEvent(ev, ip);
  });

  _ewm.begin(ewmCfg);

  // Disable WiFi if ETH_ONLY mode (must be done after begin())
  if (cfg.getInterfaceMode() == InterfaceMode::ETH_ONLY) {
    _ewm.disableWiFi();
  }
}

void ConnectionManager::setupAP() {
  ConfigStore& cfg = ConfigStore::getInstance();
  _state = ConnState::AP_MODE;
  _apStartTime = millis();

  // Start WiFi in AP mode
  String apName = String("TinyGS_") + cfg.getThingName();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str(), cfg.getApPassword());

  // Start DNS server for captive portal
  _dnsServer.start(53, "*", WiFi.softAPIP());

  Log::console(PSTR("AP mode started: %s (IP: %s)"),
               apName.c_str(), WiFi.softAPIP().toString().c_str());
}

void ConnectionManager::onEthEvent(EthWiFiManager::Event event, IPAddress ip) {
  switch (event) {
    case EthWiFiManager::Event::WiFiGotIP:
      Log::console(PSTR("WiFi connected, IP: %s"), ip.toString().c_str());
      _activeInterface = ActiveInterface::WIFI;
      _localIP = ip;
      _state = ConnState::CONNECTED;
      notifyConnected(ip, ActiveInterface::WIFI);
      break;

    case EthWiFiManager::Event::EthGotIP:
      Log::console(PSTR("Ethernet connected, IP: %s"), ip.toString().c_str());
      _activeInterface = ActiveInterface::ETHERNET;
      _localIP = ip;
      _state = ConnState::CONNECTED;
      notifyConnected(ip, ActiveInterface::ETHERNET);
      break;

    case EthWiFiManager::Event::WiFiDisconnected:
      Log::console(PSTR("WiFi disconnected"));
      if (_activeInterface == ActiveInterface::WIFI) {
        // Check if Ethernet is still up
        if (_ewm.ethernetHasIP()) {
          _state = ConnState::SWITCHING_INTERFACE;
          _switchStartTime = millis();
          // Don't notify disconnect yet - wait for grace period
        } else {
          _activeInterface = ActiveInterface::NONE;
          _state = ConnState::DISCONNECTED;
          notifyDisconnected();
        }
      }
      break;

    case EthWiFiManager::Event::EthLinkDown:
      Log::console(PSTR("Ethernet link down"));
      if (_activeInterface == ActiveInterface::ETHERNET) {
        if (WiFi.isConnected()) {
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
      Log::console(PSTR("Interface changed, new IP: %s"), ip.toString().c_str());
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
      Log::console(PSTR("AP timeout, attempting connection..."));
      WiFi.softAPdisconnect(true);
      _dnsServer.stop();
      setupEthWiFiManager();
      _state = ConnState::CONNECTING;
    }
  }

  // Interface switching grace period
  if (_state == ConnState::SWITCHING_INTERFACE) {
    if (millis() - _switchStartTime > SWITCH_GRACE_MS) {
      // Grace period expired, check which interface is available
      if (_ewm.ethernetHasIP()) {
        IPAddress ip = _ewm.localIP();
        _activeInterface = ActiveInterface::ETHERNET;
        _localIP = ip;
        _state = ConnState::CONNECTED;
        notifyInterfaceChanged(ActiveInterface::ETHERNET, ip);
      } else if (WiFi.isConnected()) {
        IPAddress ip = WiFi.localIP();
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

void ConnectionManager::forceApMode(bool enable) {
  _apModeForced = enable;
  if (enable) {
    setupAP();
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
