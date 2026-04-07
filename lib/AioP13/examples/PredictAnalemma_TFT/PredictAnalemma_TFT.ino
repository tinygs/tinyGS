/* ====================================================================

   Copyright (c) 2019-2021 Thorsten Godau (https://github.com/dl9sec)
   All rights reserved.


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

   3. Neither the name of the author(s) nor the names of any contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   ====================================================================*/

// Used to check the sun's prediction algorithm.
// See https://en.wikipedia.org/wiki/Analemma for a detailed description
// of what an analemma is.

#include <AioP13.h>
//#include <M5Stack.h>
#include <ESP32-Chimera-Core.h>
#include "graphics.h"

#define MAP_MAXX    320
#define MAP_MAXY    160

#define MAP_YOFFSET 40

int          iYear    = 2020;        // Set start year
int          iMonth   = 1;           // Set start month
int          iDay     = 1;           // Set start day
int          iHour    = 12;          // Set start hour
int          iMinute  = 0;           // Set start minute
int          iSecond  = 0;           // Set start second

double       dSunLAT  = 0;           // Sun latitude
double       dSunLON  = 0;           // Sun longitude

int          ixSUN    = 0;           // Map pixel coordinate x of sun
int          iySUN    = 0;           // Map pixel coordinate y of sun

                            // -, J , F , M , A , M , J , J , A , S , O , N , D
uint8_t      dayspermonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


void setup()
{
  int i;
  char tmpstring[100];

  uint8_t u8month, u8day;

  M5.begin();
  #if !defined(_CHIMERA_CORE_)
    M5.Power.begin();  // Only for original M5Stack library
  #endif
  M5.Lcd.setBrightness(100);
  M5.Lcd.setSwapBytes(true);  // Swap the colour byte order when rendering
  
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(10);  

  P13Sun Sun;                                                       // Create object for the sun
  P13DateTime MyTime(iYear, iMonth, iDay, iHour, iMinute, iSecond); // Set start time for the prediction
   
  //M5.Lcd.pushImage(0, MAP_YOFFSET, worldmap_1_320x160_width, worldmap_1_320x160_height, worldmap_2_320x160_data);
  M5.Lcd.pushImage(0, MAP_YOFFSET, worldmap_2_320x160_width, worldmap_2_320x160_height, worldmap_2_320x160_data); 

  
  // Draw Greenwich prime meridian
  M5.Lcd.drawLine((int16_t)MAP_MAXX/2, (int16_t)MAP_YOFFSET, (int16_t)MAP_MAXX/2, (int16_t)MAP_MAXY+MAP_YOFFSET, TFT_BLUE);
  // Draw equator
  M5.Lcd.drawLine((int16_t)0, (int16_t)MAP_MAXY/2+MAP_YOFFSET, (int16_t)MAP_MAXX, (int16_t)MAP_MAXY/2+MAP_YOFFSET, TFT_BLUE);

  M5.Lcd.setTextSize(1); M5.Lcd.setTextFont(1); M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.drawString("Greenwich prime meridian", 85, 30);
  M5.Lcd.drawString("Equator", 0, 110);
  
  Serial.printf("\r\nAnalemma prediction\r\n\r\n");

  M5.Lcd.setTextSize(1); M5.Lcd.setTextFont(1); M5.Lcd.setTextColor(TFT_WHITE);
  sprintf(tmpstring, "Analemma prediction for year %d at %02d:%02d:%02d o'clock", iYear, iHour, iMinute, iSecond);
  M5.Lcd.drawString(tmpstring, 0, 5);
  
  for ( u8month = 1; u8month < 13; u8month++ )
  {
  
    for ( u8day = 1; u8day <= dayspermonth[u8month]; u8day++ )
    {
      // Set time
      MyTime.settime(iYear, (int)u8month, (int)u8day, iHour, iMinute, iSecond);
      
      // Predict sun
      Sun.predict(MyTime);                // Predict sun for specific time
      Sun.latlon(dSunLAT, dSunLON);       // Get the rectangular coordinates
    
      latlon2xy(ixSUN, iySUN, dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY);

      Serial.printf("%4d-%02d-%02d %02d:%02d:%02d -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d)\r\n",iYear, u8month, u8day, iHour, iMinute, iSecond, dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY, ixSUN, iySUN);
   
      if ( u8month == 12 && u8day == 31 )
      {
        // Draw the sun icon for the last point...
		M5.Lcd.pushImage(ixSUN-8, (iySUN-8)+MAP_YOFFSET, Sun_15x15_width, Sun_15x15_height, Sun_15x15_data, Sun_15x15_transparent);
      }
      else
      {
        // ...and pixels for any other
        M5.Lcd.drawPixel(ixSUN, iySUN+MAP_YOFFSET, TFT_RED);
      }

    }
  }

  Serial.printf("\r\nFinished\n\r");
  
}


void loop()
{
  // put your main code here, to run repeatedly:

}
