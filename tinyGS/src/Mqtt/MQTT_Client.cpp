/*
  MQTTClient.cpp - MQTT connection class
  
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
#include "ArduinoJson.h"
#if ARDUINOJSON_USE_LONG_LONG == 0 && !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /ArduinoJson/src/ArduinoJson/Configuration.hpp and amend to #define ARDUINOJSON_USE_LONG_LONG 1 around line 68"
#endif
#include "../Radio/Radio.h"
#include "../OTA/OTA.h"
#include "../Logger/Logger.h"

MQTT_Client::MQTT_Client()
    : PubSubClient(espClient)
{
#ifdef SECURE_MQTT
  espClient.setCACert(usingNewCert ? newRoot_CA : DSTroot_CA);
#endif
  randomTime = random(randomTimeMax - randomTimeMin) + randomTimeMin;
}

void MQTT_Client::loop()
{
  if (!connected())
  {
    status.mqtt_connected = false;
    if (millis() - lastConnectionAtempt > reconnectionInterval * connectionAtempts + randomTime)
    {
      Serial.println(randomTime);
      lastConnectionAtempt = millis();
      connectionAtempts++;

      lastPing = millis();
      Log::console(PSTR("Attempting MQTT connection..."));
      reconnect();
    }
  }
  else
  {
    connectionAtempts = 0;
    status.mqtt_connected = true;
  }

  if (connectionAtempts > connectionTimeout)
  {
    Log::console(PSTR("Unable to connect to MQTT Server after many atempts. Restarting..."));
    // if board is on LOW POWER mode instead of directly reboot it, force a 4hours deep sleep. 
    ConfigManager &configManager = ConfigManager::getInstance();
    if (configManager.getLowPower()) 
    {
      Radio &radio = Radio::getInstance();
      uint32_t sleep_seconds = 4*3600; // 4 hours deep sleep. 
      Log::debug(PSTR("deep_sleep_enter"));
      esp_sleep_enable_timer_wakeup( 1000000ULL * sleep_seconds); // using ULL  Unsigned Long long
      delay(100);
      Serial.flush();
      WiFi.disconnect(true);
      delay(100);
      //  TODO: apagar OLED
      radio.moduleSleep();
      esp_deep_sleep_start();
      delay(1000);   // shouldn't arrive here
    }
    else 
    {
      ESP.restart();
    }
  }

  PubSubClient::loop();

  unsigned long now = millis();
  if (now - lastPing > pingInterval && connected())
  {
    lastPing = now;
    if (scheduledRestart)
      sendWelcome();
    else
    {
      StaticJsonDocument<128> doc;
      doc["Vbat"] = voltage();
      doc["Mem"] = ESP.getFreeHeap();
      doc["RSSI"] =WiFi.RSSI();
      doc["radio"]= status.radio_error;
      doc["InstRSSI"]= status.modeminfo.currentRssi;

      char buffer[256];
      serializeJson(doc, buffer);
      Log::debug(PSTR("%s"), buffer);
      publish(buildTopic(teleTopic, topicPing).c_str(), buffer, false);
    }
  }
}

void MQTT_Client::reconnect()
{
  ConfigManager &configManager = ConfigManager::getInstance();
  uint64_t chipId = ESP.getEfuseMac();
  char clientId[13];
  sprintf(clientId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

  if (connect(clientId, configManager.getMqttUser(), configManager.getMqttPass(), buildTopic(teleTopic, topicStatus).c_str(), 2, false, "0"))
  {
    yield();
    Log::console(PSTR("Connected to MQTT!"));
    status.mqtt_connected = true;
    subscribeToAll();
    sendWelcome();
  }
  else
  {
    status.mqtt_connected = false;

    switch (state())
    {
      case MQTT_CONNECTION_TIMEOUT:
        if (connectionAtempts > 4)
          Log::console(PSTR("MQTT conection timeout, check your wifi signal strength retrying..."), state());
        break;
      case MQTT_CONNECT_FAILED:
        if (connectionAtempts > 3)
        {
#ifdef SECURE_MQTT
          if (usingNewCert)
            espClient.setCACert(DSTroot_CA);
          else
            espClient.setCACert(newRoot_CA);
          usingNewCert = !usingNewCert;
#endif
        }
        break;
      case MQTT_CONNECT_BAD_CREDENTIALS:
      case MQTT_CONNECT_UNAUTHORIZED:
        Log::console(PSTR("MQTT authentication failure. You can check the MQTT credentials connecting to the config panel on the ip: %s."), WiFi.localIP().toString().c_str());
        break;
      default:
        Log::console(PSTR("failed, rc=%i"), state());
    }
  }
}

String MQTT_Client::buildTopic(const char *baseTopic, const char *cmnd)
{
  ConfigManager &configManager = ConfigManager::getInstance();
  String topic = baseTopic;
  topic.replace("%user%", configManager.getMqttUser());
  topic.replace("%station%", configManager.getThingName());
  topic.replace("%cmnd%", cmnd);

  return topic;
}

void MQTT_Client::subscribeToAll()
{
  subscribe(buildTopic(globalTopic, "#").c_str());
  subscribe(buildTopic(cmndTopic, "#").c_str());
}

void MQTT_Client::sendWelcome()
{
  scheduledRestart = false;
  ConfigManager &configManager = ConfigManager::getInstance();
  time_t now;
  time(&now);

  uint64_t chipId = ESP.getEfuseMac();
  char clientId[13];
  sprintf(clientId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(17) + 22 + 20 + 20 + 20 + 40+ 20;
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(configManager.getLatitude());
  station_location.add(configManager.getLongitude());
  doc["tx"] = configManager.getAllowTx();
  doc["time"] = now;
  doc["version"] = status.version;
  doc["git_version"] = status.git_version;
  doc["sat"] = status.modeminfo.satellite;
  doc["ip"] = WiFi.localIP().toString();
  if (configManager.getLowPower())
    doc["lp"].set(configManager.getLowPower());
  doc["modem_conf"].set(configManager.getModemStartup());
  doc["boardTemplate"].set(configManager.getBoardTemplate());
  doc["Mem"] = ESP.getFreeHeap();
  doc["Size"] = ESP.getSketchSize();
  doc["MD5"] = ESP.getSketchMD5();
  doc["board"] = configManager.getBoard();
  doc["mac"] = clientId;
  doc["seconds"] = millis()/1000;
  doc["Vbat"] = voltage();
  doc["chip"] = ESP.getChipModel();

  char buffer[1048];
  serializeJson(doc, buffer);
  publish(buildTopic(teleTopic, topicWelcome).c_str(), buffer, false);
}

void MQTT_Client::sendRx(String packet, bool noisy)
{
  ConfigManager &configManager = ConfigManager::getInstance();
  time_t now;
  time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(23) + 25;
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(configManager.getLatitude());
  station_location.add(configManager.getLongitude());
  doc["mode"] = status.modeminfo.modem_mode;
  doc["frequency"] = status.modeminfo.frequency;
  doc["frequency_offset"] = status.modeminfo.freqOffset;
  doc["satellite"] = status.modeminfo.satellite;

  if (String(status.modeminfo.modem_mode) == "LoRa")
  {
    doc["sf"] = status.modeminfo.sf;
    doc["cr"] = status.modeminfo.cr;
    doc["bw"] = status.modeminfo.bw;
  }
  else
  {
    doc["bitrate"] = status.modeminfo.bitrate;
    doc["freqdev"] = status.modeminfo.freqDev;
    doc["rxBw"] = status.modeminfo.bw;
  }

  doc["rssi"] = status.lastPacketInfo.rssi;
  doc["snr"] = status.lastPacketInfo.snr;
  doc["frequency_error"] = status.lastPacketInfo.frequencyerror;
  doc["unix_GS_time"] = now;
  doc["usec_time"] = (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
  doc["time_offset"] = status.time_offset;
  doc["crc_error"] = status.lastPacketInfo.crc_error;
  doc["data"] = packet.c_str();
  doc["NORAD"] = status.modeminfo.NORAD;
  doc["noisy"] = noisy;

  char buffer[1536];
  serializeJson(doc, buffer);
  Log::debug(PSTR("%s"), buffer);
  publish(buildTopic(teleTopic, topicRx).c_str(), buffer, false);
}

void MQTT_Client::sendStatus()
{
  ConfigManager &configManager = ConfigManager::getInstance();
  time_t now;
  time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(29) + 25;
  DynamicJsonDocument doc(capacity);
  JsonArray station_location = doc.createNestedArray("station_location");
  station_location.add(configManager.getLatitude());
  station_location.add(configManager.getLongitude());

  doc["version"] = status.version;
  doc["board"] = configManager.getBoard();
  doc["tx"] = configManager.getAllowTx();

  doc["mode"] = status.modeminfo.modem_mode;
  doc["frequency"] = status.modeminfo.frequency;
  doc["frequency_offset"] = status.modeminfo.freqOffset;
  doc["satellite"] = status.modeminfo.satellite;
  doc["NORAD"] = status.modeminfo.NORAD;

  if (String(status.modeminfo.modem_mode) == "LoRa")
  {
    doc["sf"] = status.modeminfo.sf;
    doc["cr"] = status.modeminfo.cr;
    doc["bw"] = status.modeminfo.bw;
  }
  else
  {
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
  doc["time_offset"] = status.time_offset;

  char buffer[1024];
  serializeJson(doc, buffer);
  publish(buildTopic(statTopic, topicStatus).c_str(), buffer, false);
}

void MQTT_Client::sendAdvParameters()
{
  ConfigManager &configManager = ConfigManager::getInstance();
  StaticJsonDocument<512> doc;
  doc["adv_prm"].set(configManager.getAvancedConfig());
  char buffer[512];
  serializeJson(doc, buffer);
  Log::debug(PSTR("%s"), buffer);
  publish(buildTopic(teleTopic, topicGet_adv_prm).c_str(), buffer, false);
}

// helper funcion (this has to dissapear)
bool isValidFrequency(uint8_t radio, float f)
{
  return !((radio == 1 && (f < 137 || f > 525)) ||
        (radio == 2 && (f < 137 || f > 1020)) ||
        (radio == 5 && (f < 410 || f > 810)) ||
        (radio == 6 && (f < 150 || f > 960)) ||
        (radio == 8 && (f < 2400|| f > 2500)));
}

void MQTT_Client::manageMQTTData(char *topic, uint8_t *payload, unsigned int length)
{
  Radio &radio = Radio::getInstance();

  bool global = true;
  char *command;
  strtok(topic, "/");                      // tinygs
  if (strcmp(strtok(NULL, "/"), "global")) // user
  {
    global = false;
    strtok(NULL, "/"); // station
  }
  strtok(NULL, "/"); // cmnd
  command = strtok(NULL, "/");
  uint16_t result = 0xFF;

  if (!strcmp(command, commandSatPos))
  {
    manageSatPosOled((char *)payload, length);
    return; // no ack
  }

  if (!strcmp(command, commandReset))
    ESP.restart();

  if (!strcmp(command, commandUpdate))
  {
    OTA::update();
    return; // no ack
  }

  if (!strcmp(command, commandFrame))
  {
    uint8_t frameNumber = atoi(strtok(NULL, "/"));
    DynamicJsonDocument doc(JSON_ARRAY_SIZE(5) * 15 + JSON_ARRAY_SIZE(15));
    deserializeJson(doc, payload, length);
    status.remoteTextFrameLength[frameNumber] = doc.size();
    Log::debug(PSTR("Received frame: %u"), status.remoteTextFrameLength[frameNumber]);

    for (uint8_t n = 0; n < status.remoteTextFrameLength[frameNumber]; n++)
    {
      status.remoteTextFrame[frameNumber][n].text_font = doc[n][0];
      status.remoteTextFrame[frameNumber][n].text_alignment = doc[n][1];
      status.remoteTextFrame[frameNumber][n].text_pos_x = doc[n][2];
      status.remoteTextFrame[frameNumber][n].text_pos_y = doc[n][3];
      String text = doc[n][4];
      status.remoteTextFrame[frameNumber][n].text = text;

      Log::debug(PSTR("Text: %u Font: %u Alig: %u Pos x: %u Pos y: %u -> %s"), n,
                 status.remoteTextFrame[frameNumber][n].text_font,
                 status.remoteTextFrame[frameNumber][n].text_alignment,
                 status.remoteTextFrame[frameNumber][n].text_pos_x,
                 status.remoteTextFrame[frameNumber][n].text_pos_y,
                 status.remoteTextFrame[frameNumber][n].text.c_str());
    }

    result = 0;
  }

  if (!strcmp(command, commandStatus))
  {
    uint8_t mode = payload[0] - '0';
    Log::debug(PSTR("Remote status requested: %u"), mode); // right now just one mode
    sendStatus();
    return;
  }

  if (!strcmp(command, commandLog))
  {
    char logStr[length + 1];
    memcpy(logStr, payload, length);
    logStr[length] = '\0';
    Log::console(PSTR("%s"), logStr);
    return; // do not send ack for this one
  }

  if (!strcmp(command, commandTx))
  {
    result = radio.sendTx(payload, length);
    Log::console(PSTR("Sending TX packet!"));
  }

  // ######################################################
  // ############## Remote tune commands ##################
  // ######################################################
  if (global)
    return;

  if (!strcmp(command, commandBeginp))
  {
    char buff[length + 1];
    memcpy(buff, payload, length);
    buff[length] = '\0';
    Log::debug(PSTR("%s"), buff);

    size_t size = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(16) + JSON_ARRAY_SIZE(8) + JSON_ARRAY_SIZE(8) + 64;
    DynamicJsonDocument doc(size);
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error.code() != DeserializationError::Ok || !doc.containsKey("mode"))
    {
      Log::console(PSTR("ERROR: Your modem config is invalid. Resetting to default"));
      return;
    }

    // check frequecy is valid prior to load 
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board))
      return; 
    
    if (!isValidFrequency(board.L_radio, doc["freq"]))
    {
      Log::console(PSTR("ERROR: Wrong frequency. Ignoring."));
      return;
    }

    ConfigManager::getInstance().setModemStartup(buff);
  }

  if (!strcmp(command, commandBegine))
  {
    size_t size = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(16) + JSON_ARRAY_SIZE(8) + JSON_ARRAY_SIZE(8) + 64;
    DynamicJsonDocument doc(size);
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error.code() != DeserializationError::Ok || !doc.containsKey("mode"))
    {
      Log::console(PSTR("ERROR: The received modem configuration is invalid. Ignoring."));
      return;
    }

    // check frequecy is valid prior to load  
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board))
      return; 
    
    if (!isValidFrequency(board.L_radio, doc["freq"]))
    {
      Log::console(PSTR("ERROR: Wrong frequency. Ignoring."));
      return;
    }
   
    ModemInfo &m = status.modeminfo;
    m.modem_mode = doc["mode"].as<String>();
    strcpy(m.satellite, doc["sat"].as<char *>());
    m.NORAD = doc["NORAD"];


    if (m.modem_mode == "LoRa")
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
      m.enc= doc["enc"];
      /////////////////////////////////////
      m.whitening_seed= doc["ws"];
      m.framing= doc["fr"];
      m.crc_by_sw= doc["cSw"];
      m.crc_nbytes= doc["cB"];
      m.crc_init= doc["cI"];
      m.crc_poly= doc["cP"];
      m.crc_finalxor= doc["cF"];
      m.crc_refIn= doc["cRI"];
      m.crc_refOut= doc["cRO"];
      /*
      RADIO_SX1278 = 1,
      RADIO_SX1276 = 2,
      RADIO_SX1268 = 5,
      RADIO_SX1262 = 6,
      RADIO_SX1280 = 8
      */
      Log::console(PSTR("Board: %i Required Bandwidth (BW): %f"),board.L_radio,m.bw);
      switch (board.L_radio)
      {
      case RADIO_SX1278:{
        /* code */
        // Receiver bandwidth in kHz (FSK/OOK) Allowed values are:
        // 2.6, 3.1, 3.9, 5.2, 6.3, 7.8, 10.4, 12.5, 15.6, 20.8, 25, 31.3, 41.7, 50, 62.5, 83.3, 100, 125, 166.7, 200, 250 kHz
        float abw[21]={2.6, 3.1, 3.9, 5.2, 6.3, 7.8, 10.4, 12.5, 15.6, 20.8, 25, 31.3, 41.7, 50, 62.5, 83.3, 100, 125, 166.7, 200, 250};
        float arxBw = m.bw;
        int i=0;
        while (m.bw>abw[i] && i<21){i++;}
        if (i<21){arxBw=abw[i];}else{arxBw=abw[i-1];}
        if (arxBw!=m.bw){
          Log::console(PSTR("The initial BW of %.2f has been matched to board BW of %.2f"),m.bw,arxBw);
          m.bw=arxBw;
        }
      }
      break;
        
      case RADIO_SX1268:{
        /* code */
        // Receiver bandwidth in kHz (FSK/OOK) Allowed values are:
        // 4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5, 23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6, 467.0
        float abw[21]={4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5, 23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6, 467.0};
        float arxBw = m.bw; 
        int i=0;
        while (m.bw>abw[i] && i<21){i++;}
        if (i<21){arxBw=abw[i];}else{arxBw=abw[i-1];}
        if (arxBw!=m.bw){
          Log::console(PSTR("The initial BW of %.2f has been matched to board BW of %.2f"),m.bw,arxBw);
          m.bw=arxBw;
        }
      }
      break;

      default:
        break;
      }
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

    radio.begin();
