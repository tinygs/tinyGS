# TinyGS — Rediseño de la Capa de Conectividad

Rama: `refactor/connection-layer` | Repo: `G4lile0/tinyGS`

---

## Contexto

**TinyGS** es una estación terrestre de bajo coste para satélites LoRa basada en ESP32. El proyecto original usaba **IotWebConf2** (WebServer Arduino + WiFi + EEPROM) y **PubSubClient** (MQTT). Se ha hecho un rediseño completo de la capa de conectividad para usar herramientas nativas más robustas.

**Objetivos:**

- WiFi + Ethernet con failover automático (EthWiFiManager)
- HTTP server nativo (`esp_http_server` de ESP-IDF)
- MQTT nativo (`esp-mqtt` de ESP-IDF)
- Almacenamiento NVS en lugar de EEPROM
- Soporte para los 5 targets: `ESP32`, `ESP32-S3`, `ESP32-S3-USB`, `ESP32-C3`, `ESP32-LILYGO_SX1280`

---

## Arquitectura de 3 capas

```text
L1: EthWiFiManager        → lib externa: https://github.com/gmag11/EthWiFiManager.git
L2: ConnectionManager     → tinyGS/src/Network/ConnectionManager.h/.cpp
L3: WebServer (esp_http)  → tinyGS/src/Network/WebServer.h/.cpp
```

---

## Ficheros nuevos

### `tinyGS/src/Network/INetworkAware.h`

Interfaz abstracta + enums de estado de red. Cualquier módulo que necesite conectividad implementa esta interfaz y se registra como observador en `ConnectionManager`.

```cpp
enum class ActiveInterface : uint8_t { NONE=0, WIFI, ETHERNET };
enum class InterfaceMode : uint8_t { WIFI_ONLY, ETH_ONLY, BOTH };

class INetworkAware {
  virtual void onConnected(IPAddress ip, ActiveInterface iface) = 0;
  virtual void onDisconnected() = 0;
  virtual void onInterfaceChanged(ActiveInterface newIface, IPAddress newIP) {}
};
```

---

### `tinyGS/src/Network/ConfigStore.h/.cpp`

Singleton que reemplaza al antiguo `ConfigManager`. Almacena toda la configuración en **NVS** (namespace `"tinygs"`). Incluye migración automática desde el formato EEPROM de IotWebConf2 (namespace `"iotwebconf"`).

**Constantes importantes:**

```cpp
constexpr auto STATION_NAME_LENGTH = 21;
constexpr auto WIFI_SSID_LENGTH    = 33;
constexpr auto WIFI_PASS_LENGTH    = 65;
constexpr auto MQTT_USER_LENGTH    = 31;
constexpr auto MQTT_PASS_LENGTH    = 31;
constexpr auto MQTT_SERVER_LENGTH  = 31;
constexpr auto AP_PASSWORD_LENGTH  = 32;
constexpr auto TEMPLATE_LEN        = 256;
constexpr auto MODEM_LEN           = 256;
constexpr auto ADVANCED_LEN        = 256;
constexpr auto configVersion       = "0.06"; // bumped para forzar migración

const uint8_t UNUSED_PIN = 255;
const uint8_t UNUSED = UNUSED_PIN;  // alias backward compatibility
```

**Enums de radio y board** (condicionales por `CONFIG_IDF_TARGET_*`):

```cpp
enum RadioModelNum_t {
  RADIO_SX1278 = 1, RADIO_SX1276 = 2,
  RADIO_SX1268 = 5, RADIO_SX1262 = 6, RADIO_SX1280 = 8
};
```

> ⚠️ `Radio.h` NO redefine este enum. Solo existe en `ConfigStore.h`.

**`board_t` struct** incluye campos Ethernet:

```cpp
struct board_t {
  // ... pines LoRa, OLED, botones ...
  uint8_t ethCS; uint8_t ethINT; uint8_t ethRST;
  uint8_t ethMISO; uint8_t ethMOSI; uint8_t ethSCK;
  uint8_t ethSpiModule;  // 0=W5500, 1=DM9051, 2=KSZ8851SNL
};
```

**API pública clave:**

