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
#include <AioP13.h>
//@estbhan
//04/08/2023
#include "../BitCode/BitCode.h"

#define CHECK_ERROR(errCode) if (errCode != RADIOLIB_ERR_NONE) { Log::console(PSTR("Radio failed, code %d\n Check that the configuration is valid for your board"), errCode);status.radio_error=errCode; return errCode; }

bool received = false;
bool eInterrupt = true;
bool noisyInterrupt = false;

bool allow_decode=true;

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
      CHECK_ERROR(radioHal->begin((status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000, m.bw, m.sf, m.cr, m.sw, m.power, m.preambleLength, m.gain, board.L_TCXO_V));
      if (m.fldro == 2)
        radioHal->autoLDRO();
      else
        radioHal->forceLDRO(m.fldro);
      radioHal->setCRC(m.crc);
      radioHal->invertIQ(m.iIQ);
    } 
    else 
    {
        CHECK_ERROR(radioHal->begin());
    }
  }
  else
  {
    CHECK_ERROR(radioHal->beginFSK((status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000, m.bitrate, m.freqDev, m.bw, m.power, m.preambleLength, (m.OOK == 255), board.L_TCXO_V));
    CHECK_ERROR(radioHal->setDataShaping(m.OOK));
    CHECK_ERROR(radioHal->setCRC(0));
    if (m.len!=0) CHECK_ERROR(radioHal->fixedPacketLengthMode(m.len));
    CHECK_ERROR(radioHal->setSyncWord(m.fsw, m.swSize));
    CHECK_ERROR(radioHal->setEncoding(m.enc));
    uint16_t seed;
    if (m.enc==2){
      if (board.L_radio==RADIO_SX1268)
      { 
        CHECK_ERROR(radioHal->setWhitening(true,m.whitening_seed));
      }else{
        Log::console(PSTR("The whitening seed could not be configured for your board"));
      }
    }      
  }

  // set the function that will be called
  // when new packet is received
  // attach the ISR to radio interrupt
  radioHal->setPacketReceivedAction(setFlag);
  // start listening for LoRa packets
  //Log::console(PSTR("[%s] Starting to listen to %s"), moduleNameString, m.satellite);
  Log::console(PSTR("[%s] Starting to listen to %s @ %s mode @ %.4f MHz"), moduleNameString, m.satellite,m.modem_mode,(status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000);
  CHECK_ERROR(radioHal->startReceive());
  status.modeminfo.currentRssi = radioHal->getRSSI(false,true);

  status.radio_ready = true;
  return RADIOLIB_ERR_NONE;
}





void Radio::tle()
{

if (status.modeminfo.tle[0] != 0) 
{

  #define MAP_MAXX    128
  #define MAP_MAXY     64
  
  #define MAP_YOFFSET 20
  int i;
  char tmpstring[100];
  //const char *tleName = "ISS (ZARYA)";
  //const char *tlel1   = "1 25544U 98067A   20300.83097691  .00001534  00000-0  35580-4 0  9996";
  //const char *tlel2   = "2 25544  51.6453  57.0843 0001671  64.9808  73.0513 15.49338189252428";
  //const uint8_t tiny_tle[34] =  { 0x15, 0x01, 0x62, 0x05, 0x4D, 0xB5, 0xED, 0x00, 0x00, 0x04, 0xBB, 0x0E, 0xE8, 0xD3, 0x2C, 0x7D, 0x47, 0x00, 0x00, 0x48, 0x5A, 0x18, 0xE0, 0xEE, 0x1E, 0x14, 0xCD, 0x59, 0xA4, 0x45, 0x1C, 0x00, 0x1A, 0x4F };
  
  const char  *pcMyName = "tinyGS";    // Observer name
  double       dMyLAT   = ConfigManager::getInstance().getLatitude();  // Latitude (Breitengrad): N -> +, S -> -
  double       dMyLON   = ConfigManager::getInstance().getLongitude(); ;  // Longitude (Längengrad): E -> +, W -> -
  double       dMyALT   = status.tle.tgsALT;      // Altitude ASL (m)
  
  double       dfreqRX  = status.modeminfo.frequency;     // Nominal downlink frequency
  double       dfreqTX  = status.modeminfo.frequency;     // Nominal uplink frequency
  
  struct tm *timeinfo;
  time_t currenttime = time(NULL);  // oe6isp  time(NULL) +(status.tle.refresh/2000);       // calculate the TLE for now + the half of the refresh time.
  timeinfo = gmtime(&currenttime);


  int          iYear    = 1900 + timeinfo->tm_year;   // Set start year
  int          iMonth   = 1+ timeinfo->tm_mon;        // Set start month
  int          iDay     = timeinfo->tm_mday;          // Set start day
  int          iHour    = timeinfo->tm_hour;          // Set start hour
  int          iMinute  = timeinfo->tm_min;           // Set start minute
  int          iSecond  = timeinfo->tm_sec;           // Set start second
  
  // Expecting the ISS to be at 289,61° elevation and 20,12° azimuth (Gpredict)
  // Result will be 289,74° elevation and 20,44° azimuth...
  // Expecting the sun to be at -60.79° elevation and 0.86° azimuth (https://www.sunearthtools.com/dp/tools/pos_sun.php)
  // Result will be -60.79° elevation and 0.89° azimuth...
  
  
  double       dSunLAT  = 0;           // Sun latitude
  double       dSunLON  = 0;           // Sun longitude
  double       dSunAZ   = 0;           // Sun azimuth
  double       dSunEL   = 0;           // Sun elevation
  
  //int          ixQTH    = 0;           // Map pixel coordinate x of QTH
  //int          iyQTH    = 0;           // Map pixel coordinate y of QTH
  int          ixSAT    = 0;           // Map pixel coordinate x of satellite
  int          iySAT    = 0;           // Map pixel coordinate y of satellite
  //int          ixSUN    = 0;           // Map pixel coordinate x of sun
  //int          iySUN    = 0;           // Map pixel coordinate y of sun
  
  char         acBuffer[20];            // Buffer for ASCII time
  
  int          aiSatFP[90][2];          // Array for storing the satellite footprint map coordinates
  int          aiSunFP[180][2];         // Array for storing the sunlight footprint map coordinates
  P13Sun Sun;                                                       // Create object for the sun
  P13DateTime MyTime(iYear, iMonth, iDay, iHour, iMinute, iSecond); // Set start time for the prediction
  P13Observer MyQTH(pcMyName, dMyLAT, dMyLON, dMyALT);              // Set observer coordinates
  P13Satellite_tGS MySAT(status.modeminfo.tle);             // Create ISS data from TLE


  // latlon2xy(ixQTH, iyQTH, dMyLAT, dMyLON, MAP_MAXX, MAP_MAXY);      // Get x/y for the pixel map 
  //Serial.printf("\r\nPrediction for %s at %s (MAP %dx%d: x = %d,y = %d):\r\n\r\n", MySAT.c_ccSatName, MyQTH.c_ccObsName, MAP_MAXX, MAP_MAXY, ixQTH, iyQTH);
  //Log::debug(PSTR( "Prediction for %s at %s"), MySAT.c_ccSatName, MyQTH.c_ccObsName);
  //Log::debug(PSTR("Prediction for (Lat = %.2f%c, Lon = %.2f%c), Alt = %.1f m ASL):"), dMyLAT, (char)39, dMyLON, (char)39, dMyALT);
  MyTime.ascii(acBuffer);             // Get time for prediction as ASCII string
  MySAT.predict(MyTime);              // Predict ISS for specific time
  MySAT.latlon(status.tle.dSatLAT, status.tle.dSatLON);     // Get the rectangular coordinates
  MySAT.elaz(MyQTH, status.tle.dSatEL, status.tle.dSatAZ);  // Get azimut and elevation for MyQTH

  latlon2xy(ixSAT, iySAT, status.tle.dSatLAT, status.tle.dSatLON, MAP_MAXX, MAP_MAXY);  // Get x/y for the pixel map
  status.satPos[0]= ixSAT;
  status.satPos[1]= iySAT;
  
  //Log::debug(PSTR("%s -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d) Az: %.2f El: %.2f\r\n\r\n"), acBuffer, status.tle.dSatLAT, status.tle.dSatLON, MAP_MAXX, MAP_MAXY, ixSAT, iySAT, status.tle.dSatAZ, status.tle.dSatEL);
  //Log::debug(PSTR( "%s UTC"), acBuffer);
  //Log::debug(PSTR( "Lat: %.2f%c Lon: %.2f%c Az: %.2f%c El: %.2f%c"), status.tle.dSatLAT, (char)39, status.tle.dSatLON, (char)39, status.tle.dSatAZ, (char)39, status.tle.dSatEL, (char)39);
  //Log::debug(PSTR("RX: %.6f MHz, TX: %.6f MHz\r\n\r\n"), MySAT.doppler(dfreqRX, P13_FRX), MySAT.doppler(dfreqTX, P13_FTX));
  //Log::debug(PSTR( "RX: %.5f MHz"), MySAT.doppler(dfreqRX, P13_FRX));


  status.tle.new_freqDoppler = (MySAT.doppler(dfreqRX, P13_FRX)- dfreqRX )*1000000 ;
  
  
  //Log::console(PSTR("[%s] Starting to listen to %s @ %s mode @ %.4f MHz"), moduleNameString, m.satellite,m.modem_mode,(status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000);


  //Serial.println( status.tle.freqDoppler );
  //sprintf(tmpstring, "TX: %.5f MHz", MySAT.doppler(dfreqTX, P13_FTX));

  
  // Calcualte ISS footprint
  //Serial.printf("Satellite footprint map coordinates:\n\r");
  
   // MySAT.footprint(aiSatFP, (sizeof(aiSatFP)/sizeof(int)/2), MAP_MAXX, MAP_MAXY, status.tle.dSatLAT, status.tle.dSatLON);

  // Predict sun
  //Sun.predict(MyTime);                // Predict ISS for specific time
  //Sun.latlon(dSunLAT, dSunLON);       // Get the rectangular coordinates
  //Sun.elaz(MyQTH, dSunEL, dSunAZ);    // Get azimut and elevation for MyQTH

  //latlon2xy(ixSUN, iySUN, dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY);

  //Serial.printf("\r\nSun -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d) Az: %.2f El: %.2f\r\n\r\n", dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY, ixSUN, iySUN, dSunAZ, dSunEL);

  Log::debug(PSTR("Doppler -> New: %.2f Hz Old: %.2f Hz  Dif: %.2f Hz"),  status.tle.new_freqDoppler, status.tle.freqDoppler, abs( status.tle.new_freqDoppler- status.tle.freqDoppler) );
  if(status.tle.dSatEL > 0) // oe6isp
  {
	if (abs( status.tle.new_freqDoppler- status.tle.freqDoppler) >  status.tle.freqTol) {
    //  status.tle.freqDoppler = status.tle.new_freqDoppler;
      status.tle.freqDoppler = status.tle.new_freqDoppler - status.tle.freqTol; // OE6ISP 
     setFrequency();
	 }
  }
} else {

  status.tle.freqDoppler = 0;

}

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

void Radio::setFrequency()
{
  // get current RSSI
  Log::debug(PSTR("Base: %.4f Mhz Offset: %.1f Hz Doppler: %.1f Hz "),status.modeminfo.frequency, status.modeminfo.freqOffset,status.tle.freqDoppler);
  begin();
  //radioHal->setFrequency( (status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000);
  //Log::debug(PSTR("Base: %.4f Mhz Offset: %.1f Hz Doppler: %.1f Hz --> Modem: %.4f Mhz"),status.modeminfo.frequency, status.modeminfo.freqOffset,status.tle.freqDoppler,(status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000);
  //Serial.print("base: ;
  //Serial.println( status.modeminfo.frequency ,4 );
  //Serial.print("offset: ");
  //Serial.println( status.modeminfo.freqOffset , 4 );
  //Serial.print("Dopler: " );
  //Serial.println( status.tle.freqDoppler ,4 );
  //Serial.print("modem: ");
  //Serial.println( (status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000 , 4);




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
  radioHal->setPacketReceivedAction(setFlag); // TODO: Check, is this needed?? include it inside startRX ??
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
  size_t respLenRaw = 0;
  int16_t state = 0;

  PacketInfo newPacketInfo;
  status.lastPacketInfo.crc_error = false;
  // read received data
  respLen = radioHal->getPacketLength();
  // workaround for radiolib FSX fixed packet definition returning always a size of 255bytes
  if (respLen == 255) respLen = status.modeminfo.len;

 uint8_t* respFrame = new uint8_t[respLen];
 uint8_t* respFrame_raw = new uint8_t[respLen];
 respLenRaw =  respLen;

 state = radioHal->readData(respFrame, respLen);
 memcpy(respFrame_raw, respFrame, respLen);
 
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
    delete[] respFrame_raw;
    startRx();
    return 4;
  }

  status.modeminfolastpckt = status.modeminfo;
  if (status.tle.freqDoppler!=0)  status.lastPacketInfo.freqDoppler = status.tle.freqDoppler; else status.lastPacketInfo.freqDoppler =0;

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
    String thisTime = "";
    if (timeinfo->tm_hour < 10)
    {
      thisTime = thisTime + " ";
    } // add leading space if required
    thisTime = String(timeinfo->tm_hour) + ":";
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
  Log::console(PSTR("[%s] RSSI:\t\t%f dBm\n[%s] SNR:\t\t%f dB\n[%s] Frequency error:\t%f Hz"),
   moduleNameString, status.lastPacketInfo.rssi, 
   moduleNameString, status.lastPacketInfo.snr, 
   moduleNameString, status.lastPacketInfo.frequencyerror);

  if (state == RADIOLIB_ERR_NONE && respLen > 0)
  {
    // read optional data
    Log::console(PSTR("Packet (%u bytes):"), respLen);
    uint16_t buffSize = respLen * 2 + 1;
    if (buffSize > 255)
      buffSize = 255;
    char *byteStr = new char[buffSize];
    for (int i = 0; i < respLen; i++)
    {
      sprintf(byteStr + i * 2 % (buffSize - 1), "%02x", respFrame[i]);
      if (i * 2 % buffSize == buffSize - 3 || i == respLen - 1)
        Log::console(PSTR("%s"), byteStr); // print before the buffer is going to loop back
    }
    delete[] byteStr;


    bool packet_logged=false;
    if (allow_decode){
      String modo=status.modeminfo.modem_mode;
      if (modo=="FSK"){
      int bytes_sincro=0;
      int frame_error=0;
      if (status.modeminfo.framing==1){ //framing=1 -> AX.25 Frame
        Log::console(PSTR("Processing AX.25 frame..."));
        // Add Synch Frame Word to the received data 
        for (int i=0;i<sizeof(status.modeminfo.fsw);i++){
          if (status.modeminfo.fsw[i]!=0){bytes_sincro++;}
        }
        size_t buffSize_pck = bytes_sincro + respLen;
        uint8_t *respFrame_fsk = new uint8_t[buffSize_pck];
        for (int i=0;i<bytes_sincro;i++){
          respFrame_fsk[i]=status.modeminfo.fsw[i];
          }
        for (int i = 0; i < respLen; i++)
        {
          respFrame_fsk[bytes_sincro+i]=respFrame[i];
        }
        uint8_t *ax25bin;
        size_t sizeAx25bin=0;
        ax25bin=new uint8_t[buffSize_pck];
        frame_error=BitCode::nrz2ax25(respFrame_fsk,buffSize_pck,ax25bin,&sizeAx25bin);
        delete[] respFrame_fsk; // Clean up respFrame_fsk
        if (frame_error!=0){
          Log::console(PSTR("Frame error!"));
        }
        //RAW packet is replaced by the processed packet.
        delete[] respFrame; // Clean up original respFrame before reassignment
        respFrame=ax25bin;
        respLen=sizeAx25bin;
      }

      if (status.modeminfo.framing==2){ //framing=2 -> PN9(Fixed 8 bits shift) de-scrambler
        uint8_t *salida;
        size_t sizeSalida=0;              
        salida=new uint8_t[respLen];
        BitCode::pn9(respFrame,respLen,salida);
        respFrame=salida;
      }
      board_t board;
      ConfigManager::getInstance().getBoardConfig(board);
      // check CRC by software if pckt is <65 bytes, of if it's bigger only for modules SX126x 
      if (frame_error==0 && status.modeminfo.crc_by_sw && ( board.L_radio==RADIO_SX1268 || board.L_radio==RADIO_SX1262 || respLen < 65 )){
        size_t newsize=respLen-status.modeminfo.crc_nbytes;
        RadioLibCRCInstance.size = status.modeminfo.crc_nbytes*8;
        RadioLibCRCInstance.poly = status.modeminfo.crc_poly;
        RadioLibCRCInstance.init = status.modeminfo.crc_init;
        RadioLibCRCInstance.out = status.modeminfo.crc_finalxor;
        RadioLibCRCInstance.refIn = status.modeminfo.crc_refIn;
        RadioLibCRCInstance.refOut = status.modeminfo.crc_refOut;
        uint16_t fcs=RadioLibCRCInstance.checksum(respFrame,newsize);
        //If the input is reflected (refIn=true) for the CRC calculation, the CRC value
        //is computed from last two bytes of respFrame reflecting in first place the bytes. 
        //If the input is not reflected (refIn=false) then the CRC calculation is computed
        //with the two last bytes directly taken from respFrame.
        uint8_t msb,lsb,msbinv,lsbinv;
        msb=respFrame[respLen-2];
        lsb=respFrame[respLen-1];
        BitCode::invierte_bits_de_un_byte(msb,&msbinv);
        BitCode::invierte_bits_de_un_byte(lsb,&lsbinv);
        uint16_t crcfield=0;
        if (status.modeminfo.crc_refIn){
          crcfield=msbinv*256+lsbinv;
        }else{
          crcfield=msb*256+lsb;
        }
        Log::console(PSTR("Received CRC: %X Calculated CRC: %X"),crcfield,fcs);
        Log::log_packet(respFrame,respLen);
        packet_logged=true;
        if (fcs!=crcfield){
            status.lastPacketInfo.crc_error = true;
            Log::console(PSTR("Error_CRC"));
            char *cad=new char[10];
            respLen=9;
            sprintf(cad,"Error_CRC");
            for (int i=0;i<9;i++){
              respFrame[i]=(char)cad[i];
            }
            delete[] cad; // Clean up cad
          }          
        }
      }
    }

    if (!packet_logged){Log::log_packet(respFrame,respLen);}

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
        delete[] respFrame_raw;
        startRx();
        return 5;
      }
    }

//    status.lastPacketInfo.crc_error = false;
    String encoded = base64::encode(respFrame, respLen);
    String encoded_raw = base64::encode(respFrame_raw, respLenRaw);
    MQTT_Client::getInstance().sendRx(encoded, noisyInterrupt,encoded_raw);
  }
  else if (state == RADIOLIB_ERR_CRC_MISMATCH || status.lastPacketInfo.crc_error )
  {
    // packet was received, but is malformed
    status.lastPacketInfo.crc_error = true;

    // if filter is active, filter the CRC errors
    if (status.modeminfo.filter[0] == 0)
    {
      String error_encoded = base64::encode("Error_CRC");
      String encoded_raw = base64::encode(respFrame_raw, respLenRaw);
      MQTT_Client::getInstance().sendRx(error_encoded, noisyInterrupt,encoded_raw);
    }
    else
    {
      Log::console(PSTR("Filter enabled, Error CRC filtered"));
      delete[] respFrame;
      delete[] respFrame_raw;
      startRx();
      return 5;
    }
  }

  delete[] respFrame;
  delete[] respFrame_raw;

  noisyInterrupt = false;
  
  // force doppler-recalc
  status.tle.freqDoppler = 99999; // oe6isp
  tle(); // oe6isp

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

  DynamicJsonDocument doc(90);
  deserializeJson(doc, payload, payload_len);

  if (doc.size()==1) {
    float frequency_offset = doc[0];
    Log::console(PSTR("Set Frequency OffSet to %.3f Hz"), frequency_offset);
    status.modeminfo.freqOffset = frequency_offset ;
    return 0;
  }


  if (doc.size()==0) {
     float frequency_offset = _atof(payload, payload_len);
    Log::console(PSTR("Set Frequency OffSet to %.3f Hz"), frequency_offset);
    status.modeminfo.freqOffset = frequency_offset ;
    return 0;
  } 


  if (doc.size()==3) {
    float frequency_offset = doc[0];
    status.tle.freqTol =  doc[1];
    status.tle.refresh =  doc[2];
    Log::console(PSTR("Set Frequency OffSet to %.3f Hz  Tol: %d Hz Refresh: %d ms"), frequency_offset,status.tle.freqTol,status.tle.refresh);
    status.modeminfo.freqOffset = frequency_offset ;
    return 0;
  }




/*
  float frequency_offset = _atof(payload, payload_len);
  Log::console(PSTR("Set Frequency OffSet to %.3f Hz"), frequency_offset);
  status.modeminfo.freqOffset = frequency_offset ;
  */
  
  //status.radio_ready = false;
  //CHECK_ERROR(radioHal->sleep());  // sleep mandatory if FastHop isn't ON.
  //delay(150);
  //CHECK_ERROR(radioHal->setFrequency(status.modeminfo.frequency * 1000000 + (status.modeminfo.freqOffset +  status.tle.freqDoppler)) / 1000000); 
  //delay(150);
  //CHECK_ERROR(radioHal->startReceive()); 
  //delay(150);
  //status.radio_ready = true;
  return RADIOLIB_ERR_NONE;
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
