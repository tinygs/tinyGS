/*
  Display.cpp - Class responsible of controlling the display
  
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

#include "Display.h"
#include "graphics.h"
#include "../Network/ConfigStore.h"
#include "../Mqtt/MQTT_credentials.h"
#include "../Logger/Logger.h"
#include <Wire.h>

SSD1306* display;
OLEDDisplayUi* ui = NULL;

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame7(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawFrame8(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
// void drawFrame9(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);


uint8_t frameCount = 8;
FrameCallback frames[] = { drawFrame1, drawFrame8, drawFrame3, drawFrame5, drawFrame2, drawFrame4, drawFrame6,drawFrame7 };
uint8_t overlaysCount = 1;
OverlayCallback overlays[] = { msOverlay };

unsigned long tick_interval;
int tick_timing = 100;
int graphVal = 1;
int delta = 1;
uint8_t oldOledBright = -1; // to force brightness update on first run

void displayInit()
{
  board_t board;
   if (!ConfigStore::getInstance().getBoardConfig(board))
    return;

  if (board.OLED__address == 0) {
    LOG_CONSOLE(PSTR("OLED disabled (ADDR=0)."));
    return;
  }

  #if CONFIG_IDF_TARGET_ESP32S3  // Heltec Lora 32 V3: enable Vext that powers the OLED
  if (ConfigStore::getInstance().getBoard() == HELTEC_LORA32_V3) {
    pinMode(36, OUTPUT);
    digitalWrite(36, LOW);
  }
  #endif

  // Pulse RST before probing — many boards (e.g. Heltec V1/V2) won't respond
  // on I2C until after the reset sequence.
  if (board.OLED__RST != UNUSED) {
    if (!((strcmp(ESP.getChipModel(), "ESP32-PICO-D4") == 0) && (board.OLED__RST == 16))) {
      pinMode(board.OLED__RST, OUTPUT);
      digitalWrite(board.OLED__RST, LOW);
      delay(50);
      digitalWrite(board.OLED__RST, HIGH);
    }
  }

  // Probe I2C — prevents hanging if no physical display is connected.
  Wire.begin(board.OLED__SDA, board.OLED__SCL);
  Wire.setTimeOut(50); // 50 ms max per transaction
  Wire.beginTransmission(board.OLED__address);
  if (Wire.endTransmission() != 0) {
    LOG_CONSOLE(PSTR("OLED not found at 0x%02X (SDA=%d SCL=%d), display disabled."),
                board.OLED__address, board.OLED__SDA, board.OLED__SCL);
    return; // ui stays NULL → all displayXxx() calls gracefully skip
  }

  LOG_CONSOLE(PSTR("OLED detected at 0x%02X (SDA=%d SCL=%d RST=%d), initializing..."),
              board.OLED__address, board.OLED__SDA, board.OLED__SCL, board.OLED__RST);

  display = new SSD1306(board.OLED__address, board.OLED__SDA, board.OLED__SCL);

  ui = new OLEDDisplayUi(display);
  ui->setTargetFPS(60);
  ui->setActiveSymbol(activeSymbol);
  ui->setInactiveSymbol(inactiveSymbol);
  ui->setIndicatorPosition(BOTTOM);
  ui->setIndicatorDirection(LEFT_RIGHT);
  ui->setFrameAnimation(SLIDE_LEFT);
  ui->setFrames(frames, frameCount);
  ui->setOverlays(overlays, overlaysCount);

  /* ui->init() also initialises the underlying display */
  ui->init();

  // Restore I2C timeout — SSD1306Wire::connect() inside ui->init() calls
  // Wire.begin() again which may reset the timeout set earlier.
  Wire.setTimeOut(50);

  if (ConfigStore::getInstance().getFlipOled())
    display->flipScreenVertically();
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);

  struct tm* timeinfo;
  time_t currenttime = time (NULL);
  if(currenttime < 0)
  {
    LOG_ERROR(PSTR("Failed to obtain time"));
    return;
  }
  timeinfo = localtime (&currenttime);

  // Usar buffer estático para evitar fragmentación del heap
  char timeBuffer[12];
  snprintf(timeBuffer, sizeof(timeBuffer), "%2d:%02d:%02d", 
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  display->drawString(128, 0, timeBuffer);

  if (ConfigStore::getInstance().getDayNightOled())
  {
    if (timeinfo->tm_hour < 6 || timeinfo->tm_hour > 18) display->normalDisplay(); else display->invertDisplay(); // change the OLED according to the time. 
  }
}

void drawRemoteFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y, uint8_t frameNumber)
{
  if (status.remoteTextFrameLength[frameNumber] == 0) ui->nextFrame();

  for (uint8_t n = 0; n < status.remoteTextFrameLength[frameNumber]; n++)
  {
    switch (status.remoteTextFrame[frameNumber][n].text_font)
    {
      case 2:
        display->setFont(ArialMT_Plain_16);
        break;
      default:
        display->setFont(ArialMT_Plain_10);
        break;
    }

    // 0 Left  1 Right  2 Center  3 Center Both
    switch (status.remoteTextFrame[frameNumber][n].text_alignment) {
      case 1:
        display->setTextAlignment(TEXT_ALIGN_RIGHT);
        break;
      case 2:
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        break;
      case 3:
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        break;
      default:
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        break;
    }
    display->drawString(x+status.remoteTextFrame[frameNumber][n].text_pos_x, y+ status.remoteTextFrame[frameNumber][n].text_pos_y,  status.remoteTextFrame[frameNumber][n].text);
  }
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  ConfigStore& configStore = ConfigStore::getInstance();

  display->drawXbm(x +10, y , Logo_width, Logo_height, Logo_bits);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  char staBuf[40];
  snprintf(staBuf, sizeof(staBuf), "Sta: %s", configStore.getThingName());
  display->drawString( x+70, y + 32, staBuf);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  drawRemoteFrame(display, state, x, y, 0);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  if (!status.radio_ready)
  {
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 0, "LoRa initialization failed.");
    display->drawString(0, 14, "Browse " + WiFi.localIP().toString());
    display->drawString(0, 28, "Ensure board selected");
    display->drawString(0, 42, "matches your hardware");

    return;
  }

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x,  y,  status.modeminfo.satellite);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  
  // Usar buffer estático para evitar fragmentación del heap
  char displayBuffer[40];
  snprintf(displayBuffer, sizeof(displayBuffer), "%s @ %.4fMHz", status.modeminfo.modem_mode, status.modeminfo.frequency);
  display->drawString(64+ x,  12 + y, displayBuffer);
  //display->drawString(x,  12 + y, "F:" );
  //display->setTextAlignment(TEXT_ALIGN_RIGHT);
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);

  if (strcmp(status.modeminfo.modem_mode, "LoRa") == 0)
  {
    snprintf(displayBuffer, sizeof(displayBuffer), "SF: %u", status.modeminfo.sf);
    display->drawString(x,  23 + y, displayBuffer);
    if (ConfigStore::getInstance().getAllowTx())
    {
      snprintf(displayBuffer, sizeof(displayBuffer), "Pwr:%ddBm", status.modeminfo.power);
      display->drawString(x,  34 + y, displayBuffer); 
    }
    else
    {
      display->drawString(x,  34 + y, "TX OFF"); 
    }
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    snprintf(displayBuffer, sizeof(displayBuffer), "BW:%.1fkHz", status.modeminfo.bw);
    display->drawString(128 + x,  23 + y, displayBuffer);
    snprintf(displayBuffer, sizeof(displayBuffer), "CR: %u", status.modeminfo.cr);
    display->drawString(128 + x,  34 + y, displayBuffer);
  } 
  else
  {
    display->drawString(x,  23 + y, "FD/BW: " );
    if (ConfigStore::getInstance().getAllowTx()) {
      snprintf(displayBuffer, sizeof(displayBuffer), "P:%ddBm", status.modeminfo.power);
      display->drawString(x,  34 + y, displayBuffer); 
    }
    else
    {
      display->drawString(x,  34 + y, "TX OFF"); 
    }
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    snprintf(displayBuffer, sizeof(displayBuffer), "%.1f/%.1fkHz", status.modeminfo.freqDev, status.modeminfo.bw);
    display->drawString(128 + x,  23 + y, displayBuffer);
    snprintf(displayBuffer, sizeof(displayBuffer), "%.1fkbps", status.modeminfo.bitrate);
    display->drawString(128 + x,  34 + y, displayBuffer);
  }
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  drawRemoteFrame(display, state, x, y, 1);
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->drawXbm(x , y , earth_width, earth_height, earth_bits);
  display->setColor(BLACK);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->fillRect(83,0,128,11);
  display->setFont(ArialMT_Plain_10);
 
  if (status.satPos[0] == 0 && status.satPos[1] == 0)
  {
    String msg = F("Waiting for Sat Pos");
    display->drawString( 65+x,  49+y+(x/2), msg );
    display->drawString( 63+x,  51+y+(x/2), msg );
    display->setColor(WHITE);
    display->drawString( 64+x,  50+y+(x/2), msg );
  }
  else 
  {
    if ((millis()-tick_interval)>tick_timing)
    {
      // Change the value to plot
      graphVal+=delta;
      tick_interval=millis();
      // If the value reaches a limit, then change delta of value
      if (graphVal >= 6)      { delta = -1;  tick_timing=50; }// ramp down value
      else if (graphVal <= 1) { delta = +1; tick_timing=100; } // ramp up value
    }

    display->fillCircle(status.satPos[0]+x, status.satPos[1]+y, graphVal+1);
    display->setColor(WHITE);
    display->drawCircle(status.satPos[0]+x, status.satPos[1]+y, graphVal);
    display->setColor(BLACK);
    display->drawCircle(status.satPos[0]+x, status.satPos[1]+y, (graphVal/3)+1);
    display->setColor(WHITE);
    display->drawCircle(status.satPos[0]+x, status.satPos[1]+y, graphVal/3);
  }
}