- Getters: `getWifiSSID()`, `getWifiPass()`, `getMqttServer()`, `getMqttPort()`, `getMqttUser()`, `getMqttPass()`, `getThingName()`, `getApPassword()`, `getLat()`, `getLon()`, `getTZ()`, `getLowPower()`, `getOledBright()`
- Setters: `setMqttServer()`, `setMqttUser()`, `setMqttPass()`, `setMqttPort()`, `setThingName()`, `setApPassword()`, `setLat()`, `setLon()`, `setTZ()`
- Buffers con longitud (para Improv): `wifiSSIDBuffer()/wifiSSIDLen()`, `wifiPassBuffer()/wifiPassLen()`, `stationNameBuffer()/stationNameLen()`, `apPasswordBuffer()/apPasswordLen()`
- `init()`, `save()`, `getBoardConfig(board_t&)`, `isFailSafeActive()`, `resetAPConfig()`
- `parseAdvancedConf(json)`, `parseBoardTemplate(json)`, `parseModemStartup(json)`
- `migrateFromEEPROM()`

> ⚠️ `BOARD_NAME_LENGTH` y `BOARD_LENGTH` están **solo en `html.h`**, no en `ConfigStore.h` (evita duplicados).

---

### `tinyGS/src/Network/ConnectionManager.h/.cpp`

Singleton que orquesta la conexión WiFi/Ethernet usando EthWiFiManager (event-driven ESP-IDF). Patrón observador.

```cpp
enum class ConnState : uint8_t {
  DISCONNECTED, AP_MODE, CONNECTING, CONNECTED, SWITCHING_INTERFACE
};
```

**API pública:**

```cpp
void init();
void loop();                         // gestiona DNS captive portal, AP timeout, grace period
bool isConnected()  const;
bool isApMode()     const;
IPAddress getLocalIP() const;
ActiveInterface getActiveInterface() const;
void forceApMode(bool enable);       // botón largo + failsafe
void managedDelay(unsigned long ms); // delay procesando DNS
void registerObserver(INetworkAware*);
void unregisterObserver(INetworkAware*);
```

**Constantes internas:**

```cpp
static constexpr unsigned long AP_TIMEOUT_MS  = 300000; // 5 min
static constexpr unsigned long SWITCH_GRACE_MS = 5000;  // 5 s grace period cambio interfaz
```

**API real de EthWiFiManager** (verificada leyendo el header de la lib):

```cpp
EthWiFiManager::Config ewmCfg;
ewmCfg.wifi.ssid      = cfg.getWifiSSID();
ewmCfg.wifi.password  = cfg.getWifiPass();
ewmCfg.ethernet.enabled   = (board.ethCS != UNUSED_PIN);
ewmCfg.ethernet.csPin     = (gpio_num_t)board.ethCS;
ewmCfg.ethernet.intPin    = (gpio_num_t)board.ethINT;
ewmCfg.ethernet.misoPin   = (gpio_num_t)board.ethMISO;
ewmCfg.ethernet.mosiPin   = (gpio_num_t)board.ethMOSI;
ewmCfg.ethernet.sckPin    = (gpio_num_t)board.ethSCK;
// spiModule: EthWiFiManager::SpiModule::W5500 / DM9051 / KSZ8851SNL
_ewm.begin(ewmCfg);
_ewm.onEvent(cb);
// NO existe _ewm.loop() — event-driven
// _ewm.ethernetHasIP()
// _ewm.activeInterface() → EthWiFiManager::ActiveInterface::WiFi o ::Ethernet
```

---

### `tinyGS/src/Network/WebServer.h/.cpp`

Servidor HTTP nativo con `esp_http_server`, implementa `INetworkAware`.

**Rutas registradas:**

| URL | Métodos | Descripción |
| ----- | --------- | ------------- |
| `/` | GET | Redirect a `/dashboard` o `/config` |
| `/logo.png` | GET | Logo PNG |
| `/config` | GET, POST | Formulario de configuración |
| `/dashboard` | GET | Panel principal |
| `/cs` | GET | AJAX consola (long-poll) |
| `/wm` | GET | AJAX datos worldmap |
| `/firmware` | GET, POST | OTA web |
| `/restart` | GET | Reinicio |

**Comportamiento importante:**

- `onConnected()` llama a `begin()` (que tiene guard `if (_server) return`)
- En modo AP, `onConnected()` **nunca se dispara** → el servidor se arranca también explícitamente en `setup()` de `tinyGS.ino`
- Autenticación: en modo AP se omite; en modo STA requiere Basic Auth (`admin` / `apPassword`)
- `askedWebLogin()` → flag para que MQTT envíe notificación al servidor TinyGS

---

### `tinyGS/src/Mqtt/MQTT_Client.h/.cpp`

Reescrito completo usando `esp-mqtt` nativo. Implementa `INetworkAware`.

**Compatibilidad de API esp-mqtt entre versiones de IDF:**

