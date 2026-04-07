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

#include <AioP13.h>
//#include <M5Stack.h>
#include <ESP32-Chimera-Core.h>
#include "graphics.h"

#define MAP_MAXX    320
#define MAP_MAXY    160

#define MAP_YOFFSET 20

const char *tleName = "ISS (ZARYA)";
const char *tlel1   = "1 25544U 98067A   21320.51955234  .00001288  00000+0  31985-4 0  9990";
const char *tlel2   = "2 25544  51.6447 309.4881 0004694 203.6966 299.8876 15.48582035312205";

// For testing purpose (geostationary, no motion)
//const char *tleName = "ES'HAIL 2";
//const char *tlel1   = "1 43700U 18090A   21320.51254296  .00000150  00000+0  00000+0 0  9998";
//const char *tlel2   = "2 43700   0.0138 278.3980 0002418 337.0092  10.7288  1.00272495 10898";


const char  *pcMyName = "DL9SEC";    // Observer name
double       dMyLAT   =  48.661563;  // Latitude (Breitengrad): N -> +, S -> -
double       dMyLON   =   9.779416;  // Longitude (Längengrad): E -> +, W -> -
double       dMyALT   = 386.0;       // Altitude ASL (m)

double       dfreqRX  = 145.800;     // Nominal downlink frequency
double       dfreqTX  = 437.800;     // Nominal uplink frequency

int          iYear    = 2021;        // Set start year
int          iMonth   = 11;          // Set start month
int          iDay     = 18;          // Set start day
int          iHour    = 23;          // Set start hour
int          iMinute  = 8;           // Set start minute
int          iSecond  = 2;           // Set start second

// Expecting the ISS to be at 289,61° elevation and 20,12° azimuth (Gpredict)
// Result will be 289,74° elevation and 20,44° azimuth...
// Expecting the sun to be at -60.79° elevation and 0.86° azimuth (https://www.sunearthtools.com/dp/tools/pos_sun.php)
// Result will be -60.79° elevation and 0.89° azimuth...

double       dSatLAT  = 0;           // Satellite latitude
double       dSatLON  = 0;           // Satellite longitude
double       dSatAZ   = 0;           // Satellite azimuth
double       dSatEL   = 0;           // Satellite elevation

double       dSunLAT  = 0;           // Sun latitude
double       dSunLON  = 0;           // Sun longitude
double       dSunAZ   = 0;           // Sun azimuth
double       dSunEL   = 0;           // Sun elevation

int          ixQTH    = 0;           // Map pixel coordinate x of QTH
int          iyQTH    = 0;           // Map pixel coordinate y of QTH
int          ixSAT    = 0;           // Map pixel coordinate x of satellite
int          iySAT    = 0;           // Map pixel coordinate y of satellite
int          ixSUN    = 0;           // Map pixel coordinate x of sun
int          iySUN    = 0;           // Map pixel coordinate y of sun

char         acBuffer[20];            // Buffer for ASCII time

int          aiSatFP[90][2];          // Array for storing the satellite footprint map coordinates
int          aiSunFP[180][2];         // Array for storing the sunlight footprint map coordinates

