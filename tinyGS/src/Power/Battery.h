#ifndef BATTERY_H
#define BATTERY_H
#include "Arduino.h"
#include <Wire.h>
#include "../Status.h"
#include "../ConfigManager/ConfigManager.h"

// Throttle reads of battery voltage (in ms)
#define BATTERY_CHECK_INTERVAL 10000

// ADC control pin polarity differs between some boards (ESP32-S3 vs
// others). `ADC_READ_ENABLE` drives the board-specific pin value that
// enables the voltage divider or ADC front-end; `ADC_READ_DISABLE`
// disables it. These are used by enableADCReading()/disableADCReading().
#if CONFIG_IDF_TARGET_ESP32S3
   #define ADC_READ_ENABLE HIGH
   #define ADC_READ_DISABLE LOW
#else
   #define ADC_READ_ENABLE LOW
   #define ADC_READ_DISABLE HIGH
#endif

// Global device status structure defined elsewhere; `status.vbat` is
// used/updated by the battery monitoring code in Battery.cpp.
extern Status status;

void initBatteryMonitoring(void);
void checkBattery(void);
float getBatteryPercentage(void);

#endif