//    radio.currentRssi();
    result = 0;
  }

  // Remote_Begin_Lora [437.7,125.0,11,8,18,11,120,8,0]
  if (!strcmp(command, commandBeginLora))
    result = radio.remote_begin_lora((char *)payload, length);

  // Remote_Begin_FSK [433.5,100.0,10.0,250.0,10,100,16,0,0]
  if (!strcmp(command, commandBeginFSK))
    result = radio.remote_begin_fsk((char *)payload, length);

  if (!strcmp(command, commandFreq))
    result = radio.remote_freq((char *)payload, length);

  if (!strcmp(command, commandBw))
    result = radio.remote_bw((char *)payload, length);

  if (!strcmp(command, commandSf))
    result = radio.remote_sf((char *)payload, length);

  if (!strcmp(command, commandCr))
    result = radio.remote_cr((char *)payload, length);

  if (!strcmp(command, commandCrc))
    result = radio.remote_crc((char *)payload, length);

  // Remote_LoRa_syncword [8,1,2,3,4,5,6,7,8,9]
  if (!strcmp(command, commandLsw))
    result = radio.remote_lsw((char *)payload, length);

  if (!strcmp(command, commandFldro))
    result = radio.remote_fldro((char *)payload, length);

  if (!strcmp(command, commandAldro))
    result = radio.remote_aldro((char *)payload, length);

  if (!strcmp(command, commandPl))
    result = radio.remote_pl((char *)payload, length);

  if (!strcmp(command, commandBr))
    result = radio.remote_br((char *)payload, length);

  if (!strcmp(command, commandFd))
    result = radio.remote_fd((char *)payload, length);

  if (!strcmp(command, commandFbw))
    result = radio.remote_fbw((char *)payload, length);

  // Remote_FSK_syncword [8,1,2,3,4,5,6,7,8,9]
  if (!strcmp(command, commandFsw))
    result = radio.remote_fsw((char *)payload, length);

  // Remote_FSK_Set_OOK + DataShapingOOK(only sx1278) [1,2]
  if (!strcmp(command, commandFook))
    result = radio.remote_fook((char *)payload, length);

  // Remote_Satellite_Name [\"FossaSat-3\" , 46494 ]
  if (!strcmp(command, commandSat))
  {
    remoteSatCmnd((char *)payload, length);
    result = 0;
  }

  // Satellite_Filter [1,0,51]   (lenght,position,byte1,byte2,byte3,byte4)
  if (!strcmp(command, commandSatFilter))
  {
    remoteSatFilter((char *)payload, length);
    result = 0;
  }

  // Send station to lightsleep (siesta) x seconds
  if (!strcmp(command, commandGoToSiesta))
  {
    if (length < 1)
      return;
    remoteGoToSiesta((char *)payload, length);
    result = 0;
  }

 // Send station to deepsleep x seconds
  if (!strcmp(command, commandGoToSleep))
  {
    if (length < 1)
      return;
    remoteGoToSleep((char *)payload, length);
    result = 0;
  }

  // Set frequency offset
  if (!strcmp(command, commandSetFreqOffset))
  {
    if (length < 1)
      return;
    result = radio.remoteSetFreqOffset((char *)payload, length);
  
  }

  if (!strcmp(command, commandSetAdvParameters))
  {
    char buff[length + 1];
    memcpy(buff, payload, length);
    buff[length] = '\0';
    Log::debug(PSTR("%s"), buff);
    ConfigManager::getInstance().setAvancedConfig(buff);
  }

  if (!strcmp(command, commandGetAdvParameters))
  {
    sendAdvParameters();
    return;
  }

  // GOD MODE  With great power comes great responsibility!
  // SPIsetRegValue  (only sx1278) [1,2,3,4,5]
  if (!strcmp(command, commandSPIsetRegValue))
    result = radio.remote_SPIsetRegValue((char *)payload, length);

  // SPIwriteRegister  (only sx1278) [1,2]
  if (!strcmp(command, commandSPIwriteRegister))
  {
    radio.remote_SPIwriteRegister((char *)payload, length);
    result = 0;
  }

  if (!strcmp(command, commandSPIreadRegister) && !global)
    result = radio.remote_SPIreadRegister((char *)payload, length);

  if (!strcmp(command, commandBatchConf))
  {
    Log::debug(PSTR("BatchConfig"));
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload, length);
    JsonObject root = doc.as<JsonObject>();
    result = 0;

    for (JsonPair kv : root)
    {
      const char *key = kv.key().c_str();
      char *value = (char *)kv.value().as<char *>();
      size_t len = strlen(value);
      Log::debug(PSTR("%s %s %u"), key, value, len);

      if (!strcmp(key, commandCrc))
        result = radio.remote_crc(value, len);

      else if (!strcmp(key, commandFreq))
        result = radio.remote_freq(value, len);

      else if (!strcmp(key, commandBeginLora))
        result = radio.remote_begin_lora(value, len);

      else if (!strcmp(key, commandBw))
        result = radio.remote_bw(value, len);

      else if (!strcmp(key, commandSf))
        result = radio.remote_sf(value, len);

      else if (!strcmp(key, commandLsw))
        result = radio.remote_lsw(value, len);

      else if (!strcmp(key, commandFldro))
        result = radio.remote_fldro(value, len);

      else if (!strcmp(key, commandAldro))
        result = radio.remote_aldro(value, len);

      else if (!strcmp(key, commandPl))
        result = radio.remote_pl(value, len);

      else if (!strcmp(key, commandBeginFSK))
        result = radio.remote_begin_fsk(value, len);

      else if (!strcmp(key, commandBr))
        result = radio.remote_br(value, len);

      else if (!strcmp(key, commandFd))
        result = radio.remote_fd(value, len);

      else if (!strcmp(key, commandFbw))
        result = radio.remote_fbw(value, len);

      else if (!strcmp(key, commandFsw))
        result = radio.remote_fsw(value, len);

      else if (!strcmp(key, commandFook))
        result = radio.remote_fook(value, len);

      else if (!strcmp(key, commandSat))
        remoteSatCmnd(value, len);

      else if (!strcmp(key, commandSatFilter))
        remoteSatFilter(value, len);

      else if (!strcmp(key, commandSPIsetRegValue))
        result = radio.remote_SPIsetRegValue(value, len);

      else if (!strcmp(key, commandSPIwriteRegister))
        radio.remote_SPIwriteRegister(value, len);

      if (result) // there was an error
      {
        Log::debug(PSTR("Error ocurred during batch config!!"));
        break;
      }
    }
  }

  if (!global)
    publish(buildTopic(statTopic, command).c_str(), (uint8_t *)&result, 2U, false);
}

