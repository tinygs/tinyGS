/*
  Sensor.h - Class to handle DHT11 sensor for satellite telemetry

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

#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <DHT.h>
#include "../Status.h"

extern Status status;

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
  SensorData read();
  bool isEnabled() { return enabled; }
  void setEnabled(bool state) { enabled = state; }
  void setTxInterval(uint16_t intervalSeconds) { txInterval = intervalSeconds * 1000UL; }

  bool shouldTransmit();
  void buildPacket(char* buffer, size_t bufferSize, const char* stationName);
  void markTransmitted();

private:
  Sensor() : dht(nullptr), enabled(false), lastTxTime(0), txInterval(10000) {}
  ~Sensor() { if (dht) delete dht; }

  DHT* dht;
  bool enabled;
  bool initialized = false;
  unsigned long lastTxTime;
  unsigned long txInterval;
  SensorData lastReading;
};

#endif
