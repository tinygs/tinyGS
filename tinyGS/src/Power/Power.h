/*
  Power.h - Class responsible of controlling the display
  
  Copyright (C) 2020 -2024 Megazaic39 [E16] , @G4lile0, @gmag12 and @dev_4m1g0

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


#ifndef POWER_H
#define POWER_H
#include "Arduino.h"
#include <Wire.h>
#include "../Status.h"
#include "../Network/ConfigStore.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * * * * A X P   C H I P   C O N F I G * * * * * * * * * *
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#define AXP192_SLAVE_ADDRESS    (0x34) // I2C slaveaddress
#define XPOWERS_AXP192_IC_TYPE  (0x03) // register address
#define XPOWERS_AXP192_CHIP_ID  (0x03) // content

#define AXP2101_SLAVE_ADDRESS   (0x34)
#define XPOWERS_AXP2101_IC_TYPE (0x03)
#define XPOWERS_AXP2101_CHIP_ID (0x4A)

#define AXP216_SLAVE_ADDRESS    (0x34)
#define XPOWERS_AXP216_IC_TYPE  (0x03)
#define XPOWERS_AXP216_CHIP_ID  (0x41)

#define AXP202_SLAVE_ADDRESS    (0x35)
#define XPOWERS_AXP202_IC_TYPE  (0x03)
#define XPOWERS_AXP202_CHIP_ID  (0x41)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * * * * A X P   C H I P   R E A D I N G * * * * * * * * *
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#define XPOWERS_AXP2101_ADC_DATA_RELUST0                 (0x34)
#define XPOWERS_AXP2101_ADC_DATA_RELUST1                 (0x35)
#define XPOWERS_AXP2101_ADC_DATA_RELUST2                 (0x36)
#define XPOWERS_AXP2101_ADC_DATA_RELUST3                 (0x37)
#define XPOWERS_AXP2101_ADC_DATA_RELUST4                 (0x38)
#define XPOWERS_AXP2101_ADC_DATA_RELUST5                 (0x39)
#define XPOWERS_AXP2101_ADC_DATA_RELUST6                 (0x3A)
#define XPOWERS_AXP2101_ADC_DATA_RELUST7                 (0x3B)
#define XPOWERS_AXP2101_ADC_DATA_RELUST8                 (0x3C)
#define XPOWERS_AXP2101_ADC_DATA_RELUST9                 (0x3D)


extern Status status;

class Power {
public:
    static Power& getInstance()
    {
        static Power instance; 
        return instance;
    }
     void checkAXP(); 
private:
    void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data);
    uint8_t I2CreadByte(uint8_t Address, uint8_t Register);
    void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data);

};
#endif