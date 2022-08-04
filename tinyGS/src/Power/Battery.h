#ifndef BATTERY_H
#define BATTERY_H
#include "Arduino.h"
#include <Wire.h>
#include "../Status.h"
#include "../ConfigManager/ConfigManager.h"

#define BATTERY_CHECK_INTERVAL 10000

extern Status status;

void initBatteryMonitoring(void);
void checkBattery(void);

#endif