| Plataforma | IDF | Formato `esp_mqtt_client_config_t` |
| ----------- | ----- | ------------------------------------ |
| `espressif32 @ 6.10.0` (ESP32, C3) | 4.4.x | **Flat**: `.uri`, `.cert_pem`, `.username`, `.password`, `.lwt_topic`, etc. |
| `pioarduino stable` (S3, LILYGO) | 5.x | **Nested**: `.broker.address.uri`, `.broker.verification.certificate`, `.credentials.username`, `.credentials.authentication.password`, `.session.last_will.topic`, etc. |

Resuelto con `#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)` en `begin()`. Requiere `#include <esp_idf_version.h>`.

**Características:**

- `_inTopic` / `_inPayload` (String) para ensamblar mensajes fragmentados por esp-mqtt
- `RxPacketMessage` struct + `QueueHandle_t rxQueue` (tamaño 10) para envío asíncrono de paquetes LoRa
- `SemaphoreHandle_t radioConfigMutex` para acceso concurrente a config de radio
- `static void mqttEventHandler(...)` como callback C para esp-mqtt
- URI MQTT: `"mqtts://" + server + ":" + port` (TLS siempre activo)
- Certificados: `newRoot_CA` (TinyGS custom) en `tinyGS/src/certs.h`
- `MQTT_CONNECTED` → `subscribeToAll()` + `sendWelcome()`
- 20+ handlers de comandos remotos en `manageMQTTData()`

---

### `tinyGS/src/Network/html.h`

Copiado desde el antiguo `ConfigManager/html.h`. Contiene constantes PROGMEM para HTML/JS del panel web. **Añadido include guard** `#ifndef TINYGS_HTML_H` para evitar redefinición (se incluye tanto desde `ConfigStore.h` como `WebServer.h`).

Los nombres `IOTWEBCONF_*` se mantienen como identificadores — **no hay dependencia de la librería IotWebConf2**.

---

## Ficheros modificados

### `tinyGS/tinyGS.ino` — Reescrito completamente

```cpp
// Singletons globales al inicio del fichero:
ConfigStore&       configStore = ConfigStore::getInstance();
ConnectionManager& connMgr     = ConnectionManager::getInstance();
MQTT_Client&       mqtt         = MQTT_Client::getInstance();
Radio&             radio        = Radio::getInstance();

// setup():
configStore.init();
if (/*MQTT creds missing*/) mqttCredentials.generateOTPCode();
displayInit(); displayShowInitialCredits();
connMgr.init();
connMgr.registerObserver(&mqtt);
webServer.begin();               // SIEMPRE, antes de registerObserver
connMgr.registerObserver(&webServer);
connMgr.managedDelay(1000);
radio.init();

// loop() — flujo basado en estados:
connMgr.loop();
if (failsafe) { OTA::update(); forceAP; return; }
ArduinoOTA.handle(); handleSerial();
if (apMode)    { displayShowApMode(); return; }
if (!mqttCreds){ mqttAutoconf(); return; }
checkButton(); radio.listen();
if (!connected){ displayShowStaMode(); return; }
// primera conexión: setupNTP(), displayShowConnected(), arduino_ota_setup()
mqtt.loop(); OTA::loop(); displayUpdate(); radio.tle();
```

`mqttAutoconf()`: usa `mqttCredentials.fetchCredentials()` y escribe en ConfigStore con setters + `save()`.

`checkButton()`: pulsación larga (>8s) → `configStore.resetAPConfig()` + `connMgr.forceApMode(true)`.

### `tinyGS/src/Status.h`

```cpp
// Campos añadidos:
uint8_t activeInterface = 0;
char localIP[16] = "";
```

### `tinyGS/src/Radio/Radio.h`

- Eliminado enum `RadioModelNum` duplicado (ya está en `ConfigStore.h` como `RadioModelNum_t`)
- `#include "../Mqtt/MQTT_Client.h"` (era `MQTT_Client_new.h`)

### `tinyGS/src/Display/Display.h/.cpp`, `src/Radio/Radio.cpp`, `src/Power/Power.h/.cpp`, `src/OTA/OTA.cpp`, `src/Mqtt/MQTT_credentials.h`

- `#include "../ConfigManager/ConfigManager.h"` → `#include "../Network/ConfigStore.h"`
- `ConfigManager::getInstance()` → `ConfigStore::getInstance()`
- `configManager.` → `configStore.`
- `getWiFiSSID()` → `getWifiSSID()`

### `tinyGS/src/Improv/tinygs_improv.h/.cpp`

