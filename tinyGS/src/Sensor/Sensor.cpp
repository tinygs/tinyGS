/*
  Sensor.cpp - Class to handle DHT11 sensor for satellite telemetry

  Copyright (C) 2024 TinyGS contributors

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

#include "Sensor.h"
#include "../Logger/Logger.h"
#include "../ConfigManager/ConfigManager.h"

void Sensor::init(uint8_t pin, uint8_t type)
{
  if (dht) {
    delete dht;
  }

  dht = new DHT(pin, type);
  dht->begin();
  initialized = true;
  lastTxTime = 0;

  Log::console(PSTR("[Sensor] DHT%d initialized on pin %d"), type == DHT11 ? 11 : 22, pin);
}

SensorData Sensor::read()
{
  SensorData data = {0, 0, false};

  if (!initialized || !dht) {
    return data;
  }

  float h = dht->readHumidity();
  float t = dht->readTemperature();

  if (isnan(h) || isnan(t)) {
    Log::debug(PSTR("[Sensor] Failed to read from DHT sensor"));
    return data;
  }

  data.temperature = t;
  data.humidity = h;
  data.valid = true;
  lastReading = data;

  return data;
}

bool Sensor::shouldTransmit()
{
  if (!enabled || !initialized) {
    return false;
  }

  unsigned long now = millis();

  if (lastTxTime == 0 || (now - lastTxTime) >= txInterval) {
    return true;
  }

  return false;
}

void Sensor::buildPacket(char* buffer, size_t bufferSize, const char* stationName)
{
  SensorData data = read();

  if (!data.valid) {
    buffer[0] = '\0';
    return;
  }

  snprintf(buffer, bufferSize, "TINYGS:%s:T:%.1f:H:%.1f",
           stationName,
           data.temperature,
           data.humidity);
}

void Sensor::markTransmitted()
{
  lastTxTime = millis();
}
