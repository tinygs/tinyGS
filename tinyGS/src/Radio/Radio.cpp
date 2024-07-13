/*
  Radio.cpp - Class to handle radio communications
  
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

#include "Radio.h"
#include "ArduinoJson.h"
#if ARDUINOJSON_USE_LONG_LONG == 0 && !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or /ArduinoJson/src/ArduinoJson/Configuration.hpp and amend to #define ARDUINOJSON_USE_LONG_LONG 1 around line 68"
#endif
#include <base64.h>
#include "../Logger/Logger.h"

//@estbhan
//04/08/2023
#include "../BitCode/BitCode.h"
#include "../Satellites/Satellites.h"

#define CHECK_ERROR(errCode) if (errCode != RADIOLIB_ERR_NONE) { Log::console(PSTR("Radio failed, code %d\n Check that the configuration is valid for your board"), errCode);status.radio_error=errCode; return errCode; }

bool received = false;
bool eInterrupt = true;
bool noisyInterrupt = false;

bool allow_decode=true;

std::string hexToASCII(std::string hex)
{
  std::string ascii = "";
  for (size_t i = 0; i < hex.length(); i += 2)
  {
    // extract two characters from hex string
    std::string part = hex.substr(i, 2);

    // change it into base 16 and typecast as the character
    char ch = (char) strtol(part.c_str(), nullptr, 16);

    // add this char to final ASCII string
    ascii += ch;
  }
  return ascii;
}

Radio::Radio()
#if CONFIG_IDF_TARGET_ESP32S3
  : spi(HSPI)
#elif CONFIG_IDF_TARGET_ESP32C3
  : spi(SPI)  
#else
  : spi(VSPI)
#endif
{
}

void Radio::init()
{
  Power& power = Power::getInstance();
  power.checkAXP();                                       // check and setup AXP192 and AXP2101 power controller
  Log::console(PSTR("[SX12xx] Initializing ... "));
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return;

  spi.begin(board.L_SCK, board.L_MISO, board.L_MOSI, board.L_NSS);

 switch (board.L_radio) {
    case RADIO_SX1278:
      radioHal = new RadioHal<SX1278>(new Module(board.L_NSS, board.L_DI00, board.L_RST, board.L_DI01, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
      moduleNameString="SX1278";
      break;
    case RADIO_SX1276:
      #if CONFIG_IDF_TARGET_ESP32                                    // Heltec Lora 32 V3 patch to enable TCXO
        if (ConfigManager::getInstance().getBoard()== LILYGO_T3_V1_6_1_HF_TCXO ) { 
          Log::console(PSTR("[SX1276] Enable TCXO 33... "));
          pinMode (33, OUTPUT); 
          digitalWrite(33, HIGH);
          delay(50);
        }
      #endif
      radioHal = new RadioHal<SX1276>(new Module(board.L_NSS, board.L_DI00, board.L_RST, board.L_DI01, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
      moduleNameString="SX1276";
      break;
    case RADIO_SX1268:
      radioHal = new RadioHal<SX1268>(new Module(board.L_NSS, board.L_DI01, board.L_RST, board.L_BUSSY, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
      moduleNameString="SX1268";
      break;
    case RADIO_SX1262:
      radioHal = new RadioHal<SX1262>(new Module(board.L_NSS, board.L_DI01, board.L_RST, board.L_BUSSY, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
      moduleNameString="SX1262";
      break;
    case RADIO_SX1280:
      radioHal = new RadioHal<SX1280>(new Module(board.L_NSS, board.L_DI01, board.L_RST, board.L_BUSSY, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
      moduleNameString="SX1280";
    default:
       radioHal = new RadioHal<SX1268>(new Module(board.L_NSS, board.L_DI01, board.L_RST, board.L_BUSSY, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0)));
       moduleNameString="default SX1268";
  }

  if (board.RX_EN != UNUSED && board.TX_EN != UNUSED)
  {
    radioHal->setRfSwitchPins(board.RX_EN, board.TX_EN);
    Log::debug(PSTR("setRfSwitchPins(RxEn->GPIO-%d, TxEn->GPIO-%d)"), board.RX_EN, board.TX_EN);
  }

  begin();
}

int16_t Radio::begin()
{
  status.radio_ready = false;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  
  ModemInfo &m = status.modeminfo;

  if (m.modem_mode == "LoRa")
  {
    if (m.frequency != 0) 
    {
      CHECK_ERROR(radioHal->begin(m.frequency + m.freqOffset, m.bw, m.sf, m.cr, m.sw, m.power, m.preambleLength, m.gain, board.L_TCXO_V));
      if (m.fldro == 2)
        radioHal->autoLDRO();
      else
        radioHal->forceLDRO(m.fldro);

      radioHal->setCRC(m.crc);
    } 
    else 
    {
        CHECK_ERROR(radioHal->begin());
    }
  }
  else
  {
    CHECK_ERROR(radioHal->beginFSK(m.frequency + m.freqOffset, m.bitrate, m.freqDev, m.bw, m.power, m.preambleLength, (m.OOK == 255), board.L_TCXO_V));
    CHECK_ERROR(radioHal->setDataShaping(m.OOK));
    CHECK_ERROR(radioHal->setCRC(0));
    if (m.len!=0) CHECK_ERROR(radioHal->fixedPacketLengthMode(m.len));
    CHECK_ERROR(radioHal->setSyncWord(m.fsw, m.swSize));
    CHECK_ERROR(radioHal->setEncoding(m.enc));
  }

  // set the function that will be called
  // when new packet is received
  // attach the ISR to radio interrupt
  radioHal->setDio0Action(setFlag);
  // start listening for LoRa packets
  Log::console(PSTR("[%s] Starting to listen to %s"), moduleNameString, m.satellite);
  CHECK_ERROR(radioHal->startReceive());
  status.modeminfo.currentRssi = radioHal->getRSSI(false,true);

  status.radio_ready = true;
  return RADIOLIB_ERR_NONE;
}

void Radio::setFlag()
{
  if (received || !eInterrupt)
    noisyInterrupt = true;

  if (!eInterrupt)
    return;

  received = true;
}

void Radio::enableInterrupt()
{ 
  eInterrupt = true;
}

void Radio::disableInterrupt()
{
  eInterrupt = false;
}

void Radio::startRx()
{
  // put module back to listen mode
  radioHal->startReceive();

  // we're ready to receive more packets,
  // enable interrupt service routine
  enableInterrupt();
  }


 void Radio::currentRssi()
{
  // get current RSSI
  status.modeminfo.currentRssi = radioHal->getRSSI(false,true);

}

int16_t Radio::sendTx(uint8_t *data, size_t length)
{
  if (!ConfigManager::getInstance().getAllowTx())
  {
    Log::error(PSTR("TX disabled by config"));
    return -1;
  }
  disableInterrupt();

  // send data
  int16_t state = 0;

  state = radioHal->transmit(data, length);
  radioHal->setDio0Action(setFlag); // TODO: Check, is this needed?? include it inside startRX ??
  startRx();

  return state;
}

int16_t Radio::sendTestPacket()
{
  return sendTx((uint8_t *)TEST_STRING, strlen(TEST_STRING));
}

int16_t Radio::moduleSleep()
{
  return radioHal->sleep();
}

uint8_t Radio::listen()
{
  // check if the flag is set (received interruption)
  if (!received)
    return 1;

  // disable the interrupt service routine while
  // processing the data
  disableInterrupt();

  // reset flag
  received = false;

  size_t respLen = 0;
  uint8_t *respFrame = 0;
  int16_t state = 0;

  PacketInfo newPacketInfo;
  status.lastPacketInfo.crc_error = false;
  // read received data
  respLen = radioHal->getPacketLength();
  // workaround for radiolib FSX fixed packet definition returning always a size of 255bytes
  if (respLen == 255) respLen = status.modeminfo.len;

  respFrame = new uint8_t[respLen];
  state = radioHal->readData(respFrame, respLen);
  newPacketInfo.rssi = radioHal->getRSSI();
  newPacketInfo.snr = radioHal->getSNR();
  newPacketInfo.frequencyerror = radioHal->getFrequencyError();


  // check if the packet info is exactly the same as the last one
  if (newPacketInfo.rssi == status.lastPacketInfo.rssi &&
      newPacketInfo.snr == status.lastPacketInfo.snr &&
      newPacketInfo.frequencyerror == status.lastPacketInfo.frequencyerror)
  {
    Log::console(PSTR("Interrupt triggered but no new data available. Check wiring and electrical interferences."));
    delete[] respFrame;
    startRx();
    return 4;
  }

  struct tm *timeinfo;
  time_t currenttime = time(NULL);
  if (currenttime < 0)
  {
    Log::error(PSTR("Failed to obtain time"));
    status.lastPacketInfo.time = "";
  }
  else
  {
    // store time of the last packet received:
    timeinfo = localtime(&currenttime);
    String thisTime = String(timeinfo->tm_mday) + "/" + String(timeinfo->tm_mon + 1) + "/" + String(timeinfo->tm_year + 1900) + " ";
    // if (timeinfo->tm_hour < 10)
    // {
    //   thisTime = thisTime + " ";
    // } // add leading space if required
    thisTime = thisTime + String(timeinfo->tm_hour) + ":";
    if (timeinfo->tm_min < 10)
    {
      thisTime = thisTime + "0";
    } // add leading zero if required
    thisTime = thisTime + String(timeinfo->tm_min) + ":";
    if (timeinfo->tm_sec < 10)
    {
      thisTime = thisTime + "0";
    } // add leading zero if required
    thisTime = thisTime + String(timeinfo->tm_sec);

    status.lastPacketInfo.time = thisTime;
  }

  status.lastPacketInfo.rssi = newPacketInfo.rssi;
  status.lastPacketInfo.snr = newPacketInfo.snr;
  status.lastPacketInfo.frequencyerror = newPacketInfo.frequencyerror;

  // print RSSI (Received Signal Strength Indicator)
  Log::console(PSTR("[%s] RSSI:\t\t%f dBm\n\t [%s] SNR:\t\t%f dB\n\t [%s] Frequency error:\t%f Hz"),
    moduleNameString, status.lastPacketInfo.rssi, 
    moduleNameString, status.lastPacketInfo.snr, 
    moduleNameString, status.lastPacketInfo.frequencyerror
  );

  if (state == RADIOLIB_ERR_NONE && respLen > 0)
  {
    // read optional data
    // Log::console(PSTR("Packet (%u bytes):"), respLen);

    uint16_t buffSize = respLen * 2 + 1;
    // if (buffSize > 255)
    //   buffSize = 255;

    char *byteStr = new char[buffSize];
    for (int i = 0; i < respLen; i++)
    {
      sprintf(byteStr + i * 2 % (buffSize - 1), "%02x", respFrame[i]);
      // if (i * 2 % buffSize == buffSize - 3 || i == respLen - 1) {
      //   Log::console(PSTR("\tEncoded: %s"), byteStr); // print before the buffer is going to loop back
      // }
    }

    status.lastPacketInfo.encoded_message = byteStr;
    status.lastPacketInfo.decoded_message = hexToASCII(std::string(byteStr)).c_str();

    // Log::console(PSTR("\tDecoded: %s"), hexToASCII(std::string(byteStr)).c_str());
    Log::console(PSTR("Packet (%u bytes):\n\t - Encoded: %s\n\t - Decoded: %s"), respLen, byteStr, status.lastPacketInfo.decoded_message.c_str());
    delete[] byteStr;

    if (allow_decode) {
      String modo=status.modeminfo.modem_mode;
      if (modo=="FSK") {
        int bytes_sincro=0;
          for (int i=0;i<sizeof(status.modeminfo.fsw);i++){
            if (status.modeminfo.fsw[i]!=0){bytes_sincro++;}
          }
          buffSize = (respLen+bytes_sincro) * 2 + 1;
          if (buffSize > 255)
            buffSize = 255;
          char *byteStr_fsk = new char[buffSize];
          for (int i=0;i<bytes_sincro;i++){
            sprintf(byteStr_fsk+(i*2),"%02X", status.modeminfo.fsw[i]);}
          for (int i = 0; i < respLen; i++)
          {
            sprintf(byteStr_fsk + (bytes_sincro + i) * 2 % (buffSize - 1), "%02X", respFrame[i]);
            if (i * 2 % buffSize == buffSize - 3 || i == respLen - 1)
              Log::console(PSTR("%s"), byteStr_fsk); // print before the buffer is going to loop back
          }
          Log::console(PSTR("Packet Buffer Size: %i"), buffSize);
          int coding=Satellites::coding(status.modeminfo.NORAD);
          if (coding==1){
            char *ax25;
            unsigned char *ax25bin;
            size_t sizeAx25bin=0;
            ax25=new char[buffSize];
            ax25bin=new unsigned char[buffSize];
            BitCode::nrz2ax25(byteStr_fsk,buffSize,ax25,ax25bin,&sizeAx25bin);
            Log::console(PSTR("%s"),ax25);
            //RAW packet is replaced by the processed packet.
            respFrame=ax25bin;
            respLen=sizeAx25bin;
          }
          delete[] byteStr_fsk;
      }
    }
    
    // if Filter enabled filter the received packet
    if (status.modeminfo.filter[0] != 0)
    {
      bool filter_flag = false;
      uint8_t filter_size = status.modeminfo.filter[0];
      uint8_t filter_ini = status.modeminfo.filter[1];

      for (uint8_t filter_pos = 0; filter_pos < filter_size; filter_pos++)
      {
        if (status.modeminfo.filter[2 + filter_pos] != respFrame[filter_ini + filter_pos])
          filter_flag = true;
      }

      // if the msg start with tiny (test packet) remove filter
      if (respFrame[0] == 0x54 && respFrame[1] == 0x69 && respFrame[2] == 0x6e && respFrame[3] == 0x79)
        filter_flag = false;

      if (filter_flag)
      {
        Log::console(PSTR("Filter enabled, doesn't looks like the expected satellite packet"));
        delete[] respFrame;
        startRx();
        return 5;
      }
    }

    status.lastPacketInfo.crc_error = false;
    String encoded = base64::encode(respFrame, respLen);
    MQTT_Client::getInstance().sendRx(encoded, noisyInterrupt);
  }
  else if (state == RADIOLIB_ERR_CRC_MISMATCH)
  {
    // packet was received, but is malformed
    status.lastPacketInfo.crc_error = true;

    // if filter is active, filter the CRC errors
    if (status.modeminfo.filter[0] == 0)
    {
      String error_encoded = base64::encode("Error_CRC");
      MQTT_Client::getInstance().sendRx(error_encoded, noisyInterrupt);
    }
    else
    {
      Log::console(PSTR("Filter enabled, Error CRC filtered"));
      delete[] respFrame;
      startRx();
      return 5;
    }
  }

  delete[] respFrame;

  noisyInterrupt = false;

  status.allPackets.push_back(status.lastPacketInfo);

  // put module back to listen mode
  startRx();

  if (state == RADIOLIB_ERR_NONE)
  {
    return 0;
  }
  else if (state == RADIOLIB_ERR_CRC_MISMATCH)
  {
    // packet was received, but is malformed
    Log::console(PSTR("[%s] CRC error! Data cannot be retrieved"), moduleNameString);
    return 2;
  }
  else if (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)
  {
    // packet was received, but is malformed
    Log::console(PSTR("[%S] Damaged header! Data cannot be retrieved"), moduleNameString);
    return 2;
  }
  else
  {
    // some other error occurred
    Log::console(PSTR("[%s] Failed, code %d"), moduleNameString, state);
    return 3;
  }
}

void Radio::readState(int state)
{
  if (state == RADIOLIB_ERR_NONE)
  {
    Log::error(PSTR("success!"));
  }
  else
  {
    Log::error(PSTR("failed, code %d"), state);
    return;
  }
}

// remote
int16_t Radio::remote_freq(char *payload, size_t payload_len)
{
  float frequency = _atof(payload, payload_len);
  Log::console(PSTR("Set Frequency: %.3f MHz"), frequency);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    ((SX1278 *)lora)->sleep(); // sleep mandatory if FastHop isn't ON.
    state = ((SX1278 *)lora)->setFrequency(frequency + status.modeminfo.freqOffset);
    ((SX1278 *)lora)->startReceive();
  }
  else
  {
    ((SX1268 *)lora)->sleep();
    state = ((SX1268 *)lora)->setFrequency(frequency + status.modeminfo.freqOffset);
    ((SX1268 *)lora)->startReceive();
  }

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.frequency = frequency;

  return state;
}

int16_t Radio::remoteSetFreqOffset(char *payload, size_t payload_len)
{
  float frequency_offset = _atof(payload, payload_len);
  Log::console(PSTR("Set Frequency OffSet to %.3f Hz"), frequency_offset);
  status.modeminfo.freqOffset = frequency_offset / 1000000;
  status.radio_ready = false;
  CHECK_ERROR(radioHal->sleep());  // sleep mandatory if FastHop isn't ON.
  CHECK_ERROR(radioHal->setFrequency(status.modeminfo.frequency+status.modeminfo.freqOffset)); 
  CHECK_ERROR(radioHal->startReceive()); 
  status.radio_ready = true;
  return RADIOLIB_ERR_NONE;
}

int16_t Radio::remote_bw(char *payload, size_t payload_len)
{
  float bw = _atof(payload, payload_len);
  Log::console(PSTR("Set bandwidth: %.3f MHz"), bw);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setBandwidth(bw);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->setBandwidth(bw);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.bw = bw;

  return state;
}

int16_t Radio::remote_sf(char *payload, size_t payload_len)
{
  uint8_t sf = _atof(payload, payload_len);
  Log::console(PSTR("Set spreading factor: %u"), sf);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setSpreadingFactor(sf);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->setSpreadingFactor(sf);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);

  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.sf = sf;

  return state;
}

int16_t Radio::remote_cr(char *payload, size_t payload_len)
{
  uint8_t cr = _atoi(payload, payload_len);
  Log::console(PSTR("Set coding rate: %u"), cr);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setCodingRate(cr);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->setCodingRate(cr);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);

  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.cr = cr;

  return state;
}

int16_t Radio::remote_crc(char *payload, size_t payload_len)
{
  bool crc = _atoi(payload, payload_len);
  Log::console(PSTR("Set CRC: %s"), crc ? F("ON") : F("OFF"));
  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setCRC(crc);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->setCRC(crc);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);
  return state;
}

int16_t Radio::remote_lsw(char *payload, size_t payload_len)
{
  uint8_t sw = _atoi(payload, payload_len);
  char strHex[2];
  sprintf(strHex, "%1x", sw);
  Log::console(PSTR("Set lsw: %s"), strHex);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setSyncWord(sw);
  else
    state = ((SX1268 *)lora)->setSyncWord(sw, 0x44);

  readState(state);
  return state;
}

int16_t Radio::remote_fldro(char *payload, size_t payload_len)
{
  bool ldro = _atoi(payload, payload_len);
  Log::console(PSTR("Set ForceLDRO: %s"), ldro ? F("ON") : F("OFF"));

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->forceLDRO(ldro);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->forceLDRO(ldro);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);

  if (state == RADIOLIB_ERR_NONE)
  {
    if (ldro)
      status.modeminfo.fldro = true;
    else
      status.modeminfo.fldro = false;
  }

  return state;
}

int16_t Radio::remote_aldro(char *payload, size_t payload_len)
{
  Log::console(PSTR("Set AutoLDRO "));
  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->autoLDRO();
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->autoLDRO();
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);
  return state;
}

int16_t Radio::remote_pl(char *payload, size_t payload_len)
{
  uint16_t pl = _atoi(payload, payload_len);
  Log::console(PSTR("Set Preamble %u"), pl);
  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setPreambleLength(pl);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    state = ((SX1268 *)lora)->setPreambleLength(pl);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.preambleLength = pl;

  return state;
}

int16_t Radio::remote_begin_lora(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, payload_len);
  float freq = doc[0];
  float bw = doc[1];
  uint8_t sf = doc[2];
  uint8_t cr = doc[3];
  uint8_t syncWord78 = doc[4];
  int8_t power = doc[5];
  uint8_t current_limit = doc[6];
  uint16_t preambleLength = doc[7];
  uint8_t gain = doc[8];
  uint16_t syncWord68 = doc[4];

  char sw78StrHex[2];
  char sw68StrHex[3];
  sprintf(sw78StrHex, "%1x", syncWord78);
  sprintf(sw68StrHex, "%2x", syncWord68);
  Log::console(PSTR("Set Frequency: %.3f MHz\nSet bandwidth: %.3f MHz\nSet spreading factor: %u\nSet coding rate: %u\nSet sync Word 127x: 0x%s\nSet sync Word 126x: 0x%s"), freq, bw, sf, cr, sw78StrHex, sw68StrHex);
  Log::console(PSTR("Set Power: %d\nSet C limit: %u\nSet Preamble: %u\nSet Gain: %u"), power, current_limit, preambleLength, gain);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    ((SX1278 *)lora)->sleep(); // sleep mandatory if FastHop isn't ON.
    state = ((SX1278 *)lora)->begin(freq + status.modeminfo.freqOffset, bw, sf, cr, syncWord78, power, preambleLength, gain);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
  }
  else
  {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board))
      return -1;
    state = ((SX1268 *)lora)->begin(freq + status.modeminfo.freqOffset, bw, sf, cr, syncWord68, power, preambleLength, board.L_TCXO_V);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
  }

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
  {
    status.modeminfo.modem_mode = "LoRa";
    status.modeminfo.frequency = freq;
    status.modeminfo.bw = bw;
    status.modeminfo.power = power;
    status.modeminfo.preambleLength = preambleLength;
    status.modeminfo.sf = sf;
    status.modeminfo.cr = cr;
  }

  return state;
}

int16_t Radio::remote_begin_fsk(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, payload_len);
  float freq = doc[0];
  float br = doc[1];
  float freqDev = doc[2];
  float rxBw = doc[3];
  int8_t power = doc[4];
  uint16_t preambleLength = doc[5];
  uint8_t ook = doc[6]; // ook and datashape
  uint8_t len = doc[7]; // ook and datashape
  

  Log::console(PSTR("Set Frequency: %.3f MHz\nSet bit rate: %.3f\nSet Frequency deviation: %.3f kHz\nSet receiver bandwidth: %.3f kHz\nSet Power: %d"), freq, br, freqDev, rxBw, power);
  Log::console(PSTR("Set Preamble Length: %u\nOOK Modulation %s\nSet datashaping %u"), preambleLength, (ook == 255) ? F("ON") : F("OFF"), ook);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->beginFSK(freq + status.modeminfo.freqOffset, br, freqDev, rxBw, power, preambleLength, (ook == 255));
    ((SX1278 *)lora)->setDataShaping(ook);
    ((SX1278 *)lora)->startReceive();
    ((SX1278 *)lora)->setPacketReceivedAction(setFlag);
    ((SX1278 *)lora)->setCRC(false);
   // ((SX1278 *)lora)->_mod->SPIsetRegValue(SX127X_REG_SYNC_CONFIG, SX127X_PREAMBLE_POLARITY_AA, 5, 5);  
    ((SX1278 *)lora)->fixedPacketLengthMode(len);

  }
  else
  {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board))
      return -1;
    state = ((SX1268 *)lora)->beginFSK(freq + status.modeminfo.freqOffset, br, freqDev, rxBw, power, preambleLength, board.L_TCXO_V);
    ((SX1268 *)lora)->setDataShaping(ook);
    ((SX1268 *)lora)->startReceive();
    ((SX1268 *)lora)->setPacketReceivedAction(setFlag);
    ((SX1268 *)lora)->setCRC(false);
    ((SX1268 *)lora)->fixedPacketLengthMode(len);
    

  }
  readState(state);

  if (state == RADIOLIB_ERR_NONE)
  {
    status.modeminfo.modem_mode = "FSK";
    status.modeminfo.frequency = freq;
    status.modeminfo.bw = rxBw;
    status.modeminfo.power = power;
    status.modeminfo.preambleLength = preambleLength;
    status.modeminfo.bitrate = br;
    status.modeminfo.freqDev = freqDev;
    status.modeminfo.OOK = ook;
    status.modeminfo.len = len;
  }

  return state;
}

int16_t Radio::remote_br(char *payload, size_t payload_len)
{
  uint8_t br = _atoi(payload, payload_len);
  Log::console(PSTR("Set FSK Bit rate: %u"), br);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setBitRate(br);
  else
    state = ((SX1268 *)lora)->setBitRate(br);

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.bitrate = br;

  return state;
}

int16_t Radio::remote_fd(char *payload, size_t payload_len)
{
  uint8_t fd = _atoi(payload, payload_len);
  Log::console(PSTR("Set FSK Frequency Dev.: %u"), fd);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setFrequencyDeviation(fd);
  else
    state = ((SX1268 *)lora)->setFrequencyDeviation(fd);

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.freqDev = fd;

  return state;
}

int16_t Radio::remote_fbw(char *payload, size_t payload_len)
{
  float frequency = _atof(payload, payload_len);
  Log::console(PSTR("Set FSK bandwidth: %.3f kHz"), frequency);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setRxBandwidth(frequency);
  else
    state = ((SX1268 *)lora)->setRxBandwidth(frequency);

  readState(state);
  if (state == RADIOLIB_ERR_NONE)
    status.modeminfo.bw = frequency;

  return state;
}

int16_t Radio::remote_fsw(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, payload_len);
  uint8_t synnwordsize = doc.size();
  uint8_t syncWord[synnwordsize];

  Serial.println("");
  Serial.print(F("Set SyncWord Size "));
  Serial.print(synnwordsize);
  Serial.print(F("-> "));

  for (uint8_t words = 0; words < synnwordsize; words++)
  {
    syncWord[words] = doc[words];
    status.modeminfo.fsw[words] = syncWord[words];
    Serial.print(F(" 0x"));
    Serial.print(syncWord[words], HEX);
    Serial.print(F(", "));
  }

   status.modeminfo.swSize = synnwordsize;

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setSyncWord(syncWord, synnwordsize);
  else
    state = ((SX1268 *)lora)->setSyncWord(syncWord, synnwordsize);

  readState(state);
  return state;
}

int16_t Radio::remote_fook(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(60);
  deserializeJson(doc, payload, payload_len);
  bool enableOOK = doc[0];
  uint8_t ook_shape = doc[1];

  Log::console(PSTR("OOK Modulation: %s"), enableOOK ? F("ON") : F("OFF"));
  Log::console(PSTR("Set OOK datashaping: %u"), ook_shape);

  int state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
  {
    state = ((SX1278 *)lora)->setOOK(enableOOK);
  }
  else
  {
    Log::error(PSTR("OOK not supported by the selected lora module!"));
    return -1;
  }

  readState(state);
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  if (board.L_radio)
    state = ((SX1278 *)lora)->setDataShapingOOK(ook_shape);

  readState(state);
  return state;
}

void Radio::remote_SPIwriteRegister(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(60);
  deserializeJson(doc, payload, payload_len);
  uint8_t reg = doc[0];
  uint8_t data = doc[1];
  Log::console(PSTR("REG ID: 0x%x to 0x%x"), reg, data);
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return;
  //if (board.L_radio)
  //  ((SX1278 *)lora)->_mod->SPIwriteRegister(reg, data);
  //  else
  //   ((SX1268*)lora)->_mod->SPIwriteRegister(reg,data);
}

int16_t Radio::remote_SPIreadRegister(char *payload, size_t payload_len)
{
  uint8_t reg = _atoi(payload, payload_len);
  uint8_t data = 0;

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  //if (board.L_radio)
   // data = ((SX1278 *)lora)->_mod->SPIreadRegister(reg);
  // else
  //   data = ((SX1268*)lora)->_mod->SPIreadRegister(reg);

  Log::console(PSTR("REG ID: 0x%x HEX : 0x%x BIN : %b"), reg, data, data);

  readState(state);
  return data;
}

int16_t Radio::remote_SPIsetRegValue(char *payload, size_t payload_len)
{
  DynamicJsonDocument doc(120);
  deserializeJson(doc, payload, payload_len);
  uint8_t reg = doc[0];
  uint8_t value = doc[1];
  uint8_t msb = doc[2];
  uint8_t lsb = doc[3];
  uint8_t checkinterval = doc[4];

  Serial.println("");
  Serial.print(F("REG ID: 0x"));
  Serial.println(reg, HEX);
  Serial.print(F("to HEX: 0x"));
  Serial.print(value, HEX);
  Serial.print(F("bin : 0x "));
  Serial.println(value, BIN);
  Serial.print(F("msb : "));
  Serial.println(msb);
  Serial.print(F("lsb : "));
  Serial.println(lsb);
  Serial.print(F("check_interval : "));
  Serial.println(checkinterval);

  int16_t state = 0;
  board_t board;
  if (!ConfigManager::getInstance().getBoardConfig(board))
    return -1;
  //if (board.L_radio)
  //  state = ((SX1278 *)lora)->_mod->SPIsetRegValue(reg, value, msb, lsb, checkinterval);
  //else
    //   state = ((SX1268*)lora)->_mod->SPIsetRegValue(reg, value, msb, lsb, checkinterval);

    readState(state);
  return state;
}

double Radio::_atof(const char *buff, size_t length)
{
  char str[length + 1];
  memcpy(str, buff, length);
  str[length] = '\0';
  return atof(str);
}

int Radio::_atoi(const char *buff, size_t length)
{
  char str[length + 1];
  memcpy(str, buff, length);
  str[length] = '\0';
  return atoi(str);
}
