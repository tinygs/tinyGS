/*
  Sensor.h - Class to handle DHT11/DHT22 sensor for satellite telemetry

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

#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <DHT.h>

struct SensorData {
  float temperature;
  float humidity;
  bool valid;
};

class Sensor {
public:
  static Sensor& getInstance()
  {
    static Sensor instance;
    return instance;
  }

  void init(uint8_t pin, uint8_t type = DHT11);
  // Stop using the sensor and release its pin
  void end();
  SensorData read();
  // Periodically refreshes the cached reading so the dashboard has fresh data
  void update();
  bool isEnabled() { return enabled; }
  bool isInitialized() { return initialized; }
  void setEnabled(bool state) { enabled = state; }
  void setTxInterval(uint16_t intervalSeconds) { txInterval = intervalSeconds * 1000UL; }
  unsigned long getTxInterval() { return txInterval; }

  bool shouldTransmit();
  void buildPacket(char* buffer, size_t bufferSize, const char* stationName);
  void markTransmitted(const char* packet);
  void markTxFailed(int16_t error);
  // Wait a full interval before the next attempt (e.g. after a read failure)
  void deferTx() { lastTxMillis = millis(); }

  // Dashboard status
  uint8_t getPin() { return pin; }
  uint8_t getType() { return type; }
  const SensorData& getLastReading() { return lastReading; }
  const char* getLastReadTime() { return lastReadTime; }
  uint32_t getReadErrors() { return readErrors; }
  uint32_t getTxCount() { return txCount; }
  uint32_t getTxErrors() { return txErrors; }
  int16_t getLastTxError() { return lastTxError; }
  const char* getLastTxTime() { return lastTxTime; }
  const char* getLastPacket() { return lastPacket; }
  // Seconds until the next TX is due (0 if due now)
  unsigned long getSecondsToNextTx();

private:
  Sensor() {}
  ~Sensor() { if (dht) delete dht; }
  static void formatNow(char* buffer, size_t size);

  static constexpr unsigned long READ_INTERVAL_MS = 5000;

  DHT* dht = nullptr;
  bool enabled = false;
  bool initialized = false;
  uint8_t pin = 0;
  uint8_t type = DHT11;
  unsigned long lastTxMillis = 0;
  unsigned long lastReadMillis = 0;
  unsigned long txInterval = 10000;
  SensorData lastReading = {0, 0, false};
  char lastReadTime[10] = "-";
  uint32_t readErrors = 0;
  uint32_t txCount = 0;
  uint32_t txErrors = 0;
  int16_t lastTxError = 0;
  char lastTxTime[10] = "-";
  char lastPacket[64] = "-";
};

#endif
