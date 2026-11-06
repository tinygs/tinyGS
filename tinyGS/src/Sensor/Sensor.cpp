/*
  Sensor.cpp - Class to handle DHT11/DHT22 sensor for satellite telemetry

  Copyright (C) 2026 TinyGS contributors

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
#include <time.h>

void Sensor::end()
{
  if (dht) {
    delete dht;
    dht = nullptr;
    pinMode(pin, INPUT);
  }
  initialized = false;
  enabled = false;
  lastReading.valid = false;
}

void Sensor::init(uint8_t pin, uint8_t type)
{
  end();

  this->pin = pin;
  this->type = type;
  dht = new DHT(pin, type);
  dht->begin();
  initialized = true;
  lastTxMillis = 0;
  lastReadMillis = 0;

  Log::console(PSTR("[Sensor] DHT%d initialized on pin %d"), type == DHT11 ? 11 : 22, pin);
}

void Sensor::formatNow(char* buffer, size_t size)
{
  time_t now = time(NULL);
  if (now > 0) {
    struct tm *timeinfo = gmtime(&now);
    snprintf(buffer, size, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  }
  else {
    snprintf(buffer, size, "%lus", millis() / 1000);
  }
}

SensorData Sensor::read()
{
  SensorData data = {0, 0, false};

  if (!initialized || !dht) {
    return data;
  }

  lastReadMillis = millis();
  float h = dht->readHumidity();
  float t = dht->readTemperature();

  if (isnan(h) || isnan(t)) {
    readErrors++;
    lastReading.valid = false;
    Log::debug(PSTR("[Sensor] Failed to read from DHT sensor (errors: %u)"), readErrors);
    return data;
  }

  data.temperature = t;
  data.humidity = h;
  data.valid = true;
  lastReading = data;
  formatNow(lastReadTime, sizeof(lastReadTime));

  return data;
}

void Sensor::update()
{
  if (!initialized)
    return;

  if (lastReadMillis == 0 || millis() - lastReadMillis >= READ_INTERVAL_MS)
    read();
}

bool Sensor::shouldTransmit()
{
  if (!enabled || !initialized) {
    return false;
  }

  return lastTxMillis == 0 || (millis() - lastTxMillis) >= txInterval;
}

unsigned long Sensor::getSecondsToNextTx()
{
  if (lastTxMillis == 0)
    return 0;

  unsigned long elapsed = millis() - lastTxMillis;
  return elapsed >= txInterval ? 0 : (txInterval - elapsed + 999) / 1000;
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

void Sensor::markTransmitted(const char* packet)
{
  lastTxMillis = millis();
  txCount++;
  lastTxError = 0;
  strlcpy(lastPacket, packet, sizeof(lastPacket));
  formatNow(lastTxTime, sizeof(lastTxTime));
}

void Sensor::markTxFailed(int16_t error)
{
  // Count the attempt so a persistent failure doesn't retry on every loop
  lastTxMillis = millis();
  txErrors++;
  lastTxError = error;
}