- Imports de ConfigStore + ConnectionManager
- `getLocalUrl()` usa `ConnectionManager::getInstance().getLocalIP()`
- `connectWifi()` usa buffers de ConfigStore
- **Fix bug web installer:** `GET_CURRENT_STATE` comprueba `strlen(globalConfigStore->getWifiSSID()) > 0` en lugar de `WiFi.status() == WL_CONNECTED` (ver sección de bugs)

### `platformio.ini`

```ini
[env]
lib_deps =
    improv/Improv@^1.2.4
    jgromes/RadioLib @ 7.5.0
    https://github.com/gmag11/EthWiFiManager.git
# Eliminadas: -DMQTT_MAX_PACKET_SIZE=1000, -DIOTWEBCONF_DEBUG_DISABLED=0
```

---

## Ficheros eliminados / movidos

| Fichero | Estado |
| --------- | -------- |
| `tinyGS/src/ConfigManager/` | Movido a `/tmp/ConfigManager.old.bak` (fuera del proyecto) |
| `tinyGS/src/Mqtt/MQTT_Client.cpp.old` | Eliminado |
| `tinyGS/src/Mqtt/MQTT_Client.h.old` | Eliminado |
| `tinyGS/src/Mqtt/MQTT_Client_new.cpp` | Renombrado a `MQTT_Client.cpp` |
| `tinyGS/src/Mqtt/MQTT_Client_new.h` | Renombrado a `MQTT_Client.h` |

> La librería `lib/IotWebConf2/` **sigue físicamente en el repo** pero no se incluye en ningún `.cpp`/`.h` activo. PlatformIO no la compila.

---

## Plataformas y versiones de IDF

| Entorno PIO | Plataforma | IDF | esp-mqtt API |
| ------------- | --------- | ----- | ------------- |
| `ESP32` | `espressif32 @ 6.10.0` | 4.4.x | Flat struct |
| `ESP32-C3` | `espressif32 @ 6.10.0` | 4.4.x | Flat struct |
| `ESP32-S3` | `pioarduino stable` | 5.x | Nested struct |
| `ESP32-S3-USB` | `pioarduino stable` | 5.x | Nested struct |
| `ESP32-LILYGO_SX1280` | `pioarduino stable` | 5.x | Nested struct |

> **EthWiFiManager** requiere que `gmag11/EthWiFiManager` en GitHub tenga soporte para ambos IDF 4.x y 5.x. El autor lo confirmó y actualizó durante esta sesión de trabajo.

---

## Estado actual

### ✅ Compilación

Todos los 5 targets compilan correctamente.

### ✅ Flasheado

ESP32-S3 y ESP32-S3-USB flasheados y verificados arrancando en modo AP.

### 🔧 Bug resuelto: WebServer no disponible en modo AP

`onConnected()` nunca se dispara en modo AP puro (no hay IP STA). **Fix**: `webServer.begin()` se llama explícitamente en `setup()` antes de `registerObserver()`.

### 🔧 Bug resuelto: Web installer (Improv) no mostraba formulario WiFi

En hardware con credenciales WiFi previas en NVS del chip, `WiFi.status() == WL_CONNECTED` era `true` aunque TinyGS no tuviera configuración propia. Improv respondía `STATE_PROVISIONED` y el web installer mostraba "ya configurado" en lugar del formulario. **Fix** en `tinygs_improv.cpp`:

```cpp
// ANTES (bug):
if (WiFi.status() == WL_CONNECTED) {
    set_state(STATE_PROVISIONED);
    ...
} else {
    set_state(STATE_AUTHORIZED); // nunca llegaba aquí
}

// DESPUÉS (fix):
bool hasCredentials = (globalConfigStore->getWifiSSID() != nullptr &&
                       strlen(globalConfigStore->getWifiSSID()) > 0);
if (hasCredentials && WiFi.status() == WL_CONNECTED) {
    set_state(STATE_PROVISIONED);
    ...
} else {
    set_state(STATE_AUTHORIZED); // correcto en primer arranque
}
```

### ⏳ Pendiente de verificar en runtime

- Flujo completo: Improv → guardar WiFi → reconexión STA → MQTT autoconf → recepción de paquetes LoRa
- Panel web en modo AP (formulario de config) y modo STA (dashboard)
- Comandos remotos MQTT (set position, modem config, sat filter, sleep, etc.)
- Failover Ethernet → WiFi (requiere hardware con módulo Ethernet)
- Migración NVS desde instalaciones antiguas con IotWebConf2 (primera vez que flasheen la nueva versión)
