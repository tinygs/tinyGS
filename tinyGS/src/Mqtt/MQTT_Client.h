/*
  MQTT_Client.h - MQTT client using native ESP-IDF esp-mqtt

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

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "../Network/ConfigStore.h"
#include "../Network/INetworkAware.h"
#include "../Status.h"
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// RX packet queue message (optimized for async sending)
struct RxPacketMessage {
  char packet[360];
  char raw_packet[360];
  bool noisy;
  char modem_mode[8];
  char satellite[25];
  float frequency;
  float freqOffset;
  uint32_t NORAD;
  uint8_t sf;
  uint8_t cr;
  float bw;
  bool iIQ;
  float bitrate;
  float freqDev;
  float rssi;
  float snr;
  float frequencyerror;
  float freqDoppler;
  bool crc_error;
  // Timestamps captured at reception time
  time_t unix_time;
  int64_t usec_time;
};

extern Status status;

class MQTT_Client : public INetworkAware {
public:
  static MQTT_Client& getInstance() {
    static MQTT_Client instance;
    return instance;
  }

  void begin();
  void loop();
  void sendWelcome();
  void sendRx(String packet, bool noisy, String raw_packet);
  void queueRx(const String& packet, bool noisy, const String& raw_packet);
  void manageMQTTData(char* topic, uint8_t* payload, unsigned int length);
  void sendStatus();
  void sendAdvParameters();
  void sendWeblogin();
  void scheduleRestart() { scheduledRestart = true; }

  bool isConnected() const { return _mqttConnected; }

  // INetworkAware
  void onConnected(IPAddress ip, ActiveInterface iface) override;
  void onDisconnected() override;
  void onInterfaceChanged(ActiveInterface iface, IPAddress ip) override;

private:
  MQTT_Client();

  // esp-mqtt handle
  esp_mqtt_client_handle_t _client = nullptr;
  bool _mqttConnected = false;
  bool _networkAvailable = false;

  String buildTopic(const char* baseTopic, const char* cmnd);
  void subscribeToAll();
  void manageSatPosOled(char* payload, size_t payload_len);
  void manageSetPosParameters(char* payload, size_t payload_len);
  void manageSetName(char* payload, size_t payload_len);
  void remoteSatCmnd(char* payload, size_t payload_len);
  void remoteSatFilter(char* payload, size_t payload_len);
  void remoteGoToSleep(char* payload, size_t payload_len);
  void remoteGoToSiesta(char* payload, size_t payload_len);
  void processRxQueue();
  void sendRxFromQueue(const RxPacketMessage& msg);
  int  voltage();
  void mqttPublish(const char* topic, const char* data, int len = 0, int qos = 0, bool retain = false);

  // Static event handler for esp-mqtt C callback
  static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
  void handleMqttEvent(esp_mqtt_event_handle_t event);

  // Incoming message assembly buffer (esp-mqtt may fragment large messages)
  String _inTopic;
  String _inPayload;

  SemaphoreHandle_t radioConfigMutex;
  QueueHandle_t rxQueue;
  static const int RX_QUEUE_SIZE = 10;

  unsigned long lastPing = 0;
  unsigned long lastConnectionAttempt = 0;
  uint8_t connectionAttempts = 0;
  bool scheduledRestart = false;

  const unsigned long pingInterval = 1 * 60 * 1000;
  const unsigned long reconnectionInterval = 20 * 1000;
  const unsigned long randomTimeMin = 10 * 1000;
  const unsigned long randomTimeMax = 20 * 1000;
  unsigned long randomTime = 0;
  unsigned long _pendingConnectAt = 0;  // millis() timestamp after which begin() should be called
  const uint16_t connectionTimeout = 6;

  const char* globalTopic PROGMEM = "tinygs/global/%cmnd%";
  const char* cmndTopic   PROGMEM = "tinygs/%user%/%station%/cmnd/%cmnd%";
  const char* teleTopic   PROGMEM = "tinygs/%user%/%station%/tele/%cmnd%";
  const char* statTopic   PROGMEM = "tinygs/%user%/%station%/stat/%cmnd%";

  // tele
  const char* topicWelcome       PROGMEM = "welcome";
  const char* topicPing          PROGMEM = "ping";
  const char* topicStatus        PROGMEM = "status";
  const char* topicRx            PROGMEM = "rx";
  const char* topicGet_adv_prm   PROGMEM = "get_adv_prm";

  // command
  const char* commandBatchConf       PROGMEM = "batch_conf";
  const char* commandUpdate          PROGMEM = "update";
  const char* commandSatPos          PROGMEM = "sat_pos_oled";
  const char* commandReset           PROGMEM = "reset";
  const char* commandFreq            PROGMEM = "freq";
  const char* commandBegine          PROGMEM = "begine";
  const char* commandBeginp          PROGMEM = "beginp";
  const char* commandBeginLora       PROGMEM = "begin_lora";
  const char* commandBeginFSK        PROGMEM = "begin_fsk";
  const char* commandFrame           PROGMEM = "frame";
  const char* commandSat             PROGMEM = "sat";
  const char* commandStatus          PROGMEM = "status";
  const char* commandLog             PROGMEM = "log";
  const char* commandTx              PROGMEM = "tx";
  const char* commandSatFilter       PROGMEM = "filter";
  const char* commandGoToSleep       PROGMEM = "sleep";
  const char* commandGoToSiesta      PROGMEM = "siesta";
  const char* commandSetFreqOffset   PROGMEM = "foff";
  const char* commandSetAdvParameters  PROGMEM = "set_adv_prm";
  const char* commandGetAdvParameters  PROGMEM = "get_adv_prm";
  const char* commandWeblogin        PROGMEM = "weblogin";
  const char* commandSetPosParameters  PROGMEM = "set_pos_prm";
  const char* commandSetName         PROGMEM = "set_name";
};

#endif
