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

#define XPOWERS_CHIP_AXP202

#include <XPowersLib.h>

byte AXPchip = 0;
byte pmustat1;
byte pmustat2;
byte pwronsta;
byte pwrofsta;
byte irqstat0;
byte irqstat1;
byte irqstat2;


void Power::I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.write(Data);
  Wire.endTransmission();
}

uint8_t Power::I2CreadByte(uint8_t Address, uint8_t Register)
{
  uint8_t Nbytes = 1;
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();
  Wire.requestFrom(Address, Nbytes);
  byte slaveByte = Wire.read();
  Wire.endTransmission();
  return slaveByte;
}

void Power::I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();
  Wire.requestFrom(Address, Nbytes);
  uint8_t index = 0;
  while (Wire.available())
    Data[index++] = Wire.read();
}


void Power::checkAXP() 
{
    bool  pmu_flag = 0;
    XPowersPMU PMU;
    uint8_t i2c_sda = -1;
    uint8_t i2c_scl = -1;
    // uint8_t pmu_irq_pin = -1;
    
    board_t board;
   if (!ConfigManager::getInstance().getBoardConfig(board))
    return;
   Log::console (PSTR ("AXPxxx chip?"));
   i2c_sda = board.OLED__SDA;
   i2c_scl = board.OLED__SCL;
 //   byte regV = 0;
//   Wire.begin(board.OLED__SDA, board.OLED__SCL);                     // I2C_SDA, I2C_SCL on all new boards
//   byte ChipID = I2CreadByte(0x34, 0x03);                            // read byte from xxx_IC_TYPE register
  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   if (PMU.getChipID() == XPOWERS_AXP192) {
    AXPchip = 1;
    Log::console (PSTR ("AXP192 found"));   // T-Beam V1.1 with AXP192 power controller
    bool result = PMU.begin (Wire, AXP192_SLAVE_ADDRESS, i2c_sda, i2c_scl);
    if (result == false) {
        Log::console (PSTR ("power is not online..."));
        return;
    }
    //I2CwriteByte (0x34, 0x28, 0xFF);       // Set LDO2 (LoRa) & LDO3 (GPS) to 3.3V , (1.8-3.3V, 100mV/step)
    PMU.setLDO2Voltage(3300);
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
    Log::console(PSTR("AXP2101 found"));  // T-Beam V1.2 with AXP2101 power controller
    I2CwriteByte(0x34, 0x93, 0x1C);       // set ALDO2 voltage to 3.3V ( LoRa VCC )
    I2CwriteByte(0x34, 0x94, 0x1C);       // set ALDO3 voltage to 3.3V ( GPS VDD )
    I2CwriteByte(0x34, 0x6A, 0x04);       // set Button battery voltage to 3.0V ( backup battery )
    I2CwriteByte(0x34, 0x64, 0x03);       // set Main battery voltage to 4.2V ( 18650 battery )
    I2CwriteByte(0x34, 0x61, 0x05);       // set Main battery precharge current to 125mA
    I2CwriteByte(0x34, 0x62, 0x0A);       // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
    I2CwriteByte(0x34, 0x63, 0x15);       // set Main battery term charge current to 125mA
    regV = I2CreadByte(0x34, 0x90);       // XPOWERS_AXP2101_LDO_ONOFF_CTRL0
    regV = regV | 0x06;                   // set bit 1 (ALDO2) and bit 2 (ALDO3)
    I2CwriteByte(0x34, 0x90, regV);       // and power channels now enabled
    regV = I2CreadByte(0x34, 0x18);       // XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL
    regV = regV | 0x06;                   // set bit 1 (Main Battery) and bit 2 (Button battery)
    I2CwriteByte(0x34, 0x18, regV);       // and chargers now enabled
    I2CwriteByte(0x34, 0x14, 0x30);       // set minimum system voltage to 4.4V (default 4.7V), for poor USB cables
    I2CwriteByte(0x34, 0x15, 0x05);       // set input voltage limit to 4.28v, for poor USB cables
    I2CwriteByte(0x34, 0x24, 0x06);       // set Vsys for PWROFF threshold to 3.2V (default - 2.6V and kill battery)
    I2CwriteByte(0x34, 0x50, 0x14);       // set TS pin to EXTERNAL input (not temperature)
    I2CwriteByte(0x34, 0x69, 0x01);       // set CHGLED for 'type A' and enable pin function
    I2CwriteByte(0x34, 0x27, 0x00);       // set IRQLevel/OFFLevel/ONLevel to minimum (1S/4S/128mS)
    I2CwriteByte(0x34, 0x30, 0x0F);       // enable ADC for SYS, VBUS, TS and Battery
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