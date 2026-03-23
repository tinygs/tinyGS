/*
  MQTT_Client.cpp - MQTT client using native ESP-IDF esp-mqtt

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

#include "MQTT_Client.h"
#include <esp_idf_version.h>
#include "ArduinoJson.h"
#if ARDUINOJSON_USE_LONG_LONG == 0 && !PLATFORMIO
#error "Using Arduino IDE is not recommended"
#endif
#include "mbedtls/base64.h"
#include "../Radio/Radio.h"
#include "../OTA/OTA.h"
#include "../Logger/Logger.h"
#include "../certs.h"
#include "../Network/ConnectionManager.h"
#include <esp_ota_ops.h>
#include <WiFi.h>

MQTT_Client::MQTT_Client() {
  randomTime = random(randomTimeMax - randomTimeMin) + randomTimeMin;
  radioConfigMutex = xSemaphoreCreateMutex();
  rxQueue = xQueueCreate(RX_QUEUE_SIZE, sizeof(RxPacketMessage));
  if (rxQueue == NULL) {
    LOG_CONSOLE(PSTR("ERROR: Failed to create RX packet queue"));
  }
}

// ---- INetworkAware ----
void MQTT_Client::onConnected(IPAddress ip, ActiveInterface iface) {
  _networkAvailable = true;
  connectionAttempts = 0; // reset on every network-level reconnect
  ConfigStore& cfg = ConfigStore::getInstance();
  if (cfg.getMqttServer()[0] == '\0' ||
      cfg.getMqttUser()[0]   == '\0' ||
      cfg.getMqttPass()[0]   == '\0') {
    LOG_CONSOLE(PSTR("MQTT: credentials not set, skipping connection"));
    return;
  }
  if (!_client) {
    // Apply random jitter (10–20 s) so not all stations hit the server simultaneously
    _pendingConnectAt = millis() + randomTime;
    LOG_CONSOLE(PSTR("MQTT: connecting in %lu ms (jitter)"), randomTime);
  } else {
    esp_mqtt_client_start(_client);
  }
}

void MQTT_Client::onDisconnected() {
  _networkAvailable = false;
  _mqttConnected = false;
  status.mqtt_connected = false;
  if (_client) {
    esp_mqtt_client_stop(_client);
  }
}

void MQTT_Client::onInterfaceChanged(ActiveInterface iface, IPAddress ip) {
  if (!_client) return;
  // esp-mqtt handles reconnection internally
  LOG_CONSOLE(PSTR("MQTT: Network interface changed, reconnecting..."));
  esp_mqtt_client_reconnect(_client);
}

// ---- esp-mqtt event handler (static) ----
void MQTT_Client::mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  MQTT_Client* self = (MQTT_Client*)handler_args;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  self->handleMqttEvent(event);
}

void MQTT_Client::handleMqttEvent(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      LOG_CONSOLE(PSTR("Connected to MQTT!"));
      _mqttConnected = true;
      status.mqtt_connected = true;
      connectionAttempts = 0;
      lastPing = millis(); // reset ping timer so loop() won't fire a ping before welcome
      subscribeToAll();
      sendWelcome();
      break;

    case MQTT_EVENT_DISCONNECTED:
      LOG_CONSOLE(PSTR("MQTT disconnected"));
      _mqttConnected = false;
      status.mqtt_connected = false;
      // Only count as a genuine MQTT failure when the network is actually up.
      // If WiFi/ETH is down, the disconnect is caused by the transport loss —
      // incrementing here would trip the restart watchdog spuriously.
      if (_networkAvailable) {
        connectionAttempts++;
      }
      break;

    case MQTT_EVENT_DATA:
      // Handle potentially fragmented messages
      if (event->topic_len > 0) {
        // First (or only) fragment - capture topic
        _inTopic = String(event->topic).substring(0, event->topic_len);
        _inPayload = "";
      }
      // Append payload data
      if (event->data_len > 0) {
        _inPayload += String(event->data).substring(0, event->data_len);
      }
      // Check if message is complete
      if (event->data_len + event->current_data_offset >= event->total_data_len) {
        // Full message received
        manageMQTTData(
          (char*)_inTopic.c_str(),
          (uint8_t*)_inPayload.c_str(),
          _inPayload.length()
        );
        _inTopic = "";
        _inPayload = "";
      }
      break;

    case MQTT_EVENT_ERROR:
      if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        LOG_ERROR(PSTR("MQTT transport error"));
      } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        LOG_CONSOLE(PSTR("MQTT connection refused, code: %d"), event->error_handle->connect_return_code);
      }
      break;

    default:
      break;
  }
}

void MQTT_Client::begin() {
  ConfigStore& cfg = ConfigStore::getInstance();

  if (_client) {
    esp_mqtt_client_destroy(_client);
    _client = nullptr;
  }

  uint64_t chipId = ESP.getEfuseMac();
  char clientId[13];
  sprintf(clientId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

  // Build LWT topic
  String lwtTopic = buildTopic(teleTopic, topicStatus);

  // Build MQTT URI: mqtts://server:port
  String uri = "mqtts://" + String(cfg.getMqttServer()) + ":" + String(cfg.getMqttPort());

  esp_mqtt_client_config_t mqtt_cfg = {};

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // ESP-IDF 5.x: nested struct format
  mqtt_cfg.broker.address.uri = uri.c_str();
  mqtt_cfg.broker.verification.certificate = newRoot_CA;
  mqtt_cfg.credentials.username = cfg.getMqttUser();
  mqtt_cfg.credentials.authentication.password = cfg.getMqttPass();
  mqtt_cfg.credentials.client_id = clientId;
  mqtt_cfg.session.last_will.topic = lwtTopic.c_str();
  mqtt_cfg.session.last_will.msg = "0";
  mqtt_cfg.session.last_will.msg_len = 1;
  mqtt_cfg.session.last_will.qos = 2;
  mqtt_cfg.session.last_will.retain = false;
  mqtt_cfg.network.reconnect_timeout_ms = reconnectionInterval;
  mqtt_cfg.buffer.size = 2048;
  mqtt_cfg.buffer.out_size = 2048;
#else
  // ESP-IDF 4.x: flat struct format
  mqtt_cfg.uri = uri.c_str();
  mqtt_cfg.cert_pem = newRoot_CA;
  mqtt_cfg.username = cfg.getMqttUser();
  mqtt_cfg.password = cfg.getMqttPass();
  mqtt_cfg.client_id = clientId;
  mqtt_cfg.lwt_topic = lwtTopic.c_str();
  mqtt_cfg.lwt_msg = "0";
  mqtt_cfg.lwt_msg_len = 1;
  mqtt_cfg.lwt_qos = 2;
  mqtt_cfg.lwt_retain = 0;
  mqtt_cfg.reconnect_timeout_ms = reconnectionInterval;
  mqtt_cfg.buffer_size = 2048;
  mqtt_cfg.out_buffer_size = 2048;
#endif

  _client = esp_mqtt_client_init(&mqtt_cfg);
  if (!_client) {
    LOG_ERROR(PSTR("Failed to init MQTT client"));
    return;
  }

  esp_mqtt_client_register_event(_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqttEventHandler, this);
  esp_mqtt_client_start(_client);
  LOG_CONSOLE(PSTR("MQTT client started, connecting to %s:%u..."), cfg.getMqttServer(), cfg.getMqttPort());
}

void MQTT_Client::loop() {
  if (!_networkAvailable) return;

  // Skip all MQTT logic if credentials are not configured yet
  {
    ConfigStore& cfg = ConfigStore::getInstance();
    if (cfg.getMqttServer()[0] == '\0' ||
        cfg.getMqttUser()[0]   == '\0' ||
        cfg.getMqttPass()[0]   == '\0') return;
  }

  // Credentials just became available (e.g. after OTP autoconfig) — start the client
  // or jitter timer has elapsed from onConnected()
  if (!_client) {
    if (_pendingConnectAt == 0)
      _pendingConnectAt = millis() + randomTime; // first time we see credentials, arm the timer
    if (millis() >= _pendingConnectAt) {
      _pendingConnectAt = 0;
      begin();
    }
    return;
  }

  // Connection timeout handling
  if (!_mqttConnected) {
    status.mqtt_connected = false;
    if (connectionAttempts > connectionTimeout) {
      LOG_CONSOLE(PSTR("Unable to connect to MQTT Server after many attempts. Restarting..."));
      ConfigStore& cfg = ConfigStore::getInstance();
      if (cfg.getLowPower()) {
        Radio& radio = Radio::getInstance();
        uint32_t sleep_seconds = 4 * 3600;
        LOG_DEBUG(PSTR("deep_sleep_enter"));
        esp_sleep_enable_timer_wakeup(1000000ULL * sleep_seconds);
        delay(100);
        Serial.flush();
        WiFi.disconnect(true);
        delay(100);
        radio.moduleSleep();
        esp_deep_sleep_start();
        delay(1000);
      } else {
        ESP.restart();
      }
    }
  }

  // Process RX queue
  processRxQueue();

  // Periodic ping
  unsigned long now = millis();
  if (now - lastPing > pingInterval && _mqttConnected) {
    lastPing = now;
    if (scheduledRestart) {
      sendWelcome();
    } else {
      StaticJsonDocument<192> doc;
      doc["Vbat"] = voltage();
      doc["Mem"] = ESP.getFreeHeap();
      doc["MinMem"] = ESP.getMinFreeHeap();
      doc["MaxBlk"] = ESP.getMaxAllocHeap();

      ConnectionManager& cm = ConnectionManager::getInstance();
      if (cm.getActiveInterface() == ActiveInterface::WIFI) {
        doc["RSSI"] = WiFi.RSSI();
      } else {
        doc["iface"] = cm.getActiveInterfaceName();
      }
      doc["radio"] = status.radio_error;
      doc["InstRSSI"] = status.modeminfo.currentRssi;

      char buffer[256];
      serializeJson(doc, buffer);
      LOG_DEBUG(PSTR("%s"), buffer);
      mqttPublish(buildTopic(teleTopic, topicPing).c_str(), buffer);
    }
  }
}

void MQTT_Client::mqttPublish(const char* topic, const char* data, int len, int qos, bool retain) {
  if (!_client || !_mqttConnected) return;
  if (len == 0) len = strlen(data);
  esp_mqtt_client_publish(_client, topic, data, len, qos, retain ? 1 : 0);
}

String MQTT_Client::buildTopic(const char* baseTopic, const char* cmnd) {
  ConfigStore& cfg = ConfigStore::getInstance();
  static char topicBuffer[128];

  const char* user = cfg.getMqttUser();
  const char* station = cfg.getThingName();

  char* p = topicBuffer;
  const char* src = baseTopic;

  while (*src) {
    if (strncmp(src, "%user%", 6) == 0) {
      strcpy(p, user); p += strlen(user); src += 6;
    } else if (strncmp(src, "%station%", 9) == 0) {
      strcpy(p, station); p += strlen(station); src += 9;
    } else if (strncmp(src, "%cmnd%", 6) == 0) {
      strcpy(p, cmnd); p += strlen(cmnd); src += 6;
    } else {
      *p++ = *src++;
    }
  }
  *p = '\0';
  return String(topicBuffer);
}

void MQTT_Client::subscribeToAll() {
  if (!_client) return;
  esp_mqtt_client_subscribe(_client, buildTopic(globalTopic, "#").c_str(), 0);
  esp_mqtt_client_subscribe(_client, buildTopic(cmndTopic, "#").c_str(), 0);
}

void MQTT_Client::sendWelcome() {
  scheduledRestart = false;
  ConfigStore& cfg = ConfigStore::getInstance();
  ConnectionManager& cm = ConnectionManager::getInstance();
  time_t now;
  time(&now);

  uint64_t chipId = ESP.getEfuseMac();
  char clientId[13];
  sprintf(clientId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(20) + 512;
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(cfg.getLatitude());
  station_location.add(cfg.getLongitude());
  doc["tx"] = cfg.getAllowTx();
  doc["time"] = now;
  doc["version"] = status.version;
  doc["git_version"] = status.git_version;
  doc["sat"] = status.modeminfo.satellite;
  doc["ip"] = cm.getLocalIP().toString();
  doc["iface"] = cm.getActiveInterfaceName();
  if (cfg.getLowPower())
    doc["lp"].set(cfg.getLowPower());
  doc["modem_conf"].set(cfg.getModemStartup());
  doc["boardTemplate"].set(cfg.getBoardTemplate());
  doc["Mem"] = ESP.getFreeHeap();
  doc["Size"] = ESP.getSketchSize();
  doc["MD5"] = ESP.getSketchMD5();
  doc["board"] = cfg.getBoard();
  doc["mac"] = clientId;
  doc["seconds"] = millis() / 1000;
  doc["Vbat"] = voltage();
  doc["chip"] = ESP.getChipModel();
  doc["slot"] = esp_ota_get_running_partition()->label;
  doc["pSize"] = esp_ota_get_running_partition()->size;
  doc["idfv"] = esp_get_idf_version();
  board_t board;
  if (cfg.getBoardConfig(board))
    doc["radioChip"] = board.L_radio;

  char buffer[1048];
  serializeJson(doc, buffer);
  mqttPublish(buildTopic(teleTopic, topicWelcome).c_str(), buffer);
}

void MQTT_Client::sendRx(String packet, bool noisy, String raw_packet) {
  ConfigStore& cfg = ConfigStore::getInstance();
  time_t now;
  time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(24) + 256 + packet.length() + raw_packet.length();
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(cfg.getLatitude());
  station_location.add(cfg.getLongitude());
  doc["mode"] = status.modeminfolastpckt.modem_mode;
  doc["frequency"] = status.modeminfolastpckt.frequency;
  doc["frequency_offset"] = status.modeminfolastpckt.freqOffset;
  if (status.lastPacketInfo.freqDoppler != 0) doc["f_doppler"] = status.lastPacketInfo.freqDoppler;
  doc["satellite"] = status.modeminfolastpckt.satellite;

  if (strcmp(status.modeminfolastpckt.modem_mode, "LoRa") == 0) {
    doc["sf"] = status.modeminfolastpckt.sf;
    doc["cr"] = status.modeminfolastpckt.cr;
    doc["bw"] = status.modeminfolastpckt.bw;
    doc["iIQ"] = status.modeminfolastpckt.iIQ;
  } else {
    doc["bitrate"] = status.modeminfolastpckt.bitrate;
    doc["freqdev"] = status.modeminfolastpckt.freqDev;
    doc["rxBw"] = status.modeminfolastpckt.bw;
    doc["data_raw"] = raw_packet;
  }

  doc["rssi"] = status.lastPacketInfo.rssi;
  doc["snr"] = status.lastPacketInfo.snr;
  doc["frequency_error"] = status.lastPacketInfo.frequencyerror;
  doc["unix_GS_time"] = now;
  doc["usec_time"] = (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
  doc["crc_error"] = status.lastPacketInfo.crc_error;
  doc["data"] = packet;
  doc["NORAD"] = status.modeminfolastpckt.NORAD;
  doc["noisy"] = noisy;

  size_t bufferSize = measureJson(doc) + 1;
  char* buffer = (char*)malloc(bufferSize);
  if (!buffer) {
    LOG_ERROR(PSTR("sendRx: Failed to allocate buffer (%u bytes)"), bufferSize);
    return;
  }

  serializeJson(doc, buffer, bufferSize);
  LOG_DEBUG_ASYNC(PSTR("%s"), buffer);
  mqttPublish(buildTopic(teleTopic, topicRx).c_str(), buffer, bufferSize - 1);
  free(buffer);
}

void MQTT_Client::sendRxFromQueue(const RxPacketMessage& msg) {
  ConfigStore& cfg = ConfigStore::getInstance();
  time_t now;
  time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  size_t packetLen = strlen(msg.packet);
  size_t rawLen = strlen(msg.raw_packet);
  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(24) + 256 + packetLen + rawLen;
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(cfg.getLatitude());
  station_location.add(cfg.getLongitude());
  doc["mode"] = msg.modem_mode;
  doc["frequency"] = msg.frequency;
  doc["frequency_offset"] = msg.freqOffset;
  if (msg.freqDoppler != 0) doc["f_doppler"] = msg.freqDoppler;
  doc["satellite"] = msg.satellite;

  if (strcmp(msg.modem_mode, "LoRa") == 0) {
    doc["sf"] = msg.sf;
    doc["cr"] = msg.cr;
    doc["bw"] = msg.bw;
    doc["iIQ"] = msg.iIQ;
  } else {
    doc["bitrate"] = msg.bitrate;
    doc["freqdev"] = msg.freqDev;
    doc["rxBw"] = msg.bw;
    doc["data_raw"] = msg.raw_packet;
  }

  doc["rssi"] = msg.rssi;
  doc["snr"] = msg.snr;
  doc["frequency_error"] = msg.frequencyerror;
  doc["unix_GS_time"] = now;
  doc["usec_time"] = (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
  doc["crc_error"] = msg.crc_error;
  doc["data"] = msg.packet;
  doc["NORAD"] = msg.NORAD;
  doc["noisy"] = msg.noisy;

  size_t bufferSize = measureJson(doc) + 1;
  char* buffer = (char*)malloc(bufferSize);
  if (!buffer) {
    LOG_ERROR(PSTR("sendRxFromQueue: Failed to allocate buffer (%u bytes)"), bufferSize);
    return;
  }

  serializeJson(doc, buffer, bufferSize);
  LOG_DEBUG_ASYNC(PSTR("%s"), buffer);
  mqttPublish(buildTopic(teleTopic, topicRx).c_str(), buffer, bufferSize - 1);
  free(buffer);
}

void MQTT_Client::queueRx(const String& packet, bool noisy, const String& raw_packet) {
  if (rxQueue == NULL) {
    sendRx(packet, noisy, raw_packet);
    return;
  }

  RxPacketMessage msg;
  msg.noisy = noisy;
  strncpy(msg.modem_mode, status.modeminfolastpckt.modem_mode, sizeof(msg.modem_mode) - 1);
  msg.modem_mode[sizeof(msg.modem_mode) - 1] = '\0';
  strncpy(msg.satellite, status.modeminfolastpckt.satellite, sizeof(msg.satellite) - 1);
  msg.satellite[sizeof(msg.satellite) - 1] = '\0';
  msg.frequency = status.modeminfolastpckt.frequency;
  msg.freqOffset = status.modeminfolastpckt.freqOffset;
  msg.NORAD = status.modeminfolastpckt.NORAD;
  msg.sf = status.modeminfolastpckt.sf;
  msg.cr = status.modeminfolastpckt.cr;
  msg.bw = status.modeminfolastpckt.bw;
  msg.iIQ = status.modeminfolastpckt.iIQ;
  msg.bitrate = status.modeminfolastpckt.bitrate;
  msg.freqDev = status.modeminfolastpckt.freqDev;
  msg.rssi = status.lastPacketInfo.rssi;
  msg.snr = status.lastPacketInfo.snr;
  msg.frequencyerror = status.lastPacketInfo.frequencyerror;
  msg.freqDoppler = status.lastPacketInfo.freqDoppler;
  msg.crc_error = status.lastPacketInfo.crc_error;

  size_t packetLen = packet.length();
  size_t rawLen = raw_packet.length();
  if (packetLen >= sizeof(msg.packet)) packetLen = sizeof(msg.packet) - 1;
  if (rawLen >= sizeof(msg.raw_packet)) rawLen = sizeof(msg.raw_packet) - 1;

  memcpy(msg.packet, packet.c_str(), packetLen);
  msg.packet[packetLen] = '\0';
  memcpy(msg.raw_packet, raw_packet.c_str(), rawLen);
  msg.raw_packet[rawLen] = '\0';

  if (xQueueSend(rxQueue, &msg, 0) != pdTRUE) {
    RxPacketMessage discarded;
    xQueueReceive(rxQueue, &discarded, 0);
    if (xQueueSend(rxQueue, &msg, 0) == pdTRUE) {
      LOG_DEBUG_ASYNC(PSTR("RX queue full, oldest packet discarded"));
    } else {
      LOG_DEBUG_ASYNC(PSTR("RX queue error, packet lost"));
    }
  }
}

void MQTT_Client::processRxQueue() {
  if (rxQueue == NULL || !_mqttConnected) return;

  RxPacketMessage msg;
  int processed = 0;
  while (processed < 2 && xQueueReceive(rxQueue, &msg, 0) == pdTRUE) {
    sendRxFromQueue(msg);
    processed++;
  }

  UBaseType_t pending = uxQueueMessagesWaiting(rxQueue);
  if (pending > 0) {
    LOG_DEBUG_ASYNC(PSTR("RX queue: %u packets pending"), pending);
  }
}

void MQTT_Client::sendStatus() {
  ConfigStore& cfg = ConfigStore::getInstance();
  time_t now;
  time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  StaticJsonDocument<512> doc;
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(cfg.getLatitude());
  station_location.add(cfg.getLongitude());

  doc["version"] = status.version;
  doc["board"] = cfg.getBoard();
  doc["tx"] = cfg.getAllowTx();
  doc["mode"] = status.modeminfo.modem_mode;
  doc["frequency"] = status.modeminfo.frequency;
  doc["frequency_offset"] = status.modeminfo.freqOffset;
  doc["satellite"] = status.modeminfo.satellite;
  doc["NORAD"] = status.modeminfo.NORAD;

  if (strcmp(status.modeminfo.modem_mode, "LoRa") == 0) {
    doc["sf"] = status.modeminfo.sf;
    doc["cr"] = status.modeminfo.cr;
    doc["bw"] = status.modeminfo.bw;
  } else {
    doc["bitrate"] = status.modeminfo.bitrate;
    doc["freqdev"] = status.modeminfo.freqDev;
    doc["rxBw"] = status.modeminfo.bw;
  }

  doc["pl"] = status.modeminfo.preambleLength;
  doc["CRC"] = status.modeminfo.crc;
  doc["FLDRO"] = status.modeminfo.fldro;
  doc["OOK"] = status.modeminfo.OOK;
  doc["rssi"] = status.lastPacketInfo.rssi;
  doc["snr"] = status.lastPacketInfo.snr;
  doc["frequency_error"] = status.lastPacketInfo.frequencyerror;
  doc["crc_error"] = status.lastPacketInfo.crc_error;
  doc["unix_GS_time"] = now;
  doc["usec_time"] = (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;

  char buffer[1024];
  serializeJson(doc, buffer);
  mqttPublish(buildTopic(statTopic, topicStatus).c_str(), buffer);
}

void MQTT_Client::sendAdvParameters() {
  ConfigStore& cfg = ConfigStore::getInstance();
  StaticJsonDocument<512> doc;
  doc["adv_prm"].set(cfg.getAdvancedConfig());
  char buffer[512];
  serializeJson(doc, buffer);
  LOG_DEBUG(PSTR("%s"), buffer);
  mqttPublish(buildTopic(teleTopic, topicGet_adv_prm).c_str(), buffer);
}

void MQTT_Client::sendWeblogin() {
  LOG_DEBUG(PSTR("Asking for weblogin link"));
  mqttPublish(buildTopic(teleTopic, "get_weblogin").c_str(), "1");
}

bool isValidFrequency(uint8_t radio, float f) {
  return !((radio == 1 && (f < 137 || f > 525)) ||
           (radio == 2 && (f < 137 || f > 1020)) ||
           (radio == 5 && (f < 410 || f > 810)) ||
           (radio == 6 && (f < 150 || f > 960)) ||
           (radio == 8 && (f < 2400 || f > 2500)));
}

void MQTT_Client::manageMQTTData(char* topic, uint8_t* payload, unsigned int length) {
  Radio& radio = Radio::getInstance();

  bool global = true;
  char topicCopy[128];
  strncpy(topicCopy, topic, sizeof(topicCopy) - 1);
  topicCopy[sizeof(topicCopy) - 1] = '\0';

  char* command;
  strtok(topicCopy, "/");                  // tinygs
  if (strcmp(strtok(NULL, "/"), "global")) { // user
    global = false;
    strtok(NULL, "/"); // station
  }
  strtok(NULL, "/"); // cmnd
  command = strtok(NULL, "/");
  if (!command) return;

  uint16_t result = 0xFF;

  if (!strcmp(command, commandSatPos)) {
    manageSatPosOled((char*)payload, length);
    return;
  }

  if (!strcmp(command, commandReset))
    ESP.restart();

  if (!strcmp(command, commandUpdate)) {
    OTA::update();
    return;
  }

  if (!strcmp(command, commandWeblogin)) {
    LOG_CONSOLE_ASYNC(PSTR("Weblogin: %.*s"), length, payload);
    return;
  }

  if (!strcmp(command, commandFrame)) {
    char* frameNumStr = strtok(NULL, "/");
    uint8_t frameNumber = frameNumStr ? atoi(frameNumStr) : 0;
    StaticJsonDocument<512> doc;
    deserializeJson(doc, payload, length);
    status.remoteTextFrameLength[frameNumber] = doc.size();

    for (uint8_t n = 0; n < status.remoteTextFrameLength[frameNumber]; n++) {
      status.remoteTextFrame[frameNumber][n].text_font = doc[n][0];
      status.remoteTextFrame[frameNumber][n].text_alignment = doc[n][1];
      status.remoteTextFrame[frameNumber][n].text_pos_x = doc[n][2];
      status.remoteTextFrame[frameNumber][n].text_pos_y = doc[n][3];
      const char* text = doc[n][4].as<const char*>();
      strncpy(status.remoteTextFrame[frameNumber][n].text, text ? text : "",
              sizeof(status.remoteTextFrame[frameNumber][n].text) - 1);
      status.remoteTextFrame[frameNumber][n].text[sizeof(status.remoteTextFrame[frameNumber][n].text) - 1] = '\0';
    }
    result = 0;
  }

  if (!strcmp(command, commandStatus)) {
    sendStatus();
    return;
  }

  if (!strcmp(command, commandLog)) {
    char logStr[length + 1];
    memcpy(logStr, payload, length);
    logStr[length] = '\0';
    LOG_CONSOLE_ASYNC(PSTR("%s"), logStr);
    return;
  }

  if (!strcmp(command, commandTx)) {
    if (!radio.isReady()) {
      LOG_CONSOLE_ASYNC(PSTR("Radio not ready, ignoring TX command"));
      return;
    }
    if (xSemaphoreTake(radioConfigMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: Could not acquire radio config mutex for TX"));
      return;
    }
    result = radio.sendTx(payload, length);
    LOG_CONSOLE_ASYNC(PSTR("Sending TX packet!"));
    xSemaphoreGive(radioConfigMutex);
  }

  // Remote tune commands (station-specific only)
  if (global) return;

  // All station-specific commands below require the radio to be initialized
  if (!radio.isReady()) {
    LOG_CONSOLE_ASYNC(PSTR("Radio not ready, ignoring remote command: %s"), command);
    return;
  }

  if (!strcmp(command, commandBeginp)) {
    char buff[length + 1];
    memcpy(buff, payload, length);
    buff[length] = '\0';

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error.code() != DeserializationError::Ok || !doc.containsKey("mode")) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: Your modem config is invalid. Resetting to default"));
      return;
    }

    board_t board;
    if (!ConfigStore::getInstance().getBoardConfig(board)) return;

    if (!isValidFrequency(board.L_radio, doc["freq"])) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: Invalid frequency. Ignoring."));
      return;
    }
    ModemInfo& m = status.modeminfo;
    m.tle[0] = 0;
    status.tle.freqDoppler = 0;
    status.tle.new_freqDoppler = 0;
    ConfigStore::getInstance().setModemStartup(buff);
  }

  if (!strcmp(command, commandBegine)) {
    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error.code() != DeserializationError::Ok || !doc.containsKey("mode")) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: The received modem configuration is invalid. Ignoring."));
      return;
    }

    board_t board;
    if (!ConfigStore::getInstance().getBoardConfig(board)) return;

    if (!isValidFrequency(board.L_radio, doc["freq"])) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: Invalid frequency. Ignoring."));
      return;
    }

    if (xSemaphoreTake(radioConfigMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
      LOG_CONSOLE_ASYNC(PSTR("ERROR: Could not acquire radio config mutex"));
      return;
    }

    radio.clearPacketReceivedAction();
    radio.disableInterrupt();

    ModemInfo& m = status.modeminfo;
    const char* mode = doc["mode"].as<const char*>();
    strncpy(m.modem_mode, mode ? mode : "", sizeof(m.modem_mode) - 1);
    m.modem_mode[sizeof(m.modem_mode) - 1] = '\0';
    strcpy(m.satellite, doc["sat"].as<const char*>());
    m.NORAD = doc["NORAD"];

    if (strcmp(mode, "LoRa") == 0)
    {
      m.frequency = doc["freq"];
      m.bw = doc["bw"];
      m.sf = doc["sf"];
      m.cr = doc["cr"];
      m.sw = doc["sw"];
      m.power = doc["pwr"];
      m.preambleLength = doc["pl"];
      m.gain = doc["gain"];
      m.crc = doc["crc"];
      m.fldro = doc["fldro"];
      m.iIQ = doc["iIQ"] ? doc["iIQ"].as<bool>() : false;
      m.len = doc["len"] ? doc["len"].as<int>() : 0;
    }
    else
    {
      m.frequency = doc["freq"];
      m.bw = doc["bw"];
      m.bitrate = doc["br"];
      m.freqDev = doc["fd"];
      m.power = doc["pwr"];
      m.preambleLength = doc["pl"];
      m.OOK = doc["ook"];
      m.len = doc["len"];
      m.swSize = doc["fsw"].size();
      for (int i = 0; i < 8; i++)
      {
        if (i < m.swSize)
          m.fsw[i] = doc["fsw"][i];
        else
          m.fsw[i] = 0;
      }
      m.enc = doc["enc"];
      m.whitening_seed = doc["ws"];
      m.framing = doc["fr"];
      m.crc_by_sw = doc["cSw"];
      m.crc_nbytes = doc["cB"];
      m.crc_init = doc["cI"];
      m.crc_poly = doc["cP"];
      m.crc_finalxor = doc["cF"];
      m.crc_refIn = doc["cRI"];
      m.crc_refOut = doc["cRO"];
    }

    // packets Filter
    uint8_t filterSize = doc["filter"].size();
    for (int i = 0; i < 8; i++)
    {
      if (i < filterSize)
        status.modeminfo.filter[i] = doc["filter"][i];
      else
        status.modeminfo.filter[i] = 0;
    }

    // sat tle
    if ((doc.containsKey("tle") && doc["tle"].is<const char*>()) || (doc.containsKey("tlx") && doc["tlx"].is<const char*>())) {

      const char* base64Tle = nullptr;

      status.tle.freqDoppler = 0;
      if (doc.containsKey("tle")) {
        base64Tle = doc["tle"].as<const char*>();
        status.tle.freqComp = true;
      } else {
        base64Tle = doc["tlx"].as<const char*>();
        status.tle.freqComp = false;
      }

      size_t inputLen = strlen(base64Tle);
      size_t outputLen = 0;
      size_t maxTleSize = 64;
      size_t maxDecodedLength = (inputLen * 3 + 3) / 4;

      if (maxDecodedLength > maxTleSize) {
        return;
      }

      int ret = mbedtls_base64_decode(m.tle, maxTleSize, &outputLen, (const unsigned char*)base64Tle, inputLen);

      if (ret == 0) {
        radio.tle();
      }
    } else {
      m.tle[0] = 0;
      status.tle.freqDoppler = 0;
      status.tle.new_freqDoppler = 0;
      status.tle.freqComp = false;
    }

    radio.begin();
    radio.enableInterrupt();
    xSemaphoreGive(radioConfigMutex);
    result = 0;
  }

  if (!strcmp(command, commandBeginLora))
    result = radio.remote_begin_lora((char*)payload, length);

  if (!strcmp(command, commandBeginFSK))
    result = radio.remote_begin_fsk((char*)payload, length);

  if (!strcmp(command, commandFreq))
    result = radio.remote_freq((char*)payload, length);

  if (!strcmp(command, commandSat)) {
    remoteSatCmnd((char*)payload, length);
    result = 0;
  }

  if (!strcmp(command, commandSatFilter)) {
    remoteSatFilter((char*)payload, length);
    result = 0;
  }

  if (!strcmp(command, commandGoToSiesta)) {
    if (length < 1) return;
    remoteGoToSiesta((char*)payload, length);
    result = 0;
  }

  if (!strcmp(command, commandGoToSleep)) {
    if (length < 1) return;
    remoteGoToSleep((char*)payload, length);
    result = 0;
  }

  if (!strcmp(command, commandSetFreqOffset)) {
    if (length < 1) return;
    result = radio.remoteSetFreqOffset((char*)payload, length);
  }

  if (!strcmp(command, commandSetAdvParameters)) {
    char buff[length + 1];
    memcpy(buff, payload, length);
    buff[length] = '\0';
    ConfigStore::getInstance().setAdvancedConfig(buff);
    result = 0;
  }

  if (!strcmp(command, commandSetPosParameters)) {
    manageSetPosParameters((char*)payload, length);
    return;
  }

  if (!strcmp(command, commandSetName)) {
    manageSetName((char*)payload, length);
    return;
  }

  if (!strcmp(command, commandGetAdvParameters)) {
    sendAdvParameters();
    result = 0;
    return;
  }

  if (!global) {
    mqttPublish(buildTopic(statTopic, command).c_str(), (const char*)&result, 2, 0, false);
  }
}

void MQTT_Client::manageSetPosParameters(char* payload, size_t payload_len) {
  StaticJsonDocument<96> doc;
  deserializeJson(doc, payload, payload_len);
  if (doc.size() == 1) {
    status.tle.tgsALT = doc[0];
  }

  if (doc.size() == 3) {
    float receivedLat = doc[0];
    float receivedLon = doc[1];
    status.tle.tgsALT = doc[2];

    float currentLat = ConfigStore::getInstance().getLatitude();
    float currentLon = ConfigStore::getInstance().getLongitude();

    if (receivedLat != currentLat) {
      char buff[10];
      sprintf(buff, "%.3f", receivedLat);
      ConfigStore::getInstance().setLat(buff);
    }
    if (receivedLon != currentLon) {
      char buff[10];
      sprintf(buff, "%.3f", receivedLon);
      ConfigStore::getInstance().setLon(buff);
    }
  }
}

void MQTT_Client::manageSetName(char* payload, size_t payload_len) {
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, payload_len);
  if (error) return;

  if (doc.is<JsonArray>() && doc.size() == 2) {
    const char* received_mac_temp = doc[0].as<const char*>();
    const char* new_name_temp = doc[1].as<const char*>();

    if (received_mac_temp && new_name_temp) {
      uint64_t chipId = ESP.getEfuseMac();
      char clientId[13];
      sprintf(clientId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

      if (strcmp(received_mac_temp, clientId) == 0) {
        LOG_DEBUG(PSTR("Renaming to %s"), new_name_temp);
        ConfigStore::getInstance().setThingName(new_name_temp);
      }
    }
  }
}

void MQTT_Client::manageSatPosOled(char* payload, size_t payload_len) {
  StaticJsonDocument<64> doc;
  deserializeJson(doc, payload, payload_len);
  status.satPos[0] = doc[0];
  status.satPos[1] = doc[1];
}

void MQTT_Client::remoteSatCmnd(char* payload, size_t payload_len) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, payload_len);
  strcpy(status.modeminfo.satellite, doc[0]);
  uint32_t NORAD = doc[1];
  status.modeminfo.NORAD = NORAD;
  LOG_DEBUG(PSTR("Listening Satellite: %s NORAD: %u"), status.modeminfo.satellite, NORAD);
}

void MQTT_Client::remoteSatFilter(char* payload, size_t payload_len) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, payload_len);
  uint8_t filter_size = doc.size();

  for (uint8_t i = 0; i < 8 && i < filter_size; i++) {
    status.modeminfo.filter[i] = doc[i];
  }
  LOG_DEBUG(PSTR("Sat packets Filter enabled"));
}

void MQTT_Client::remoteGoToSleep(char* payload, size_t payload_len) {
  Radio& radio = Radio::getInstance();
  StaticJsonDocument<64> doc;
  deserializeJson(doc, payload, payload_len);

  uint32_t sleep_seconds = doc[0];
  LOG_DEBUG(PSTR("deep_sleep_enter"));
  esp_sleep_enable_timer_wakeup(1000000ULL * sleep_seconds);
  delay(100);
  Serial.flush();
  WiFi.disconnect(true);
  delay(100);
  radio.moduleSleep();
  esp_deep_sleep_start();
  delay(1000);
}

void MQTT_Client::remoteGoToSiesta(char* payload, size_t payload_len) {
  StaticJsonDocument<64> doc;
  deserializeJson(doc, payload, payload_len);

  uint32_t sleep_seconds = doc[0];
  LOG_DEBUG(PSTR("light_sleep_enter"));
  esp_sleep_enable_timer_wakeup(1000000ULL * sleep_seconds);
  delay(100);
  Serial.flush();
  WiFi.disconnect(true);
  delay(100);
  int ret = esp_light_sleep_start();
  WiFi.disconnect(false);
  LOG_DEBUG(PSTR("light_sleep: %d\n"), ret);
  delay(500);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:    LOG_DEBUG(PSTR("Wakeup: RTC_IO")); break;
    case ESP_SLEEP_WAKEUP_EXT1:    LOG_DEBUG(PSTR("Wakeup: RTC_CNTL")); break;
    case ESP_SLEEP_WAKEUP_TIMER:   LOG_DEBUG(PSTR("Wakeup: timer")); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: LOG_DEBUG(PSTR("Wakeup: touchpad")); break;
    case ESP_SLEEP_WAKEUP_ULP:     LOG_DEBUG(PSTR("Wakeup: ULP")); break;
    default: LOG_DEBUG(PSTR("Wakeup: unknown %d"), wakeup_reason); break;
  }
}

int MQTT_Client::voltage() {
  int voltages[21];
  for (int i = 0; i < 21; i++) {
    voltages[i] = analogRead(36);
  }
  // Bubble sort
  for (int i = 1; i < 21; i++) {
    for (int j = 0; j < 20; j++) {
      if (voltages[j + 1] < voltages[j]) {
        int temp = voltages[j];
        voltages[j] = voltages[j + 1];
        voltages[j + 1] = temp;
      }
    }
  }
  return voltages[10]; // median
}