void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  drawRemoteFrame(display, state, x, y, 2);
}

void drawFrame7(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  drawRemoteFrame(display, state, x, y, 3);
}

void drawFrame8(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x+2, 16+y, "MQTT:" );
  if (status.mqtt_connected) { display->drawString( x+7,  26+y, "ON"); }  else { display->drawString( x+5,  26+y, "OFF"); }
  display->drawXbm(x + 32, y + 4, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  // The coordinates define the center of the text
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 42 + y, "Connected " + (WiFi.localIP().toString()));
}  

void displayShowConnected()
{
  if (!ui) return;
  display->clear();
  display->drawXbm(34, 0 , WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 , 34 , "WiFi:" + String(ConfigStore::getInstance().getWifiSSID()));
  display->drawString(64 , 44 , "OTP --->    " + String(mqttCredentials.getOTPCode ()) );
  display->drawString(64 ,53 , (WiFi.localIP().toString()));
  display->display();
}

void displayShowInitialCredits()
{
  if (!ui) return;
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,5,"tinyGS");
  display->setFont(ArialMT_Plain_10);
  display->drawString(50,23,"ver. " + String(status.version));

  display->drawString(5,38,"by @gmag12 @4m1g0");
  display->drawString(40,52,"& @g4lile0");
  display->display();
}

void displayShowApMode()
{
  if (!ui) return;
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 6,"Connect to AP:");
  display->drawString(0,18,"->"+String(ConfigStore::getInstance().getThingName()));
  display->drawString(5,32,"to configure your Station");
  display->drawString(10,52,"IP:   192.168.4.1");
  display->display();
}

void displayShowStaMode(bool ap)
{
  if (!ui) return;
  display->clear();
  display->drawXbm(34, 0 , WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 , 35 , "Connecting " + String(ConfigStore::getInstance().getWifiSSID()));
  if (ap)
    display->drawString(64 , 52 , "Config AP available");
  display->display();
}

/**
 * Updates the display brightness based on the configuration.
 * If the brightness has changed, it sets the new brightness or turns off the display.
 * Also updates the UI if screen is on.
 */
void displayUpdate()
{
  if (!ui) return;
  // Get the current OLED brightness from configuration
  uint8_t oledBright = ConfigStore::getInstance().getOledBright();

  // Check if brightness has changed
  if (oldOledBright != oledBright) {
    if (oledBright) {
      // Set the new brightness
      display->setBrightness(2*oledBright);
    } else {
      // Turn off the display if brightness is 0
      displayTurnOff();
    }
    // Save the brightness value
    oldOledBright = oledBright;
  }

  if (oledBright) {
    // Update the UI if screen is on
    ui->update();
  }
}

void displayTurnOff()
{
  if (!ui) return;
  display->displayOff();
  oldOledBright = 0;
}

void displayNextFrame() {
  if (ui)
    ui->nextFrame();
}
