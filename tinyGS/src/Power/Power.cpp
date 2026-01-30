/*
  Power.cpp - Class responsible of controlling the display
  
  Copyright (C) 2020 -2024  Megazaic39 [E16] , @G4lile0, @gmag12 and @dev_4m1g0

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


#include "Power.h"
#include "../Logger/Logger.h"


byte AXPchip = 0;
byte pmustat1;
byte pmustat2;
byte pwronsta;
byte pwrofsta;
byte irqstat0;
byte irqstat1;
byte irqstat2;

#define LEGACY_BATT_PIN 36

Power::Power() : pmuWire(&Wire) {}

void Power::I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  pmuWire->beginTransmission(Address);
  pmuWire->write(Register);
  pmuWire->write(Data);
  pmuWire->endTransmission();
}

uint8_t Power::I2CreadByte(uint8_t Address, uint8_t Register)
{
  uint8_t Nbytes = 1;
  pmuWire->beginTransmission(Address);
  pmuWire->write(Register);
  pmuWire->endTransmission();
  pmuWire->requestFrom(Address, Nbytes);
  byte slaveByte = pmuWire->read();
  pmuWire->endTransmission();
  return slaveByte;
}

void Power::I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  pmuWire->beginTransmission(Address);
  pmuWire->write(Register);
  pmuWire->endTransmission();
  pmuWire->requestFrom(Address, Nbytes);
  uint8_t index = 0;
  while (pmuWire->available())
    Data[index++] = pmuWire->read();
}


void Power::checkAXP() 
{ 
   board_t board;
   uint8_t boardIdx = ConfigManager::getInstance().getBoard();
   if (!ConfigManager::getInstance().getBoardConfig(board))
    return;
  Log::console(PSTR("AXPxxx chip?"));   
  byte regV = 0;
  
  if (boardIdx == LILYGO_TBEAM_SUPREME || boardIdx == TTGO_TBEAM_SX1262) {
      Log::console(PSTR("PMU: Using Supreme Wire1 on pins %d/%d"), SUPREME_PMU_SDA, SUPREME_PMU_SCL);
      Wire1.begin(SUPREME_PMU_SDA, SUPREME_PMU_SCL);
      pmuWire = &Wire1;
  } else {
      Wire.begin(board.OLED__SDA, board.OLED__SCL);                     // I2C_SDA, I2C_SCL on all new boards
      pmuWire = &Wire;
  }
  
  byte ChipID = I2CreadByte(0x34, 0x03);                            // read byte from xxx_IC_TYPE register
  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  if (ChipID == XPOWERS_AXP192_CHIP_ID) { // 0x03
    AXPchip = 1;
    Log::console(PSTR("AXP192 found"));   // T-Beam V1.1 with AXP192 power controller
    I2CwriteByte(0x34, 0x28, 0xFF);       // Set LDO2 (LoRa) & LDO3 (GPS) to 3.3V , (1.8-3.3V, 100mV/step)
    regV = I2CreadByte(0x34, 0x12);       // Power Output Control
    regV = regV | 0x0C;                   // set bit 2 (LDO2) and bit 3 (LDO3)
    I2CwriteByte(0x34, 0x12, regV);       // and power channels now enabled
    I2CwriteByte(0x34, 0x84, 0b11000010); // Set ADC sample rate to 200hz, TS pin control 20uA
    I2CwriteByte(0x34, 0x82, 0xFF);       // |Set ADC to|
    I2CwriteByte(0x34, 0x83, 0x80);       // |All Enable|
    I2CwriteByte(0x34, 0x33, 0xC3);       // Bat charge voltage to 4.2, Current 360MA and 10% for stop charging
    I2CwriteByte(0x34, 0x36, 0x0C);       // 128ms power on, 4s power off, 1s Long key press
    I2CwriteByte(0x34, 0x30, 0x80);       // Disable VBUS limits
    I2CwriteByte(0x34, 0x39, 0xFC);       // Set TS protection to 3.2256v -> disable
    I2CwriteByte(0x34, 0x32, 0x46);       // CHGLED controlled by the charging function
    pmustat1 = I2CreadByte(0x34, 0x00); pmustat2 = I2CreadByte(0x34, 0x01);
    irqstat0 = I2CreadByte(0x34, 0x44); irqstat1 = I2CreadByte(0x34, 0x45); irqstat2 = I2CreadByte(0x34, 0x46);
    Log::console(PSTR("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);
    Log::console(PSTR("IRQ status 1,2,3    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
  }
  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  if (ChipID == XPOWERS_AXP2101_CHIP_ID) {// 0x4A
    AXPchip = 2;
    Log::console(PSTR("AXP2101 found"));  // T-Beam V1.2 or Supreme with AXP2101 power controller
    
    if (boardIdx == LILYGO_TBEAM_SUPREME || boardIdx == TTGO_TBEAM_SX1262) {
      // T-Beam Supreme
      Log::console(PSTR("Configuring for T-Beam Supreme"));
      I2CwriteByte(0x34, 0x92, 0x1C);       // set ALDO1 voltage to 3.3V ( Display )
      I2CwriteByte(0x34, 0x94, 0x1C);       // set ALDO3 voltage to 3.3V ( Radio )
      I2CwriteByte(0x34, 0x95, 0x1C);       // set ALDO4 voltage to 3.3V ( GPS )
      I2CwriteByte(0x34, 0x6A, 0x04);       // set Button battery voltage to 3.0V ( backup battery )
      I2CwriteByte(0x34, 0x64, 0x03);       // set Main battery voltage to 4.2V ( 18650 battery )
      I2CwriteByte(0x34, 0x61, 0x05);       // set Main battery precharge current to 125mA
      I2CwriteByte(0x34, 0x62, 0x0A);       // set Main battery charger current to 400mA
      I2CwriteByte(0x34, 0x63, 0x15);       // set Main battery term charge current to 125mA
      
      regV = I2CreadByte(0x34, AXP2101_LDO_ONOFF_CTRL0);
      regV = regV | (1 << AXP2101_ALDO1_BIT) | (1 << AXP2101_ALDO3_BIT) | (1 << AXP2101_ALDO4_BIT);
      I2CwriteByte(0x34, AXP2101_LDO_ONOFF_CTRL0, regV);       // and power channels now enabled
    } else {
      // T-Beam V1.2 (SDA likely 21)
      I2CwriteByte(0x34, 0x93, 0x1C);       // set ALDO2 voltage to 3.3V ( LoRa VCC )
      I2CwriteByte(0x34, 0x94, 0x1C);       // set ALDO3 voltage to 3.3V ( GPS VDD )
      I2CwriteByte(0x34, 0x6A, 0x04);       // set Button battery voltage to 3.0V ( backup battery )
      I2CwriteByte(0x34, 0x64, 0x03);       // set Main battery voltage to 4.2V ( 18650 battery )
      I2CwriteByte(0x34, 0x61, 0x05);       // set Main battery precharge current to 125mA
      I2CwriteByte(0x34, 0x62, 0x0A);       // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
      I2CwriteByte(0x34, 0x63, 0x15);       // set Main battery term charge current to 125mA
      regV = I2CreadByte(0x34, AXP2101_LDO_ONOFF_CTRL0);
      regV = regV | (1 << AXP2101_ALDO2_BIT) | (1 << AXP2101_ALDO3_BIT);
      I2CwriteByte(0x34, AXP2101_LDO_ONOFF_CTRL0, regV);       // and power channels now enabled
    }

    regV = I2CreadByte(0x34, 0x18);       // XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL
    regV = regV | 0x06;                   // set bit 1 (Main Battery) and bit 2 (Button battery)
    I2CwriteByte(0x34, 0x18, regV);       // and chargers now enabled
    I2CwriteByte(0x34, 0x14, 0x30);       // set minimum system voltage to 4.4V (default 4.7V), for poor USB cables
    I2CwriteByte(0x34, 0x15, 0x05);       // set input voltage limit to 4.28v, for poor USB cables
    I2CwriteByte(0x34, 0x24, 0x06);       // set Vsys for PWROFF threshold to 3.2V (default - 2.6V and kill battery)
    I2CwriteByte(0x34, 0x50, 0x14);       // set TS pin to EXTERNAL input (not temperature)
    I2CwriteByte(0x34, 0x69, 0x01);       // set CHGLED for 'type A' and enable pin function
    I2CwriteByte(0x34, 0x27, 0x00);       // set IRQLevel/OFFLevel/ONLevel to minimum (1S/4S/128mS)
    I2CwriteByte(0x34, 0x30, 0xFF);       // enable ADC for SYS, VBUS, TS and Battery
    pmustat1 = I2CreadByte(0x34, 0x00); pmustat2 = I2CreadByte(0x34, 0x01);
    pwronsta = I2CreadByte(0x34, 0x20); pwrofsta = I2CreadByte(0x34, 0x21);
    irqstat0 = I2CreadByte(0x34, 0x48); irqstat1 = I2CreadByte(0x34, 0x49); irqstat2 = I2CreadByte(0x34, 0x4A);
    Log::console(PSTR("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);
    Log::console(PSTR("PWRON,PWROFF status : %02X,%02X"), pwronsta, pwrofsta);
    Log::console(PSTR("IRQ status 0,1,2    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
  }
  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Wire.end();
}

float Power::getBatteryVoltage() {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board)) return 0;
    
    float voltage = 0;
    
    if (AXPchip == 1) { // AXP192
        uint8_t buf[2];
        I2Cread(AXP192_SLAVE_ADDRESS, 0x78, 2, buf);
        voltage = ((buf[0] << 4) | (buf[1] & 0x0F)) * 1.1;
    } else if (AXPchip == 2) { // AXP2101
        uint8_t buf[2] = {0, 0};
        I2Cread(AXP2101_SLAVE_ADDRESS, AXP2101_BATTERY_VOLT_H, sizeof(buf), buf);
        // AXP2101: 1mV/bit
        voltage = (buf[0] << AXP2101_BATT_VOLT_SHIFT) | (buf[1] & AXP2101_BATT_VOLT_MASK); 
    } else {
        // Fallback for boards without PMU (Heltec V1/V2 etc uses GPIO 36)
        int length = 21;
        int voltages[22];
        
        for (int i = 0; i < 21; i++) {
            voltages[i] = analogRead(LEGACY_BATT_PIN); 
        }
        
        // BubbleSortAsc
        int i, j, flag = 1;
        int temp;
        for (i = 1; (i <= length) && flag; i++) {
            flag = 0;
            for (j = 0; j < (length - 1); j++) {
                if (voltages[j + 1] < voltages[j]) {
                    temp = voltages[j];
                    voltages[j] = voltages[j + 1];
                    voltages[j + 1] = temp;
                    flag = 1;
                }
            }
        }
        voltage = (float)voltages[10];
    }
    return voltage;
}

float Power::getVbusVoltage() {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board)) return 0;
    
    float voltage = 0;
    if (AXPchip == 1) { // AXP192
        uint8_t buf[2] = {0, 0};
        I2Cread(AXP192_SLAVE_ADDRESS, 0x5A, 2, buf); // 0x5A: VBUS voltage [11:4], 0x5B: [3:0]
        voltage = ((buf[0] << 4) | (buf[1] & 0x0F)) * 1.7;
    } else if (AXPchip == 2) { // AXP2101
        uint8_t buf[2] = {0, 0};
        I2Cread(AXP2101_SLAVE_ADDRESS, AXP2101_VBUS_VOLT_H, 2, buf);
        // AXP2101 VBUS: 1mV/bit
        voltage = (buf[0] << AXP2101_VBUS_VOLT_SHIFT) | (buf[1] & AXP2101_VBUS_VOLT_MASK);
    }
    return voltage;
}

int Power::getBatteryPercentage() {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board)) return 0;
    
    int pct = 0;
    
    // Safety check: if voltage is 0, percentage must be 0 (ignore fuel gauge or recalc)
    float v = getBatteryVoltage();
    if (v < 100) return 0; // Voltage is in mV, so < 0.1V is effectively 0

    if (AXPchip == 2) { // AXP2101 has fuel gauge
        pct = I2CreadByte(AXP2101_SLAVE_ADDRESS, AXP2101_FUEL_GAUGE);
        if (pct > 100) pct = 0; // Invalid
    } else {
        // Simple voltage based approx for others
        if (v > 4100) pct = 100;
        else if (v < 3300) pct = 0;
        else pct = (v - 3300) / (4100 - 3300) * 100;
    }
    return pct;
}

void Power::setGnssPower(bool on) {
    board_t board;
    if (!ConfigManager::getInstance().getBoardConfig(board))
        return;

    if (AXPchip == 1) { // AXP192
        uint8_t reg = I2CreadByte(AXP192_SLAVE_ADDRESS, 0x12);
        if (on) reg |= (1 << 3); else reg &= ~(1 << 3); // LDO3
        I2CwriteByte(AXP192_SLAVE_ADDRESS, 0x12, reg);
    } else if (AXPchip == 2) { // AXP2101
        uint8_t reg = I2CreadByte(AXP2101_SLAVE_ADDRESS, AXP2101_LDO_ONOFF_CTRL0);
        uint8_t boardIdx = ConfigManager::getInstance().getBoard();
        
        if (boardIdx == LILYGO_TBEAM_SUPREME || boardIdx == TTGO_TBEAM_SX1262) { // Supreme
             if (on) reg |= (1 << AXP2101_ALDO4_BIT); 
             else reg &= ~(1 << AXP2101_ALDO4_BIT);
        } else {
             if (on) reg |= (1 << AXP2101_ALDO3_BIT); else reg &= ~(1 << AXP2101_ALDO3_BIT);
        }
        I2CwriteByte(AXP2101_SLAVE_ADDRESS, AXP2101_LDO_ONOFF_CTRL0, reg);
    }
}