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

//#define XPOWERS_CHIP_AXP202

#include <XPowersLib.h>

byte AXPchip = 0;
byte pmustat1;
byte pmustat2;
byte pwronsta;
byte pwrofsta;
byte irqstat0;
byte irqstat1;
byte irqstat2;


void Power::I2CwriteByte (uint8_t Address, uint8_t Register, uint8_t Data) {
    Wire.beginTransmission (Address);
    Wire.write (Register);
    Wire.write (Data);
    Wire.endTransmission ();
}

uint8_t Power::I2CreadByte (uint8_t Address, uint8_t Register) {
    uint8_t Nbytes = 1;
    Wire.beginTransmission (Address);
    Wire.write (Register);
    Wire.endTransmission ();
    Wire.requestFrom (Address, Nbytes);
    byte slaveByte = Wire.read ();
    Wire.endTransmission ();
    return slaveByte;
}

void Power::I2Cread (uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data) {
    Wire.beginTransmission (Address);
    Wire.write (Register);
    Wire.endTransmission ();
    Wire.requestFrom (Address, Nbytes);
    uint8_t index = 0;
    while (Wire.available ())
        Data[index++] = Wire.read ();
}


void Power::checkAXP () {
    bool  pmu_flag = 0;
    XPowersLibInterface* PMU;
    uint8_t i2c_sda = -1;
    uint8_t i2c_scl = -1;
    // uint8_t pmu_irq_pin = -1;

    board_t board;
    if (!ConfigManager::getInstance ().getBoardConfig (board))
        return;
    Log::console (PSTR ("AXPxxx chip?"));
    i2c_sda = board.OLED__SDA;
    i2c_scl = board.OLED__SCL;
    Log::console (PSTR ("I2C on SDA: %d, SCL: %d"), i2c_sda, i2c_scl);
    Wire.begin (i2c_sda, i2c_scl);

    uint8_t chipID = -1;

    if (!PMU) {
        PMU = new XPowersAXP2101 (Wire);
        if (!PMU->init ()) {
            Log::console ("No AXP2101 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Log::console ("AXP2101 PMU init succeeded");
            chipID = PMU->getChipID ();
            Log::console (PSTR ("AXP chip ID: %d"), chipID);
            AXPchip = 2;
            Log::console (PSTR ("AXP2101 found"));  // T-Beam V1.2 with AXP2101 power controller

            bool result;

            // Unuse power channel
            PMU->disablePowerOutput (XPOWERS_DCDC2);
            PMU->disablePowerOutput (XPOWERS_DCDC3);
            PMU->disablePowerOutput (XPOWERS_DCDC4);
            PMU->disablePowerOutput (XPOWERS_DCDC5);
            PMU->disablePowerOutput (XPOWERS_ALDO1);
            PMU->disablePowerOutput (XPOWERS_ALDO4);
            PMU->disablePowerOutput (XPOWERS_BLDO1);
            PMU->disablePowerOutput (XPOWERS_BLDO2);
            PMU->disablePowerOutput (XPOWERS_DLDO1);
            PMU->disablePowerOutput (XPOWERS_DLDO2);

            // GNSS RTC PowerVDD 3300mV
            result = PMU->setPowerChannelVoltage (XPOWERS_VBACKUP, 3000);
            Log::console (PSTR ("Set VBACKUP 3.0V : %s"), result ? "OK" : "Failed");
            result = PMU->enablePowerOutput (XPOWERS_VBACKUP);
            Log::console (PSTR ("Enable VBACKUP : %s"), result ? "OK" : "Failed");

            // ESP32 VDD 3300mV
            //  ! No need to set, automatically open , Don't close it
            //PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
            //PMU->setProtectedChannel(XPOWERS_DCDC1);
            uint16_t dcdc1_voltage = PMU->getPowerChannelVoltage (XPOWERS_DCDC1);
            Log::console (PSTR ("DCDC1 voltage : %d mV"), dcdc1_voltage);

            // Enable LCD voltage
            result = PMU->setPowerChannelVoltage (XPOWERS_DCDC3, 3300);
            Log::console (PSTR ("Set DCDC3 3.3V : %s"), result ? "OK" : "Failed");
            result = PMU->enablePowerOutput (XPOWERS_DCDC3);
            Log::console (PSTR ("Enable DCDC3 : %s"), result ? "OK" : "Failed");

            // LoRa VDD 3300mV
            result = PMU->setPowerChannelVoltage (XPOWERS_ALDO2, 3300);
            Log::console (PSTR ("Set ALDO2 3.3V : %s"), result ? "OK" : "Failed");
            result = PMU->enablePowerOutput (XPOWERS_ALDO2);
            Log::console (PSTR ("Enable ALDO2 : %s"), result ? "OK" : "Failed");

            // GNSS VDD 3300mV but disable it
            result = PMU->setPowerChannelVoltage (XPOWERS_ALDO3, 3300);
            Log::console (PSTR ("Set ALDO3 3.3V : %s"), result ? "OK" : "Failed");
            result = PMU->disablePowerOutput (XPOWERS_ALDO3);
            Log::console (PSTR ("Disable ALDO3 : %s"), result ? "OK" : "Failed");
            // Set constant current charging current
            result = PMU->setChargerConstantCurr (XPOWERS_AXP192_CHG_CUR_450MA);
            Log::console (PSTR ("Set charger current 450mA : %s"), result ? "OK" : "Failed");

            // Set up the charging voltage
            result = PMU->setChargeTargetVoltage (XPOWERS_AXP192_CHG_VOL_4V2);
            Log::console (PSTR ("Set charging voltage 4.2V : %s"), result ? "OK" : "Failed");
            PMU->setChargingLedMode (XPOWERS_CHG_LED_BLINK_1HZ);

            result = PMU->setSysPowerDownVoltage (3200);
            Log::console (PSTR ("Set system power down voltage 3.2V : %s"), result ? "OK" : "Failed");

            uint16_t battV = PMU->getBattVoltage ();
            uint16_t sysV = PMU->getSystemVoltage ();

            Log::console (PSTR ("PMU battV,sysV : %d, %d"), battV, sysV);

            uint64_t irqstatus = PMU->getIrqStatus ();

            irqstat0 = irqstatus & 0xFF;
            irqstat1 = (irqstatus >> 8) & 0xFF;
            irqstat2 = (irqstatus >> 16) & 0xFF;
            Log::console (PSTR ("IRQ status 0,1,2    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
        }
    }

    if (!PMU) {
        PMU = new XPowersAXP192 (Wire);
        if (!PMU->init ()) {
            Log::console ("No AXP192 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Log::console ("AXP192 PMU init succeeded");
            chipID = PMU->getChipID ();
            Log::console (PSTR ("AXP chip ID: %d"), chipID);

            AXPchip = 1;
            Log::console (PSTR ("AXP192 found"));   // T-Beam V1.1 with AXP192 power controller

            // lora radio power channel
            PMU->setPowerChannelVoltage (XPOWERS_LDO2, 3300);
            PMU->enablePowerOutput (XPOWERS_LDO2);

            // oled module power channel,
               // disable it will cause abnormal communication between boot and AXP power supply,
               // do not turn it off
            PMU->setPowerChannelVoltage (XPOWERS_DCDC1, 3300);
            // enable oled power
            PMU->enablePowerOutput (XPOWERS_DCDC1);

            // gnss module power channel -  turned off
            PMU->setPowerChannelVoltage (XPOWERS_LDO3, 3300);
            PMU->disablePowerOutput (XPOWERS_LDO3);

            // protected oled power source
            PMU->setProtectedChannel (XPOWERS_DCDC1);
            // protected esp32 power source
            PMU->setProtectedChannel (XPOWERS_DCDC3);

            // disable not use channel
            PMU->disablePowerOutput (XPOWERS_DCDC2);

            // disable all axp chip interrupt
            PMU->disableIRQ (XPOWERS_AXP192_ALL_IRQ);

            // Set constant current charging current
            PMU->setChargerConstantCurr (XPOWERS_AXP192_CHG_CUR_450MA);

            // Set up the charging voltage
            PMU->setChargeTargetVoltage (XPOWERS_AXP192_CHG_VOL_4V2);
            PMU->setSysPowerDownVoltage (3200);

            uint16_t battV = PMU->getBattVoltage ();
            uint16_t sysV = PMU->getSystemVoltage ();

            Log::console (PSTR ("PMU battV,sysV : %02X,%02X"), battV, sysV);

            //Log::console (PSTR ("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);

            uint64_t irqstatus = PMU->getIrqStatus ();

            irqstat0 = irqstatus & 0xFF;
            irqstat1 = (irqstatus >> 8) & 0xFF;
            irqstat2 = (irqstatus >> 16) & 0xFF;

            Log::console (PSTR ("IRQ status 1,2,3    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);

        }
    }

    if (!PMU) {
         /*
          * In XPowersLib, if the XPowersAXPxxx object is released, Wire.end() will be called at the same time.
          * In order not to affect other devices, if the initialization of the PMU fails, Wire needs to be re-initialized once,
          * if there are multiple devices sharing the bus.
          * * */
        Wire.begin (i2c_sda, i2c_scl);
        return;
    }
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

    // if (PMU && chipID == XPOWERS_AXP192) {
        // AXPchip = 1;
        // Log::console (PSTR ("AXP192 found"));   // T-Beam V1.1 with AXP192 power controller

        // // lora radio power channel
        // PMU->setPowerChannelVoltage (XPOWERS_LDO2, 3300);
        // PMU->enablePowerOutput (XPOWERS_LDO2);

        // // oled module power channel,
        //    // disable it will cause abnormal communication between boot and AXP power supply,
        //    // do not turn it off
        // PMU->setPowerChannelVoltage (XPOWERS_DCDC1, 3300);
        // // enable oled power
        // PMU->enablePowerOutput (XPOWERS_DCDC1);

        // // gnss module power channel -  turned off
        // PMU->setPowerChannelVoltage (XPOWERS_LDO3, 3300);
        // PMU->disablePowerOutput (XPOWERS_LDO3);

        // // protected oled power source
        // PMU->setProtectedChannel (XPOWERS_DCDC1);
        // // protected esp32 power source
        // PMU->setProtectedChannel (XPOWERS_DCDC3);

        // // disable not use channel
        // PMU->disablePowerOutput (XPOWERS_DCDC2);

        // // disable all axp chip interrupt
        // PMU->disableIRQ (XPOWERS_AXP192_ALL_IRQ);

        // // Set constant current charging current
        // PMU->setChargerConstantCurr (XPOWERS_AXP192_CHG_CUR_450MA);

        // // Set up the charging voltage
        // PMU->setChargeTargetVoltage (XPOWERS_AXP192_CHG_VOL_4V2);
        // PMU->setSysPowerDownVoltage (3200);

        // uint16_t battV = PMU->getBattVoltage ();
        // uint16_t sysV = PMU->getSystemVoltage ();

        // Log::console (PSTR ("PMU battV,sysV : %02X,%02X"), battV, sysV);

        // //Log::console (PSTR ("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);

        // uint64_t irqstatus = PMU->getIrqStatus ();

        // irqstat0 = irqstatus & 0xFF;
        // irqstat1 = (irqstatus >> 8) & 0xFF;
        // irqstat2 = (irqstatus >> 16) & 0xFF;

        // Log::console (PSTR ("IRQ status 1,2,3    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
    // } else if (PMU && chipID == XPOWERS_AXP2101) {

        // AXPchip = 2;
        // Log::console (PSTR ("AXP2101 found"));  // T-Beam V1.2 with AXP2101 power controller

        // // Unuse power channel
        // PMU->disablePowerOutput (XPOWERS_DCDC2);
        // PMU->disablePowerOutput (XPOWERS_DCDC3);
        // PMU->disablePowerOutput (XPOWERS_DCDC4);
        // PMU->disablePowerOutput (XPOWERS_DCDC5);
        // PMU->disablePowerOutput (XPOWERS_ALDO1);
        // PMU->disablePowerOutput (XPOWERS_ALDO4);
        // PMU->disablePowerOutput (XPOWERS_BLDO1);
        // PMU->disablePowerOutput (XPOWERS_BLDO2);
        // PMU->disablePowerOutput (XPOWERS_DLDO1);
        // PMU->disablePowerOutput (XPOWERS_DLDO2);

        // // GNSS RTC PowerVDD 3300mV
        // PMU->setPowerChannelVoltage (XPOWERS_VBACKUP, 3000);
        // PMU->enablePowerOutput (XPOWERS_VBACKUP);

        // // ESP32 VDD 3300mV
        // //  ! No need to set, automatically open , Don't close it
        // //PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        // //PMU->setProtectedChannel(XPOWERS_DCDC1);
        // uint16_t dcdc1_voltage = PMU->getPowerChannelVoltage (XPOWERS_DCDC1);
        // Log::console (PSTR ("DCDC1 voltage : %d mV"), dcdc1_voltage);

        // // Enable LCD voltage
        // PMU->setPowerChannelVoltage (XPOWERS_DCDC3, 3300);
        // PMU->enablePowerOutput (XPOWERS_DCDC3);

        // // LoRa VDD 3300mV
        // PMU->setPowerChannelVoltage (XPOWERS_ALDO2, 3300);
        // PMU->enablePowerOutput (XPOWERS_ALDO2);

        // // GNSS VDD 3300mV but disable it
        // PMU->setPowerChannelVoltage (XPOWERS_ALDO3, 3300);
        // PMU->disablePowerOutput (XPOWERS_ALDO3);
        // // Set constant current charging current
        // PMU->setChargerConstantCurr (XPOWERS_AXP192_CHG_CUR_450MA);

        // // Set up the charging voltage
        // PMU->setChargeTargetVoltage (XPOWERS_AXP192_CHG_VOL_4V2);
        // PMU->setChargingLedMode (XPOWERS_CHG_LED_BLINK_1HZ);

        // PMU->setSysPowerDownVoltage (3200);

        // uint16_t battV = PMU->getBattVoltage ();
        // uint16_t sysV = PMU->getSystemVoltage ();

        // Log::console (PSTR ("PMU battV,sysV : %02X,%02X"), battV, sysV);

        // uint64_t irqstatus = PMU->getIrqStatus ();

        // irqstat0 = irqstatus & 0xFF;
        // irqstat1 = (irqstatus >> 8) & 0xFF;
        // irqstat2 = (irqstatus >> 16) & 0xFF;
        // Log::console (PSTR ("IRQ status 0,1,2    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);

        // I2CwriteByte (0x34, 0x93, 0x1C);       // set ALDO2 voltage to 3.3V ( LoRa VCC )
        // I2CwriteByte (0x34, 0x94, 0x1C);       // set ALDO3 voltage to 3.3V ( GPS VDD )
        // I2CwriteByte (0x34, 0x6A, 0x04);       // set Button battery voltage to 3.0V ( backup battery )
        // I2CwriteByte (0x34, 0x64, 0x03);       // set Main battery voltage to 4.2V ( 18650 battery )
        // I2CwriteByte (0x34, 0x61, 0x05);       // set Main battery precharge current to 125mA
        // I2CwriteByte (0x34, 0x62, 0x0A);       // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
        // I2CwriteByte (0x34, 0x63, 0x15);       // set Main battery term charge current to 125mA
        // regV = I2CreadByte (0x34, 0x90);       // XPOWERS_AXP2101_LDO_ONOFF_CTRL0
        // regV = regV | 0x06;                   // set bit 1 (ALDO2) and bit 2 (ALDO3)
        // I2CwriteByte (0x34, 0x90, regV);       // and power channels now enabled
        // regV = I2CreadByte (0x34, 0x18);       // XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL
        // regV = regV | 0x06;                   // set bit 1 (Main Battery) and bit 2 (Button battery)
        // I2CwriteByte (0x34, 0x18, regV);       // and chargers now enabled
        // I2CwriteByte (0x34, 0x14, 0x30);       // set minimum system voltage to 4.4V (default 4.7V), for poor USB cables
        // I2CwriteByte (0x34, 0x15, 0x05);       // set input voltage limit to 4.28v, for poor USB cables
        // I2CwriteByte (0x34, 0x24, 0x06);       // set Vsys for PWROFF threshold to 3.2V (default - 2.6V and kill battery)
        // I2CwriteByte (0x34, 0x50, 0x14);       // set TS pin to EXTERNAL input (not temperature)
        // I2CwriteByte (0x34, 0x69, 0x01);       // set CHGLED for 'type A' and enable pin function
        // I2CwriteByte (0x34, 0x27, 0x00);       // set IRQLevel/OFFLevel/ONLevel to minimum (1S/4S/128mS)
        // I2CwriteByte (0x34, 0x30, 0x0F);       // enable ADC for SYS, VBUS, TS and Battery
        // pmustat1 = I2CreadByte (0x34, 0x00); pmustat2 = I2CreadByte (0x34, 0x01);
        // pwronsta = I2CreadByte (0x34, 0x20); pwrofsta = I2CreadByte (0x34, 0x21);
        // irqstat0 = I2CreadByte (0x34, 0x48); irqstat1 = I2CreadByte (0x34, 0x49); irqstat2 = I2CreadByte (0x34, 0x4A);
        // Log::console (PSTR ("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);
        // Log::console (PSTR ("PWRON,PWROFF status : %02X,%02X"), pwronsta, pwrofsta);
        // Log::console (PSTR ("IRQ status 0,1,2    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
    // }
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //Wire.end ();
}