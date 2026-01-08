/*
  Radio.h - Class to handle radio communications
  
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

#ifndef RADIO_H
#define RADIO_H
#define RADIOLIB_EXCLUDE_HTTP

#include <RadioLib.h>
#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"
#include "../Mqtt/MQTT_Client.h"
#include "RadioHal.hpp"
#include "../Power/Power.h"

extern Status status;

enum RadioModelNum {
  RADIO_SX1278 = 1,
  RADIO_SX1276 = 2,
  RADIO_SX1268 = 5,
  RADIO_SX1262 = 6,
  RADIO_SX1280 = 8
};

class Radio {
public:
  static Radio& getInstance()
  {
    static Radio instance; 
    return instance;
  }

  void init();
  int16_t begin();
  void setFrequency();
  void enableInterrupt();
  void disableInterrupt();
  void startRx();
  void tle();
  void currentRssi();
  int16_t moduleSleep();
  uint8_t listen();
  bool isReady() { return status.radio_ready; }
  int16_t remote_freq(char* payload, size_t payload_len);

  int16_t remote_begin_lora(char* payload, size_t payload_len);
  int16_t remote_begin_fsk(char* payload, size_t payload_len);

  int16_t sendTx(uint8_t* data, size_t length);
  int16_t sendTestPacket();
  int16_t remoteSetFreqOffset(char* payload, size_t payload_len);
  void clearPacketReceivedAction();
   
private:
  Radio();
  PhysicalLayer* lora; // TODO: Remove this
  IRadioHal* radioHal;
  void readState(int state);
  static void setFlag();
  SPIClass spi;
  const char* TEST_STRING = "TinyGS-test "; // make sure this always start with "TinyGS-test"!!!
  const char* moduleNameString = "Uninitalised";

  double _atof(const char* buff, size_t length);
  int _atoi(const char* buff, size_t length);
};

#endif
