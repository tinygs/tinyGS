/*
  MQTTClient.h - MQTT connection class
  
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

#define SECURE_MQTT // Comment this line if you are not using MQTT over SSL

#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Estructura optimizada para cola de paquetes RX no bloqueante
// Solo guarda los campos necesarios para reconstruir el mensaje MQTT
// Base64: ceil(256/3)*4 = 344 chars + null = 345 bytes, redondeamos a 360
struct RxPacketMessage {
  char packet[360];           // Base64 encoded packet
  char raw_packet[360];       // Base64 encoded raw packet  
  bool noisy;
  // Info del modem (solo campos necesarios para el JSON)
  char modem_mode[8];
  char satellite[25];
  float frequency;
  float freqOffset;
  uint32_t NORAD;
  // LoRa específicos
  uint8_t sf;
  uint8_t cr;
  float bw;
  bool iIQ;
  // FSK específicos
  float bitrate;
  float freqDev;
  // Info del paquete
  float rssi;
  float snr;
  float frequencyerror;
  float freqDoppler;
  bool crc_error;
  // Timestamps captured at reception time
  time_t unix_time;
  int64_t usec_time;
};
// Tamaño total: ~820 bytes x 10 = ~8.2KB (vs ~1KB x 10 con estructuras completas)

#if MQTT_MAX_PACKET_SIZE != 1000  && !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /PubSubClient/src/PubSubClient.h  and set #define MQTT_MAX_PACKET_SIZE 1000"
#endif
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#include "../certs.h"
#else
#include <WiFiClient.h>
#endif

extern Status status;

class MQTT_Client : public PubSubClient {
public:
  static MQTT_Client& getInstance()
  {
    static MQTT_Client instance; 
    return instance;
  }
  void begin();
  void loop();
  void sendWelcome();
  void sendRx(String packet, bool noisy, String raw_packet);  // Solo usado como fallback
  void queueRx(const String& packet, bool noisy, const String& raw_packet);
  void manageMQTTData(char *topic, uint8_t *payload, unsigned int length);
  void sendStatus ();
  void sendAdvParameters ();
  void sendWeblogin ();
  void scheduleRestart () { scheduledRestart = true; };

protected:
#ifdef SECURE_MQTT
  WiFiClientSecure espClient;
#else
  WiFiClient espClient;
#endif
  void reconnect();

private:
  const char* LOG_TAG = "MQTT";
  MQTT_Client ();
  String buildTopic(const char * baseTopic, const char * cmnd);
  void subscribeToAll();
  void manageSatPosOled(char* payload, size_t payload_len);
  void manageSetPosParameters(char* payload, size_t payload_len);
  void manageSetName(char* payload, size_t payload_len);
  void remoteSatCmnd(char* payload, size_t payload_len);
  void remoteSatFilter(char* payload, size_t payload_len);
  void remoteGoToSleep(char* payload, size_t payload_len);
  void remoteGoToSiesta(char* payload, size_t payload_len);
  void processRxQueue();
  void sendRxFromQueue(const RxPacketMessage& msg);  // Envía paquete desde la cola

  int  voltage();
  
  //bool usingNewCert = true;
  SemaphoreHandle_t radioConfigMutex;
  QueueHandle_t rxQueue;
  static const int RX_QUEUE_SIZE = 10;  // Cola de hasta 10 paquetes
  unsigned long lastPing = 0;
  unsigned long lastConnectionAtempt = 0;
  uint8_t connectionAtempts = 0;
  bool scheduledRestart = false;

  const unsigned long pingInterval = 1 * 60 * 1000;
  const unsigned long reconnectionInterval = 20 * 1000;
  const unsigned long randomTimeMin = 10 * 1000;
  const unsigned long randomTimeMax = 20 * 1000;
  unsigned long randomTime = 0;
  const uint16_t connectionTimeout = 6;

  const char* globalTopic PROGMEM = "tinygs/global/%cmnd%";
  const char* cmndTopic PROGMEM = "tinygs/%user%/%station%/cmnd/%cmnd%";
  const char* teleTopic PROGMEM = "tinygs/%user%/%station%/tele/%cmnd%";
  const char* statTopic PROGMEM = "tinygs/%user%/%station%/stat/%cmnd%";

  // tele
  const char* topicWelcome PROGMEM = "welcome";
  const char* topicPing PROGMEM= "ping";
  const char* topicStatus PROGMEM = "status";
  const char* topicRx PROGMEM= "rx";
  const char* topicGet_adv_prm PROGMEM = "get_adv_prm";

  // command
  const char* commandBatchConf PROGMEM= "batch_conf";
  const char* commandUpdate PROGMEM= "update";
  const char* commandSatPos PROGMEM= "sat_pos_oled";
  const char* commandReset PROGMEM= "reset";
  const char* commandFreq PROGMEM= "freq";
   const char* commandBegine PROGMEM= "begine";
  const char* commandBeginp PROGMEM= "beginp";
  const char* commandBeginLora PROGMEM= "begin_lora";
  const char* commandBeginFSK PROGMEM= "begin_fsk";
  const char* commandFrame PROGMEM= "frame";
  const char* commandSat PROGMEM= "sat";
  const char* commandStatus PROGMEM= "status";
  const char* commandLog PROGMEM= "log";
  const char* commandTx PROGMEM= "tx";
  const char* commandSatFilter PROGMEM= "filter";
  const char* commandGoToSleep PROGMEM= "sleep";
  const char* commandGoToSiesta PROGMEM= "siesta";
  const char* commandSetFreqOffset PROGMEM= "foff";
  const char* commandSetAdvParameters PROGMEM= "set_adv_prm";
  const char* commandGetAdvParameters PROGMEM = "get_adv_prm";
  const char* commandWeblogin PROGMEM = "weblogin";
  const char* commandSetPosParameters PROGMEM= "set_pos_prm"; 
  const char* commandSetName PROGMEM= "set_name"; 

};

#endif