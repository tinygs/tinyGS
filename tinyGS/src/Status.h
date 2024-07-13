/*
  Status.h - Data structure for system status
  
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

#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#ifndef Status_h
#define Status_h

#include <vector>

struct PacketInfo {
  String time = "Waiting";
  String encoded_message = "";
  String decoded_message = "";
  float rssi = 0;
  float snr = 0;
  float frequencyerror = 0; // Hz
  bool crc_error = false;
};

struct ModemInfo {
  char satellite[25]  = "Waiting";
  String  modem_mode  = "LoRa" ;     // 1-LoRa  2-FSK  3-GMSK
  float   frequency   = 0; // MHz  
  float   freqOffset  = 0;       // Hz 
  float   bw          = 0;   // kHz dual sideban
  uint8_t sf          = 0 ;
  uint8_t cr          = 0 ;
  uint8_t sw          = 18;
  int8_t  power       = 5 ;
  uint16_t preambleLength = 8;
  float  	bitrate     = 9.6 ;
  float   freqDev     = 5.0;
  uint8_t    OOK      = 0; // 0 disable  1 -> 0.3  2-> 0.5  3 -> 0.6  4-> 1.0
  bool    crc         = true;
  uint8_t fldro       = 1;
  uint8_t gain        = 0;
  uint32_t  NORAD     = 46494;  // funny this remember me WARGames
  uint8_t   fsw[8]    = {0,0,0,0,0,0,0,0};
  uint8_t   swSize    = 0;
  uint8_t   filter[8] = {0,0,0,0,0,0,0,0};
  uint8_t   len       = 64;     // FSK expected lenght in packet mode
  uint8_t   enc       = 0;      // FSK  transmission encoding. (0 -> NRZ(sx127x, sx126x)(defaul).  1 -> MANCHESTER(sx127x), WHITENING(sx126x).  2 -> WHITENING(sx127x, sx126x). 10 -> NRZ(sx127x), WHITENING(sx126x).
  float currentRssi = 0;
};

struct TextFrame {   
  uint8_t text_font;
  uint8_t text_alignment;
  int16_t text_pos_x;
  int16_t text_pos_y; 
  String text = "123456789012345678901234567890";
};

struct Status {
  const uint32_t version = 2403241; // version year month day release
  const char* git_version = GIT_VERSION;
  bool mqtt_connected = false;
  bool radio_ready = false;
  int16_t radio_error = 0;
  PacketInfo lastPacketInfo;
  std::vector<PacketInfo> allPackets;
  ModemInfo modeminfo;
  float satPos[2] = {0, 0};
  uint8_t remoteTextFrameLength[4] = {0, 0, 0, 0};
  TextFrame remoteTextFrame[4][15];
  float time_offset = 0;
 };

#endif
