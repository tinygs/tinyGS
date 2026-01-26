# LilyGo T-Beam Supreme (S3) Setup for TinyGS

The LilyGo T-Beam Supreme is an ESP32-S3 based board with an SX1262 LoRa radio, AXP2101 PMU, and an integrated GPS (Ublox M10 or L76K).

## Pin Mapping (Verified)

| Function | Pin (GPIO) |
| :--- | :--- |
| **LoRa CS (NSS)** | 10 |
| **LoRa DIO1** | 1 |
| **LoRa BUSY** | 4 |
| **LoRa RESET** | 5 |
| **LoRa SCK** | 12 |
| **LoRa MISO** | 13 |
| **LoRa MOSI** | 11 |
| **I2C SDA** | 17 |
| **I2C SCL** | 18 |
| **GPS TX** | 8 |
| **GPS RX** | 9 |
| **GPS Wakeup** | 7 |
| **PMU SDA** | 42 (Wire1) |
| **PMU SCL** | 41 (Wire1) |
| **User Button** | 0 |
| **Green LED** | 3 |

## Features Added

### 1. Power Management (AXP2101)
- Full support for AXP2101 PMU via Wire1.
- Correct battery voltage reading (1mV/bit).
- Accurate fuel gauge percentage.
- Automatic power control for Radio and GPS.

### 2. GNSS Integration
- Automatic time sync on fix.
- **Auto-Location**: Dynamically updates station coordinates when moved.
- **Configurable Intervals**: Set how often the GPS wakes up to update location (saves battery).
- **Dashboard Status**: Shows `FIX`, `NO FIX`, or `SLEEP` state.

### 3. Mobile Support
- Station pushes its new location to the TinyGS MQTT server immediately when GNSS detects a change.
- Defaulted to "Auto Location" enabled for this board.

## Configuration

1. Connect to the TinyGS Access Point.
2. Navigate to `http://192.168.4.1/config`.
3. Select **LilyGo T-Beam Supreme (SX1262)** as the board type.
4. (Optional) Enable **Auto Location (GNSS)** and set the **GNSS Update Interval**.
5. Save and Restart.

## Troubleshooting

- **No Packets**: Ensure you are using a 433MHz or 868/915MHz antenna matching your board variant. The SX1262 pins are identical for all frequency variants.
- **No GPS Fix**: The antenna needs a clear view of the sky. "NO FIX" is common indoors.
- **Battery 0.00V**: Ensure the board is selected as "Supreme" to enable the AXP2101 driver.
