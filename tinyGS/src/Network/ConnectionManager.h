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
  /// Returns true if Ethernet hardware failed to initialise at boot.
  bool isEthFailed()   const { return _runtimeEthFailed; }

  IPAddress getLocalIP()          const;
  ActiveInterface getActiveInterface() const { return _activeInterface; }
  String getActiveInterfaceName() const;
  int32_t getNetworkRSSI() const;

  // ---- AP management ----
  void forceApMode(bool enable);
  void stopAP();

  // ---- WiFi connection (used by Improv) ----
  bool connectWiFi(const char* ssid, const char* password, unsigned long timeoutMs = 10000);

  // ---- WiFi scan (delegate to EthWiFiManager) ----
  int16_t          scanNetworks(bool async = false, bool showHidden = false);
  int16_t          scanComplete() const;
  void             scanDelete();
  String           scannedSSID(uint8_t i) const;
  int32_t          scannedRSSI(uint8_t i) const;
  wifi_auth_mode_t scannedEncryptionType(uint8_t i) const;

  // ---- Credential management ----
  /// Update stored WiFi credentials and trigger a reconnection if WiFi is enabled.
  void updateWiFiCredentials(const char* ssid, const char* password);

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

  // When connectivity is restored while the AP is active, keep it up for this
  // long so any client already connected can finish configuring the device.
  unsigned long _apCloseAfterConnectTime = 0;          // 0 = not scheduled
  static constexpr unsigned long AP_CLOSE_DELAY_MS = 120000; // 2 min

  // Connection timeout: fall back to AP if no interface connects in time
  unsigned long _connectStartTime = 0;
  static constexpr unsigned long CONNECT_TIMEOUT_MS = 60000; // 1 min

  // ETH-only connection timeout: start AP rescue if Ethernet never comes up
  unsigned long _ethConnectStartTime = 0;
  static constexpr unsigned long ETH_CONNECT_TIMEOUT_MS = 60000; // 1 min

  // Set to true if Ethernet hardware failed to initialise at startup.
  // Used to force WIFI_ONLY regardless of the configured InterfaceMode.
  bool _runtimeEthFailed = false;

  // Interface switching grace period
  static constexpr unsigned long SWITCH_GRACE_MS = 5000;
  unsigned long _switchStartTime = 0;
};

#endif // CONNECTIONMANAGER_H
