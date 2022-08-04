#include "Battery.h"
#include "../ConfigManager/ConfigManager.h"
#include "../Logger/Logger.h"

void initBatteryMonitoring(void)
{
    board_t board;
    if (ConfigManager::getInstance().getBoardConfig(board)) {
        Log::console (PSTR ("Setting up battery monitoring pin"));
        if (board.ADC_CTL != UNUSED)
            pinMode(board.ADC_CTL, OUTPUT); // Enable pin for ADC reading
        if (board.VBAT_AIN != UNUSED) {
            pinMode(board.VBAT_AIN, INPUT); // VBAT monitoring pin
            adcAttachPin(board.VBAT_AIN);
            analogReadResolution(12);
            analogSetAttenuation(ADC_11db);
        }
    }
}

void checkBattery(void)
{
  static unsigned long lastReadTime = 0;
  board_t board;

  // Throttle the battery check interval
  if (millis() - lastReadTime > BATTERY_CHECK_INTERVAL) {
    lastReadTime = millis();
    if (ConfigManager::getInstance().getBoardConfig(board)) {
      if (board.VBAT_AIN != UNUSED && board.VBAT_SCALE != UNUSED) {
        digitalWrite(board.ADC_CTL, HIGH); // Enable ADC reading
        delay(100); // Allow voltage to stabilize
        uint16_t temp = analogRead(board.VBAT_AIN);
        digitalWrite(board.ADC_CTL, LOW); // Disable ADC reading to save power
        if (status.vbat == 0.0) {
            status.vbat = board.VBAT_SCALE * temp;
        } else {
            status.vbat = (status.vbat + (board.VBAT_SCALE * temp))/2;
        }      
      }
    }
  }
}