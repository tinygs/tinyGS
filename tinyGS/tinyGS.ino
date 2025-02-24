/***********************************************************************
  tinyGS.ini - GroundStation firmware
  
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

  ***********************************************************************

  TinyGS is an open network of Ground Stations distributed around the
  world to receive and operate LoRa satellites, weather probes and other
  flying objects, using cheap and versatile modules.

  This project is based on ESP32 boards and is compatible with sx126x and
  sx127x you can build you own board using one of these modules but most
  of us use a development board like the ones listed in the Supported
  boards section.

  Supported boards
    Heltec WiFi LoRa 32 V1 (433MHz & 863-928MHz versions)
    Heltec WiFi LoRa 32 V2 (433MHz & 863-928MHz versions)
    TTGO LoRa32 V1 (433MHz & 868-915MHz versions)
    TTGO LoRa32 V2 (433MHz & 868-915MHz versions)
    TTGO LoRa32 V2 (Manually swapped SX1267 to SX1278)
    T-BEAM + OLED (433MHz & 868-915MHz versions)
    T-BEAM V1.0 + OLED
    FOSSA 1W Ground Station (433MHz & 868-915MHz versions)
    ESP32 dev board + SX126X with crystal (Custom build, OLED optional)
    ESP32 dev board + SX126X with TCXO (Custom build, OLED optional)
    ESP32 dev board + SX127X (Custom build, OLED optional)

  Supported modules
    sx126x
    sx127x

    Web of the project: https://tinygs.com/
    Github: https://github.com/G4lile0/tinyGS
    Main community chat: https://t.me/joinchat/DmYSElZahiJGwHX6jCzB3Q

    In order to onfigure your Ground Station please open a private chat to get your credentials https://t.me/tinygs_personal_bot
    Data channel (station status and received packets): https://t.me/tinyGS_Telemetry
    Test channel (simulator packets received by test groundstations): https://t.me/TinyGS_Test

    Developers:
      @gmag12       https://twitter.com/gmag12
      @dev_4m1g0    https://twitter.com/dev_4m1g0
      @g4lile0      https://twitter.com/G4lile0

====================================================
  IMPORTANT:
    - Follow this guide to get started: https://github.com/G4lile0/tinyGS/wiki/Quick-Start
    - Arduino IDE is NOT recommended, please use Platformio: https://github.com/G4lile0/tinyGS/wiki/Platformio

**************************************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "src/ConfigManager/ConfigManager.h"
#include "src/Display/Display.h"
#include "src/Mqtt/MQTT_Client.h"
#include "src/Status.h"
#include "src/Radio/Radio.h"
#include "src/ArduinoOTA/ArduinoOTA.h"
#include "src/OTA/OTA.h"
#include "src/Logger/Logger.h"
#include "time.h"


#if  RADIOLIB_VERSION_MAJOR != (0x06) || RADIOLIB_VERSION_MINOR != (0x04) || RADIOLIB_VERSION_PATCH != (0x00) || RADIOLIB_VERSION_EXTRA != (0x00)
#error "You are not using the correct version of RadioLib please copy TinyGS/lib/RadioLib on Arduino/libraries"
#endif

#ifndef RADIOLIB_GODMODE
#if !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /RadioLib/src/BuildOpt.h and uncomment #define RADIOLIB_GODMODE around line 367" 
#endif
#endif

ConfigManager& configManager = ConfigManager::getInstance();
MQTT_Client& mqtt = MQTT_Client::getInstance();
Radio& radio = Radio::getInstance();

const char* ntpServer = "time.cloudflare.com";

//////////////////////////////////////////////
//Measure of Chip Temperature and Compensation
//////////////////////////////////////////////
bool allow_logging_verbose_freq_comp=false;
bool allow_logging_freq_comp=true;
unsigned long now=0;
unsigned long last_measu_temp=0;
unsigned long last_log_temp=0;
unsigned long last_write_temp=0;
float tempe_measurement=0.0;
float last_tempe_measurement=0.0;
float temperatura=-90.0;
float freq_off=0.0;
float t1,t2,f1,f2;
float frequency_deviation_limit=10000;//In absolute value
//////////////////////////////////////////////

// Global status
Status status;

void printControls();
void switchTestmode();
void checkButton();
void setupNTP();

void configured()
{
  configManager.setConfiguredCallback(NULL);
  configManager.printConfig();
  radio.init();
}

void wifiConnected()
{
  configManager.setWifiConnectionCallback(NULL);
  setupNTP();
  displayShowConnected();
  arduino_ota_setup();
  configManager.delay(100); // finish animation

  if (configManager.getLowPower())
  {
    Log::debug(PSTR("Set low power CPU=80Mhz"));
    setCpuFrequencyMhz(80); //Set CPU clock to 80MHz
  }

  configManager.delay(400); // wait to show the connected screen and stabilize frequency
}

void setup()
{ 
#if CONFIG_IDF_TARGET_ESP32C3
  setCpuFrequencyMhz(160);
#else
  setCpuFrequencyMhz(240);
#endif
  Serial.begin(115200);
  delay(100);
  Log::console(PSTR("TinyGS Version %d - %s"), status.version, status.git_version);
  Log::console(PSTR("Chip  %s - %d"),  ESP.getChipModel(),ESP.getChipRevision());
  configManager.setWifiConnectionCallback(wifiConnected);
  configManager.setConfiguredCallback(configured);
  configManager.init();
  if (configManager.isFailSafeActive())
  {
    configManager.setConfiguredCallback(NULL);
    configManager.setWifiConnectionCallback(NULL);
    Log::console(PSTR("FATAL ERROR: The board is in a boot loop, rescue mode launched. Connect to the WiFi AP: %s, and open a web browser on ip 192.168.4.1 to fix your configuration problem or upload a new firmware."), configManager.getThingName());
    return;
  }
  // make sure to call doLoop at least once before starting to use the configManager
  configManager.doLoop();
  board_t board;
  if(configManager.getBoardConfig(board))
    pinMode (board.PROG__BUTTON, INPUT_PULLUP);
  displayInit();
  displayShowInitialCredits();
  configManager.delay(1000);
  mqtt.begin();

  if (configManager.getOledBright() == 0)
  {
    displayTurnOff();
  }
  
  printControls();
}

void loop() {  
  configManager.doLoop();
  if (configManager.isFailSafeActive())
  {
    static bool updateAttepted = false;
    if (!updateAttepted && configManager.isConnected()) {
      updateAttepted = true;
      OTA::update(); // try to update as last resource to recover from this state
    }

    if (millis() > 10000 || updateAttepted)
      configManager.forceApMode(true);
    
    return;
  }

  ArduinoOTA.handle();
  handleSerial();

  if (configManager.getState() < 2) // not ready or not configured
  {
    displayShowApMode();
    return;
  }
  
  // configured and no connection
  checkButton();
  if (radio.isReady())
  {
    status.radio_ready = true;
    radio.listen();
  }
  else {
    status.radio_ready = false;
  }

  if (configManager.getState() < 4) // connection or ap mode
  {
    displayShowStaMode(configManager.isApMode());
    return;
  }

  // connected

  mqtt.loop();
  OTA::loop();
  if (configManager.getOledBright() != 0) displayUpdate();

  if (configManager.getAllowFreqComp()){ 
    //////////////////////////////////////////////
    //Measure of Chip Temperature and Compensation
    //////////////////////////////////////////////
    now = millis();
    if ((now - last_measu_temp > 60000) || last_measu_temp==0)
    {
      last_measu_temp = now;
      tempe_measurement=temperatureRead();
      //22/09/2022
      //For some reason, the value 53.33 is a recurrent value provided
      //by the board which seems to do not correspond to a real measurement.
      //For this reason, when this value is provided that value
      //it is not considered for frequency correction.
      if (abs(tempe_measurement-53.333)>0.001){
        temperatura=tempe_measurement;
      }else{
        //Log::console(PSTR("Temperature %2.2f ºC not considered"),tempe_measurement);
        last_measu_temp=now-30000;//Acelerate the measure to do it in 30"
      }
      if ((now-last_write_temp>60000)
        && temperatura!=-90.0 
        && last_tempe_measurement!=temperatura){

        //This determine the offset of the station
        t1=configManager.get_temp_1(); f1=configManager.get_freq_dev_1();
        t2=configManager.get_temp_2(); f2=configManager.get_freq_dev_2();
        //Log::console(PSTR("Temp 1: %.2f Freq 1: %.2f Temp 2: %.2f Freq 1: %.2f Temperature: %.2f"),t1,f1,t2,f2,temperatura);
        if (t1!=-90.0 && t2!=-90.0 && f1!=-20000.0 && f2!=-20000.0){
            freq_off=f1+(((f2-f1)/(t2-t1))*(temperatura-t1));
            //The negative value (-freq_off) compensates the deviation calculated in the line above
            if (abs(freq_off)<frequency_deviation_limit) {
              status.modeminfo.freqOffset=-freq_off/1000000.0;
            }else{
              status.modeminfo.freqOffset=0.0;
              Log::console(PSTR("Frequency Deviation calculation exceed %5.1f Hz. Frequency Deviation set to 0 Hz."),frequency_deviation_limit);
            }
            last_tempe_measurement=temperatura;
            //For the new frequency offset is taken, the current satellite config needs to be commanded.
            radio.begin();
            //Log::console (PSTR("Chip Ta: %2.2f ºC -> Set Rx Freq to: %.6f Hz")
            if (allow_logging_verbose_freq_comp 
            || (allow_logging_freq_comp && (now-last_log_temp)>60000)
            || last_log_temp==0  ) {last_log_temp=now;
                Log::console (PSTR("Chip Ta: %2.2f ºC -> FreqOffset: %.6f MHz")
                              ,temperatura
                              // ,status.modeminfo.frequency
                              // ,-freq_off
                              ,status.modeminfo.freqOffset);
            }

            last_write_temp=now;
            temperatura=-90.0;
        }else{
          Log::console(PSTR("Check Curve Temperature parameters. Frequency Correction not performed."));
        }
      }
    }
  }else{
    if (status.modeminfo.freqOffset!=0.0){
      status.modeminfo.freqOffset=0.0;
      last_measu_temp==0;
      radio.begin();
    }
  }
  /////////////////////////////////////////////////
}

void setupNTP()
{
  configTime(0, 0, ntpServer);
  setenv("TZ", configManager.getTZ(), 1); 
  tzset();
  
  configManager.delay(1000);
}

void checkButton()
{
  #define RESET_BUTTON_TIME 8000
  static unsigned long buttPressedStart = 0;
  board_t board;
  
  if (configManager.getBoardConfig(board) && !digitalRead (board.PROG__BUTTON))
  {
    if (!buttPressedStart)
    {
      buttPressedStart = millis();
    }
    else if (millis() - buttPressedStart > RESET_BUTTON_TIME) // long press
    {
      Log::console(PSTR("Rescue mode forced by button long press!"));
      Log::console(PSTR("Connect to the WiFi AP: %s and open a web browser on ip 192.168.4.1 to configure your station and manually reboot when you finish."), configManager.getThingName());
      configManager.forceDefaultPassword(true);
      configManager.forceApMode(true);
      buttPressedStart = 0;
    }
  }
  else {
    unsigned long elapsedTime = millis() - buttPressedStart;
    if (elapsedTime > 30 && elapsedTime < 1000) // short press
      displayNextFrame();
    buttPressedStart = 0;
  }
}

void handleSerial()
{
  if(Serial.available())
  {
    radio.disableInterrupt();

    // get the first character
    char serialCmd1 = Serial.read();
    char serialCmd = ' ';

    // wait for a bit to receive any trailing characters
    configManager.delay(500);
    if (serialCmd1 == '!') serialCmd = Serial.read();
    // dump the serial buffer
    while(Serial.available())
    {
      Serial.read();
    }

    // process serial command
    switch(serialCmd) {
      case ' ':
      break;         
      case 'e':
        configManager.resetAllConfig();
        ESP.restart();
        break;
      case 'b':
        ESP.restart();
        break;
      case 'p':
        if (!configManager.getAllowTx())
        {
          Log::console(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
          break;
        }

        static long lastTestPacketTime = 0;
        if (millis() - lastTestPacketTime < 20*1000)
        {
          Log::console(PSTR("Please wait a few seconds to send another test packet."));
          break;
        }
        
        radio.sendTestPacket();
        lastTestPacketTime = millis();
        Log::console(PSTR("Sending test packet to nearby stations!"));
        break;
      default:
        Log::console(PSTR("Unknown command: %c"), serialCmd);
        break;
    }

    radio.enableInterrupt();
  }
}

// function to print controls
void printControls()
{
  Log::console(PSTR("------------- Controls -------------"));
  Log::console(PSTR("!e - erase board config and reset"));
  Log::console(PSTR("!b - reboot the board"));
  Log::console(PSTR("!p - send test packet to nearby stations (to check transmission)"));
  Log::console(PSTR("------------------------------------"));
}