// Battery.cpp
// Battery monitoring helpers for tinyGS

#include <stdlib.h>
#include "Battery.h"
#include "../ConfigManager/ConfigManager.h"
#include "../Logger/Logger.h"
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

adc_cali_handle_t cali_handle = NULL;
#define DEFAULT_VREF 1100


// Arduino Framework Battery Writeup
// https://digitalconcepts.net.au/arduino/index.php?op=Battery

// Initialize ADC pins and ADC configuration for the board's VBAT input.
// If the board provides an `ADC_CTL` pin (to enable/disable the voltage
// divider), that pin is configured as an output. If the `VBAT_AIN` analog
// input is available, attach it to the ADC and set resolution/attenuation
// appropriate for battery voltage measurement.
void initBatteryMonitoring(void)
{
    board_t board;
    if (ConfigManager::getInstance().getBoardConfig(board)) {
        // If ADC_CTL is defined, set it up as the enable pin for ADC reading
        if (board.ADC_CTL != UNUSED) {
            pinMode(board.ADC_CTL, OUTPUT); // control pin for ADC/voltage divider
        }
        if (board.VBAT_AIN != UNUSED) {
            // Configure the analog input used for VBAT monitoring
            pinMode(board.VBAT_AIN, INPUT);
            // Use 12-bit resolution and 11dB attenuation for ~0-3.6V range
            analogReadResolution(12);
            analogSetAttenuation(ADC_11db);
            // Characterize ADC for voltage conversion
            adc_cali_curve_fitting_config_t cali_config = {
                .unit_id    = ADC_UNIT_1,
                .atten      = ADC_ATTEN_DB_12,
                .bitwidth   = ADC_BITWIDTH_DEFAULT,
            };

            adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
        }
        checkBattery(); // Initial read to seed status.vbat
    }
}

// Drive the ADC control pin to the enabled state (if present) and wait
// briefly for voltages to stabilize before taking a reading.
inline void enableADCReading(board_t board)
{
    if (board.ADC_CTL != UNUSED) {
        digitalWrite(board.ADC_CTL, ADC_READ_ENABLE);
        delay(50); // Allow voltage to stabilize before sampling
    }
}

// Disable the ADC control pin to remove standby current from the
// voltage divider (if the board uses one). No delay is necessary here.
inline void disableADCReading(board_t board)
{
    if (board.ADC_CTL != UNUSED) {
        digitalWrite(board.ADC_CTL, ADC_READ_DISABLE);
    }
}

// Periodically read the VBAT analog input and update the global
// `status.vbat` value. Reads are throttled by `BATTERY_CHECK_INTERVAL`.
// The raw ADC value is scaled by `board.VBAT_SCALE` (provided by the
// board configuration) to convert it into volts. 
void checkBattery(void)
{
  static unsigned long lastReadTime = 0;
  board_t board;

  // Throttle the battery check interval
  if (millis() - lastReadTime > BATTERY_CHECK_INTERVAL) {
    lastReadTime = millis();
    if (ConfigManager::getInstance().getBoardConfig(board)) {
      if (board.VBAT_AIN != UNUSED && board.VBAT_SCALE != UNUSED) {
        int voltage = 0;

        enableADCReading(board);

        // Read raw ADC
        int raw = analogRead(board.VBAT_AIN);

        disableADCReading(board);

        //Convert adc_reading to voltage in mV
        adc_cali_raw_to_voltage(cali_handle, raw, &voltage);

        // Convert and smooth: seed or simple average with previous value
        float measured = (board.VBAT_SCALE * voltage) / 1000.0f; // convert mV to V
         Log::debug(PSTR("adc raw: %d, voltage: %f V"), raw, measured);
        if (status.vbat == 0.0) {
            status.vbat = measured; // initial seed
        } else {
            status.vbat = (status.vbat + measured) / 2.0f; // simple smoothing
        }
      }
    }
  }
}

// Convert the current battery voltage status.vbat to a percentage.
// Assumes a linear discharge curve between 3.0V (0%) and 4.15V (100%).
// Returns 999.0f if the voltage is above 4.2V, indicating charging state.
float getBatteryPercentage(void)
{
    const float poweredVoltage = 4.23f; // Voltage above 100%
    const float maxVoltage = 4.15f; // Voltage at 100%
    const float minVoltage = 3.5f; // Voltage at 0%
    float voltage = status.vbat;
    if (voltage >= poweredVoltage) {
        return 999.0f; // Plugged in / charging
    } else if (voltage >= maxVoltage) {
        return 100.0f;
    } else if (voltage <= minVoltage) {
        return 0.0f;
    } else {
        return ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0f;
     }
}