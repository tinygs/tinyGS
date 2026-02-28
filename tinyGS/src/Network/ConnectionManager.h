/*
  ConnectionManager.h - Network connection supervisor using EthWiFiManager

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

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <Arduino.h>
#include <EthWiFiManager.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "INetworkAware.h"
#include "ConfigStore.h"
#include <vector>

enum class ConnState : uint8_t {
  DISCONNECTED = 0,
  AP_MODE,
  CONNECTING,
  CONNECTED,
  SWITCHING_INTERFACE
};

class ConnectionManager {
public:
  static ConnectionManager& getInstance() {
    static ConnectionManager instance;
    return instance;
  }

  void init();
  void loop();

  // ---- State queries ----
  bool isConnected()  const { return _state == ConnState::CONNECTED; }
  bool isApMode()     const { return _state == ConnState::AP_MODE; }
  ConnState getState() const { return _state; }

  IPAddress getLocalIP()          const;
  ActiveInterface getActiveInterface() const { return _activeInterface; }
  String getActiveInterfaceName() const;

  // ---- AP management ----
  void forceApMode(bool enable);

  // ---- Observer registration ----
  void registerObserver(INetworkAware* obs);
  void unregisterObserver(INetworkAware* obs);

  // ---- Manual delay (replaces configManager.delay) ----
  void managedDelay(unsigned long ms);

private:
  ConnectionManager();
  void setupEthWiFiManager();
  void setupAP();
  void startAlwaysAP();
  void onEthEvent(EthWiFiManager::Event event, IPAddress ip);
  void notifyConnected(IPAddress ip, ActiveInterface iface);
  void notifyDisconnected();
  void notifyInterfaceChanged(ActiveInterface iface, IPAddress ip);

  EthWiFiManager _ewm;
  DNSServer      _dnsServer;
  ConnState      _state = ConnState::DISCONNECTED;
  ActiveInterface _activeInterface = ActiveInterface::NONE;
  IPAddress      _localIP;

  std::vector<INetworkAware*> _observers;

  // AP mode
  bool _apModeForced = false;
  unsigned long _apStartTime = 0;
  static constexpr unsigned long AP_TIMEOUT_MS = 300000; // 5 min

  // Connection timeout: fall back to AP if WiFi doesn't connect in time
  unsigned long _connectStartTime = 0;
  static constexpr unsigned long CONNECT_TIMEOUT_MS = 30000; // 30 s

  // Interface switching grace period
  static constexpr unsigned long SWITCH_GRACE_MS = 5000;
  unsigned long _switchStartTime = 0;
};

#endif // CONNECTIONMANAGER_H
