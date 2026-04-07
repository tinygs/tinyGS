/*
  OTA.cpp - On The Air Update Class
  
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






#include "./OTA.h"

#if !defined(CONFIG_IDF_TARGET_ESP32)
  #include <Arduino.h>
  #include <WiFiClientSecure.h>
#endif


#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"
#include "../Logger/Logger.h"

extern Status status;
bool usingNewCert = true;
const long MIN_TIME_BEFORE_UPDATE = random(60000, 20*60*1000);

void OTA::update()
{
#ifdef SECURE_OTA
  WiFiClientSecure client;
//  if (usingNewCert)
    client.setCACert(newRoot_CA);
//  else
//    client.setCACert(DSTroot_CA);
#else
  WiFiClient client;
#endif

  uint64_t chipId = ESP.getEfuseMac();
  char clientId[13];
  sprintf(clientId, "%04X%08X",(uint16_t)(chipId>>32), (uint32_t)chipId);

  ConfigManager& c = ConfigManager::getInstance();
  char url[255];
  sprintf_P(url, PSTR("%s?user=%s&name=%s&mac=%s&version=%d&rescue=%s"), OTA_URL, c.getMqttUser(), c.getThingName(), clientId, status.version, (c.isFailSafeActive()?"true":"false"));

  Log::debug(PSTR("Checking for firmware Updates...  "));
  t_httpUpdate_return ret = httpUpdate.update(client, url, status.git_version);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      usingNewCert = !usingNewCert;
      Log::info(PSTR("Update failed Error (%d): %s\n"), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES: // server 304
      Log::info(PSTR("No updates required"));
      break;

    case HTTP_UPDATE_OK:
      Log::info(PSTR("Update ok but ESP has not restarted!!! (This should never be printed)"));
      break;
  }
}

unsigned static long lastUpdateTime = 0;
void OTA::loop()
{
  if (millis() < MIN_TIME_BEFORE_UPDATE)
    return;

  if (millis() - lastUpdateTime > TIME_BETTWEN_UPDATE_CHECK)
  {
    lastUpdateTime = millis();
    update();
  }
}
