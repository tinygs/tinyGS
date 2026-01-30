# TinyGS Project Notes

## Device Status
- **Port:** `/dev/ttyACM0`
- **Board:** `lilygo-t-beam-supreme`
- **Detected:** Yes
- **Date:** 2026-01-22

## Configuration
- **Project Path:** `/home/bruce/dev/tinygs`
- **PlatformIO Config:** `platformio.ini` present
- **Possible Environments:**
    - ESP32 (heltec_wifi_lora_32)
    - ESP32-S3 (heltec_wifi_lora_32_V3)
    - ESP32-S3-USB (Native USB)
    - ESP32-C3
    - ESP32-LILYGO_SX1280
    - lilygo-t-beam-supreme

## Investigation Log
- Detected `/dev/ttyACM0`.
- Monitor active; device running TinyGS 2601184.
- **Bug Found:** Device crashes on `!p` command when unconfigured.
- **Crash Analysis:**
  - Backtrace decoded: Crash at `Radio::sendTx` in `Radio.cpp:378`.
  - Cause: `radioHal` is NULL because `Radio::init()` is only called after configuration.
  - Fix: Patched `tinyGS/tinyGS.ino` to check `radio.isReady()` before `!p`.
- **Capabilities Updated:** Agent can build and deploy firmware autonomously.
- **Next Steps:**
  1. Build and upload patched firmware. (Done)
  2. Configure device using OTP.
  3. **TODO:** Fix AXP2101 battery reading (currently reporting 0.00V 10% despite battery installed).
  4. **TODO:** Investigate why WiFi fails to connect to "iot" network despite correct credentials.

## Development Notes
- Always use python to monitor the serial device (e.g., `pyserial` or custom scripts) rather than `pio device monitor` if possible, or ensure correct reset sequences.