void MQTT_Client::manageSatPosOled(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(60);
  deserializeJson(doc, payload, payload_len);
  status.satPos[0] = doc[0];
  status.satPos[1] = doc[1];
}

void MQTT_Client::remoteSatCmnd(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, payload_len);
  strcpy(status.modeminfo.satellite, doc[0]);
  uint32_t NORAD = doc[1];
  status.modeminfo.NORAD = NORAD;

  Log::debug(PSTR("Listening Satellite: %s NORAD: %u"), status.modeminfo.satellite, NORAD);
}

void MQTT_Client::remoteSatFilter(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, payload_len);
  uint8_t filter_size = doc.size();

  status.modeminfo.filter[0] = doc[0];
  status.modeminfo.filter[1] = doc[1];

  Log::debug(PSTR("Set Sat Filter Size %d"), status.modeminfo.filter[0]);
  Log::debug(PSTR("Set Sat Filter POS  %d"), status.modeminfo.filter[1]);
  Log::debug(PSTR("-> "));
  for (uint8_t filter_pos = 2; filter_pos < filter_size; filter_pos++)
  {
    status.modeminfo.filter[filter_pos] = doc[filter_pos];
    Log::debug(PSTR(" 0x%x  ,"), status.modeminfo.filter[filter_pos]);
  }
  Log::debug(PSTR("Sat packets Filter enabled"));
}

