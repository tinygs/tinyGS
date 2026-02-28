/*
  INetworkAware.h - Interface for network-aware components

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

#ifndef INETWORKAWARE_H
#define INETWORKAWARE_H

#include <IPAddress.h>

enum class ActiveInterface : uint8_t {
  NONE = 0,
  WIFI,
  ETHERNET
};

enum class InterfaceMode : uint8_t {
  WIFI_ONLY = 0,
  ETH_ONLY,
  BOTH         // WiFi + Ethernet with automatic failover
};

class INetworkAware {
public:
  virtual ~INetworkAware() = default;

  /// Called when a network interface obtains an IP address
  virtual void onConnected(IPAddress ip, ActiveInterface iface) = 0;

  /// Called when all network interfaces lose connectivity
  virtual void onDisconnected() = 0;

  /// Called when the active interface changes (e.g. Ethernet → WiFi failover)
  virtual void onInterfaceChanged(ActiveInterface newIface, IPAddress newIP) {}
};

#endif // INETWORKAWARE_H
