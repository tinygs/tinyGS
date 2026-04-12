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
  // Always initialise EthWiFiManager first — this runs esp_netif_init() /
  // esp_event_loop_create_default() which are prerequisites for ANY network
  // operation (WiFi, Ethernet, DNS, httpd sockets …).
  setupEthWiFiManager();

  // Determine effective mode. Ethernet is unavailable if the board template
  // has it disabled OR if the hardware failed to initialise at runtime.
  board_t board;
  bool ethAvail = !_runtimeEthFailed && cfg.getBoardConfig(board) && board.ethEN;
  InterfaceMode mode = cfg.getInterfaceMode();
  if (!ethAvail && mode != InterfaceMode::WIFI_ONLY)
    mode = InterfaceMode::WIFI_ONLY;

  if (mode == InterfaceMode::ETH_ONLY) {
    // Ethernet-only: wait for ETH. If it never connects within the timeout,
    // start AP rescue mode so the user can reconfigure.
    _state = ConnState::CONNECTING;
    _ethConnectStartTime = millis();
  } else if (strlen(cfg.getWifiSSID()) > 0) {
    _state = ConnState::CONNECTING;
    _connectStartTime = millis();
  } else {
    // No credentials, WiFi-capable mode → AP for provisioning.
    // Small delay to let the NetworkEvents task finish processing the
    // WIFI_EVENT_STA_START that WiFi.mode(WIFI_STA) just fired.  On dual-core
    // chips the event task runs on a different core and its range-for over
    // _cbEventList would race with the emplace_front that the upcoming
    // WiFi.mode(WIFI_AP_STA) triggers inside WiFi.AP.onEnable().
    delay(100);
    setupAP();
  }
}