void MQTT_Client::remoteGoToSleep(char *payload, size_t payload_len)
{
  Radio &radio = Radio::getInstance();
  DynamicJsonDocument doc(60);
  deserializeJson(doc, payload, payload_len);

  uint32_t sleep_seconds = doc[0];                        // max 
  //uint8_t  int_pin = doc [1];   // 99 no int pin

  Log::debug(PSTR("deep_sleep_enter"));
  esp_sleep_enable_timer_wakeup( 1000000ULL * sleep_seconds); // using ULL  Unsigned Long long
  //esp_sleep_enable_ext0_wakeup(int_pin,0);
  delay(100);
  Serial.flush();
  WiFi.disconnect(true);
  delay(100);
  //  TODO: apagar OLED
  radio.moduleSleep();
  esp_deep_sleep_start();
  delay(1000);   // shouldn't arrive here

}


void MQTT_Client::remoteGoToSiesta(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(60);
  deserializeJson(doc, payload, payload_len);

  uint32_t sleep_seconds = doc[0];                        // max 
  //uint8_t  int_pin = doc [1];   // 99 no int pin

  Log::debug(PSTR("light_sleep_enter"));
  esp_sleep_enable_timer_wakeup( 1000000ULL * sleep_seconds); // using ULL  Unsigned Long long
  //esp_sleep_enable_ext0_wakeup(int_pin,0);
  delay(100);
  Serial.flush();
  WiFi.disconnect(true);
  delay(100);
  int ret = esp_light_sleep_start();
  WiFi.disconnect(false);
  Log::debug(PSTR("light_sleep: %d\n"), ret);
  // for stations with sleep disable OLED
  //displayTurnOff();
  delay(500);
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Log::debug(PSTR("Wakeup caused by external signal using RTC_IO"));
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Log::debug(PSTR("Wakeup caused by external signal using RTC_CNTL"));
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Log::debug(PSTR("Wakeup caused by timer"));
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Log::debug(PSTR("Wakeup caused by touchpad"));
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Log::debug(PSTR("Wakeup caused by ULP program"));
    break;
  default:
    Log::debug(PSTR("Wakeup was not caused by deep sleep: %d\n"), wakeup_reason);
    break;
  }
}