void setup()
{
  int i;
  char tmpstring[100];

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
  P13Observer MyQTH(pcMyName, dMyLAT, dMyLON, dMyALT);              // Set observer coordinates
  P13Satellite MySAT(tleName, tlel1, tlel2);                        // Create ISS data from TLE
 
  //M5.Lcd.pushImage(0, MAP_YOFFSET, worldmap_1_320x160_width, worldmap_1_320x160_height, worldmap_2_320x160_data);
  M5.Lcd.pushImage(0, MAP_YOFFSET, worldmap_2_320x160_width, worldmap_2_320x160_height, worldmap_2_320x160_data); 

  latlon2xy(ixQTH, iyQTH, dMyLAT, dMyLON, MAP_MAXX, MAP_MAXY);      // Get x/y for the pixel map 

  Serial.printf("\r\nPrediction for %s at %s (MAP %dx%d: x = %d,y = %d):\r\n\r\n", MySAT.c_ccSatName, MyQTH.c_ccObsName, MAP_MAXX, MAP_MAXY, ixQTH, iyQTH);

  M5.Lcd.setTextSize(1); M5.Lcd.setTextFont(1); M5.Lcd.setTextColor(TFT_WHITE);
  
  sprintf(tmpstring, "Prediction for %s at %s", MySAT.c_ccSatName, MyQTH.c_ccObsName);
  M5.Lcd.drawString(tmpstring, 0, 0);
  sprintf(tmpstring, "(Lat = %.4f%c, Lon = %.4f%c, Alt = %.1fm ASL):", dMyLAT, (char)39, dMyLON, (char)39, dMyALT);
  M5.Lcd.drawString(tmpstring, 0, 10);
  
  M5.Lcd.pushImage(ixQTH-5, (iyQTH-5)+MAP_YOFFSET, Groundstation_9x9_width, Groundstation_9x9_height, Groundstation_9x9_data, Groundstation_9x9_transparent);


  MyTime.ascii(acBuffer);             // Get time for prediction as ASCII string
  MySAT.predict(MyTime);              // Predict ISS for specific time
  MySAT.latlon(dSatLAT, dSatLON);     // Get the rectangular coordinates
  MySAT.elaz(MyQTH, dSatEL, dSatAZ);  // Get azimut and elevation for MyQTH

  latlon2xy(ixSAT, iySAT, dSatLAT, dSatLON, MAP_MAXX, MAP_MAXY);  // Get x/y for the pixel map

  Serial.printf("%s -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d) Az: %.2f El: %.2f\r\n\r\n", acBuffer, dSatLAT, dSatLON, MAP_MAXX, MAP_MAXY, ixSAT, iySAT, dSatAZ, dSatEL);

  M5.Lcd.pushImage(ixSAT-8, (iySAT-8)+MAP_YOFFSET, Satellite_15x15_width, Satellite_15x15_height, Satellite_15x15_data, Satellite_15x15_transparent);

  sprintf(tmpstring, "%s UTC", acBuffer);
  M5.Lcd.drawString(tmpstring, 0, 190);
  sprintf(tmpstring, "Lat: %.2f%c Lon: %.2f%c Az: %.2f%c El: %.2f%c", dSatLAT, (char)39, dSatLON, (char)39, dSatAZ, (char)39, dSatEL, (char)39);
  M5.Lcd.drawString(tmpstring, 0, 200);  
  
  Serial.printf("RX: %.6f MHz, TX: %.6f MHz\r\n\r\n", MySAT.doppler(dfreqRX, P13_FRX), MySAT.doppler(dfreqTX, P13_FTX));

  sprintf(tmpstring, "RX: %.5f MHz", MySAT.doppler(dfreqRX, P13_FRX));
  M5.Lcd.drawString(tmpstring, 0, 210);
  sprintf(tmpstring, "TX: %.5f MHz", MySAT.doppler(dfreqTX, P13_FTX));
  M5.Lcd.drawString(tmpstring, 0, 220);  

  
  // Calcualte ISS footprint
  Serial.printf("Satellite footprint map coordinates:\n\r");
  
  MySAT.footprint(aiSatFP, (sizeof(aiSatFP)/sizeof(int)/2), MAP_MAXX, MAP_MAXY, dSatLAT, dSatLON);

  // Print ISS footprint
  for (i = 0; i < (sizeof(aiSatFP)/sizeof(int)/2); i++)
  {
    Serial.printf("%2d: x = %d, y = %d\r\n", i, aiSatFP[i][0], aiSatFP[i][1]);
	M5.Lcd.drawPixel(aiSatFP[i][0], MAP_YOFFSET+aiSatFP[i][1], TFT_RED);
  }

  // Predict sun
  Sun.predict(MyTime);                // Predict ISS for specific time
  Sun.latlon(dSunLAT, dSunLON);       // Get the rectangular coordinates
  Sun.elaz(MyQTH, dSunEL, dSunAZ);    // Get azimut and elevation for MyQTH

  latlon2xy(ixSUN, iySUN, dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY);

  Serial.printf("\r\nSun -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d) Az: %.2f El: %.2f\r\n\r\n", dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY, ixSUN, iySUN, dSunAZ, dSunEL);
  M5.Lcd.pushImage(ixSUN-8, (iySUN-8)+MAP_YOFFSET, Sun_15x15_width, Sun_15x15_height, Sun_15x15_data, Sun_15x15_transparent);

  // Calcualte sunlight footprint
  Serial.printf("Sunlight footprint map coordinates:\n\r");
  
  Sun.footprint(aiSunFP, (sizeof(aiSunFP)/sizeof(int)/2), MAP_MAXX, MAP_MAXY, dSunLAT, dSunLON);

  // Print sunlight footprint
  for (i = 0; i < (sizeof(aiSunFP)/sizeof(int)/2); i++)
  {
    Serial.printf("%2d: x = %d, y = %d\r\n", i, aiSunFP[i][0], aiSunFP[i][1]);
	M5.Lcd.drawPixel(aiSunFP[i][0], MAP_YOFFSET+aiSunFP[i][1], TFT_YELLOW);
  }

  Serial.printf("\r\nFinished\n\r");
  
}


void loop()
{
  // put your main code here, to run repeatedly:

}