void ConnectionManager::setupEthWiFiManager() {
  ConfigStore& cfg = ConfigStore::getInstance();
  InterfaceMode mode = cfg.getInterfaceMode();

  EthWiFiManager::Config ewmCfg;

  // Determine Ethernet hardware availability FIRST so we can override mode
  // before it influences any other decision.
  board_t board;
  bool ethHardwareAvailable = cfg.getBoardConfig(board) && board.ethEN;

  // If no Ethernet hardware is present, refuse to honour ETH_ONLY / BOTH —
  // silently downgrade to WIFI_ONLY so the device still connects.
  if (!ethHardwareAvailable && mode != InterfaceMode::WIFI_ONLY) {
    LOG_CONSOLE(PSTR("[NET] No Ethernet hardware — forcing Interface Mode to WIFI_ONLY"));
    mode = InterfaceMode::WIFI_ONLY;
  }

  // WiFi credentials — always passed to the config struct so enableWiFi()
  // can use them later, but we disable WiFi after begin() if ETH_ONLY.
  ewmCfg.wifi.ssid     = cfg.getWifiSSID();
  ewmCfg.wifi.password = cfg.getWifiPassword();
  // Only enable WiFi if there are actual credentials AND the mode allows it.
  // Passing wifi.enabled=true with an empty SSID causes EthWiFiManager to call
  // WiFi.begin("","") which triggers an immediate STA_DISCONNECTED event; the
  // auto-reconnect handler then calls esp_wifi_connect() in a tight loop,
  // keeping the radio busy and preventing WiFi scans (used by Improv) from
  // completing. With wifi.enabled=false the radio stays in STA mode (set by
  // initWiFi) so scanning still works, but no connection is attempted until
  // Improv (or the web UI after a restart) supplies real credentials via
  // enableWiFi().
  const bool hasWifiCredentials = (strlen(cfg.getWifiSSID()) > 0);
  ewmCfg.wifi.enabled  = hasWifiCredentials && (mode != InterfaceMode::ETH_ONLY);

  if (ethHardwareAvailable) {
    ewmCfg.ethernet.enabled = true;

    if (board.ethPHY == 0xFF) {
      // ── Internal EMAC mode (ESP32 classic RMII) ───────────────────────
#if ETHWIFI_INTERNAL_EMAC
      ewmCfg.ethernet.mode         = EthWiFiManager::EthernetMode::InternalEmac;

      // Map board template PHY type (html.h numbering) to EthWiFiManager enum.
      // html.h: 0=LAN8720, 1=IP101, 2=RTL8201, 3=DP83848, 4=KSZ8041, 5=KSZ8081
      // EthWiFiManager::EmacPhyChip: IP101=0, RTL8201=1, LAN8720=2, DP83848=3, KSZ8041=4, KSZ8081=5
      static constexpr EthWiFiManager::EmacPhyChip phyMap[] = {
        EthWiFiManager::EmacPhyChip::LAN8720,   // html 0
        EthWiFiManager::EmacPhyChip::IP101,      // html 1
        EthWiFiManager::EmacPhyChip::RTL8201,    // html 2
        EthWiFiManager::EmacPhyChip::DP83848,    // html 3
        EthWiFiManager::EmacPhyChip::KSZ8041,    // html 4
        EthWiFiManager::EmacPhyChip::KSZ8081,    // html 5
      };
      ewmCfg.ethernet.emacPhyChip  = (board.ethPhyType < sizeof(phyMap)/sizeof(phyMap[0]))
                                     ? phyMap[board.ethPhyType]
                                     : EthWiFiManager::EmacPhyChip::LAN8720;
      ewmCfg.ethernet.emacPhyAddr  = board.ethPhyAddr;
      ewmCfg.ethernet.emacMdcPin   = (gpio_num_t)board.ethMDC;
      ewmCfg.ethernet.emacMdioPin  = (gpio_num_t)board.ethMDIO;
      ewmCfg.ethernet.emacRmiiRefClkPin      = (gpio_num_t)board.ethRefClk;
      ewmCfg.ethernet.emacRmiiClockExtInput  = board.ethClkExt;
      if (board.ethPhyRST != UNUSED_PIN)
        ewmCfg.ethernet.emacPhyResetPin = (gpio_num_t)board.ethPhyRST;
      LOG_CONSOLE(PSTR("[ETH] Config: InternalEmac PHYtype=%d addr=%d MDC=%d MDIO=%d RefClk=%d ClkExt=%d RST=%d"),
        board.ethPhyType, board.ethPhyAddr, board.ethMDC, board.ethMDIO,
        board.ethRefClk, (int)board.ethClkExt, board.ethPhyRST);

      // On boards like ESP32-ETH01, GPIO16 enables an external 50 MHz oscillator
      // (active-high, with 10 K pull-down → OFF at boot).  The oscillator must be
      // running BEFORE esp_eth_mac_new_esp32() / mac->init() is called — if it
      // isn't, the EMAC DMA soft-reset never completes → "reset timeout".
      // drive HIGH here, well before begin().
      if (board.ethOscEN != UNUSED_PIN) {
        LOG_CONSOLE(PSTR("[ETH] Enabling oscillator on GPIO %d"), board.ethOscEN);
        pinMode(board.ethOscEN, OUTPUT);
        digitalWrite(board.ethOscEN, HIGH);
        delay(5); // oscillator startup time (typ < 2 ms)
      }

      // Active-low PHY hardware reset: pulse LOW→HIGH so the PHY is fully out
      // of reset before begin().  The library will also reset it during
      // phy->init(), but doing it here ensures the oscillator (if driven by the
      // PHY's REFCLKO) is stable before the MAC init runs.
      if (board.ethPhyRST != UNUSED_PIN) {
        LOG_CONSOLE(PSTR("[ETH] PHY hard-reset on GPIO %d"), board.ethPhyRST);
        pinMode(board.ethPhyRST, OUTPUT);
        digitalWrite(board.ethPhyRST, LOW);
        delay(10);
        digitalWrite(board.ethPhyRST, HIGH);
        delay(150); // PHY PLL lock time
      }

      // When using external clock input (EMAC_CLK_EXT_IN), configure the
      // REF_CLK pin as a plain input and give the oscillator time to stabilise.
      if (board.ethClkExt) {
        pinMode(board.ethRefClk, INPUT);
        delay(50);
      }
#else
      LOG_CONSOLE(PSTR("[ETH] InternalEmac requested but not supported on this chip — Ethernet disabled"));
      ewmCfg.ethernet.enabled = false;
#endif
    } else {
      // ── SPI Ethernet module ────────────────────────────────────────────
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

      // Explicitly set SPI mode to override the InternalEmac default on ESP32 classic.
#if ETHWIFI_INTERNAL_EMAC
      ewmCfg.ethernet.mode = EthWiFiManager::EthernetMode::Spi;
#endif

      // Map board ethPHY to SpiModule enum: 0=W5500, 1=DM9051, 2=KSZ8851SNL
      switch (board.ethPHY) {
        case 1:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::DM9051; break;
        case 2:  ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::KSZ8851SNL; break;
        default: ewmCfg.ethernet.spiModule = EthWiFiManager::SpiModule::W5500; break;
      }
      // SPI bus: 2=SPI2_HOST, 3=SPI3_HOST (ESP32-C3 only has SPI2_HOST)
#if defined(SPI3_HOST)
      ewmCfg.ethernet.spiHost = (board.ethSPI == 3) ? SPI3_HOST : SPI2_HOST;
#else
      ewmCfg.ethernet.spiHost = SPI2_HOST;
#endif
      ewmCfg.ethernet.csPin  = (gpio_num_t)board.ethCS;
      ewmCfg.ethernet.intPin = (gpio_num_t)board.ethINT;

      // SPI data pins: use dedicated ethernet pins if defined, else share with radio
      if (board.ethMISO != UNUSED_PIN && board.ethMOSI != UNUSED_PIN && board.ethSCK != UNUSED_PIN) {
        ewmCfg.ethernet.misoPin = (gpio_num_t)board.ethMISO;
        ewmCfg.ethernet.mosiPin = (gpio_num_t)board.ethMOSI;
        ewmCfg.ethernet.sckPin  = (gpio_num_t)board.ethSCK;
      } else {
        ewmCfg.ethernet.misoPin = (gpio_num_t)board.L_MISO;
        ewmCfg.ethernet.mosiPin = (gpio_num_t)board.L_MOSI;
        ewmCfg.ethernet.sckPin  = (gpio_num_t)board.L_SCK;
      }

      LOG_CONSOLE(PSTR("[ETH] Config: SPI PHY=%d CS=%d INT=%d RST=%d MISO=%d MOSI=%d SCK=%d bus=%d"),
        board.ethPHY, board.ethCS, board.ethINT, board.ethRST,
        ewmCfg.ethernet.misoPin, ewmCfg.ethernet.mosiPin, ewmCfg.ethernet.sckPin,
        board.ethSPI);
    }
  } else {
    ewmCfg.ethernet.enabled = false;
    LOG_CONSOLE(PSTR("[ETH] Disabled (ethEN=false in board template)"));
  }

  // Register event handler
  _ewm.onEvent([this](EthWiFiManager::Event ev, IPAddress ip) {
    onEthEvent(ev, ip);
  });

  if (!_ewm.begin(ewmCfg)) {
    LOG_CONSOLE(PSTR("[NET] EthWiFiManager begin() failed — network may be limited"));
  }

  // If Ethernet hardware failed during begin() (EthernetDisabled event fired
  // synchronously), override mode to WIFI_ONLY now, before the switch below,
  // and re-enable WiFi which may have been disabled in ewmCfg.
  if (_runtimeEthFailed && mode != InterfaceMode::WIFI_ONLY) {
    LOG_CONSOLE(PSTR("[NET] Ethernet init failed — forcing Interface Mode to WIFI_ONLY"));
    mode = InterfaceMode::WIFI_ONLY;
    EthWiFiManager::WiFiConfig wCfg;
    wCfg.ssid          = cfg.getWifiSSID();
    wCfg.password      = cfg.getWifiPassword();
    wCfg.autoReconnect = true;
    _ewm.enableWiFi(wCfg);
  }

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
      // Schedule AP close after a grace period so any connected client can finish.
      if (_ewm.isAPActive() && !_apModeForced && _apCloseAfterConnectTime == 0)
        _apCloseAfterConnectTime = millis();
      if (ConfigStore::getInstance().getAlwaysAP()) startAlwaysAP();
      notifyConnected(ip, ActiveInterface::WIFI);
      break;

    case EthWiFiManager::Event::EthGotIP:
      LOG_CONSOLE(PSTR("Ethernet connected, IP: %s"), ip.toString().c_str());
      // Schedule AP close after a grace period so any connected client can finish.
      if (_ewm.isAPActive() && !_apModeForced && _apCloseAfterConnectTime == 0)
        _apCloseAfterConnectTime = millis();
      _ethConnectStartTime = 0; // cancel the ETH-only AP rescue timeout
      _activeInterface = ActiveInterface::ETHERNET;
      _localIP = ip;
      _state = ConnState::CONNECTED;
      if (ConfigStore::getInstance().getAlwaysAP()) startAlwaysAP();
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
          _localIP = IPAddress();
          // Go back to CONNECTING and restart the AP rescue timer so that if
          // the cable stays unplugged for ETH_CONNECT_TIMEOUT_MS the AP fires.
          _state = ConnState::CONNECTING;
          _ethConnectStartTime = millis();
          notifyDisconnected();
        }
      }
      break;

    case EthWiFiManager::Event::EthernetDisabled:
      // Ethernet hardware failed to initialise. Set the flag so that
      // setupEthWiFiManager() — which is still on the call stack — can
      // override the InterfaceMode and re-enable WiFi after begin() returns.
      _runtimeEthFailed = true;
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

  // Keep DNS running while AP is active (covers both AP_MODE and the grace
  // period after connectivity is restored).
  if (_ewm.isAPActive())
    _dnsServer.processNextRequest();

  // AP mode timeout (no connectivity yet — try reconnecting WiFi)
  if (_state == ConnState::AP_MODE && !_apModeForced) {
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

  // Delayed AP close: connectivity was restored; keep AP up for AP_CLOSE_DELAY_MS
  // so any client already connected can finish configuring, then close it.
  if (_apCloseAfterConnectTime != 0 &&
      millis() - _apCloseAfterConnectTime > AP_CLOSE_DELAY_MS) {
    if (_ewm.isAPActive() && !_apModeForced) {
      LOG_CONSOLE(PSTR("Closing fallback AP (grace period elapsed)"));
      _ewm.disableAP();
      _dnsServer.stop();
    }
    _apCloseAfterConnectTime = 0;
  }

  // Connection timeout: if WiFi hasn't connected, fall back to AP mode.
  // Skip in ETH_ONLY mode — Ethernet has its own (longer) timeout below.
  if (_state == ConnState::CONNECTING && _ewm.isWiFiEnabled()) {
    if (millis() - _connectStartTime > CONNECT_TIMEOUT_MS) {
      LOG_CONSOLE(PSTR("WiFi connection timeout, starting AP mode..."));
      _ewm.disableWiFi();   // stops EthWiFiManager's internal reconnect loop
      setupAP();
    }
  }

  // ETH-only timeout: if Ethernet never came up, start AP rescue so the user
  // can reconfigure (e.g. wrong PHY settings).  _ethConnectStartTime is only
  // set when mode == ETH_ONLY, so this never fires in WiFi/BOTH modes.
  if (_state == ConnState::CONNECTING && _ethConnectStartTime != 0) {
    if (millis() - _ethConnectStartTime > ETH_CONNECT_TIMEOUT_MS) {
      LOG_CONSOLE(PSTR("Ethernet connection timeout, starting AP rescue mode..."));
      _ethConnectStartTime = 0;
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
  if (enable && !_ewm.isAPActive()) {
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