// Helper class to use as a callback
void manageMQTTDataCallback(char *topic, uint8_t *payload, unsigned int length)
{
  Log::debug(PSTR("Received MQTT message: %s : %.*s"), topic, length, payload);
  MQTT_Client::getInstance().manageMQTTData(topic, payload, length);
}

void MQTT_Client::begin()
{
  ConfigManager &configManager = ConfigManager::getInstance();
  setServer(configManager.getMqttServer(), configManager.getMqttPort());
  setCallback(manageMQTTDataCallback);
}



int MQTT_Client::voltage() {
  int medianVoltage;
  int length = 21;
  int voltages[22];
  
  for (int i = 0; i < 22; i++)
  {
    voltages[i] = analogRead(36); 
    }
  
  //    BubbleSortAsc   from https://www.luisllamas.es/arduino-bubble-sort/
   int i, j, flag = 1;
   int temp;
   for (i = 1; (i <= length) && flag; i++)
   {
      flag = 0;
      for (j = 0; j < (length - 1); j++)
      {
         if (voltages[j + 1] < voltages[j])
         {
            temp = voltages[j];
            voltages[j] = voltages[j + 1];
            voltages[j + 1] = temp;
            flag = 1;
         }
      }
   }
  medianVoltage = voltages[10];
  return medianVoltage;
}

