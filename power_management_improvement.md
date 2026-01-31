# TinyGS Power Management Improvement Plan

## Goal
Minimize power consumption for solar/battery-powered stations (specifically LilyGo T-Beam Supreme) without sacrificing core functionality (receiving satellite packets).

## Current Status (Baseline)
- **ESP32-S3:** Always Active (WiFi connected).
- **Radio:** Always Rx (Listening).
- **GNSS:** Managed (Sleeps when fix obtained).
- **OLED:** Managed (Times out).
- **Sensors:** Managed (Compass/Baro put to deep sleep).
- **PMU Rails:** Optimized (Unused rails disabled).
- **Estimated Draw:** ~115mA @ 3.7V.

## Planned Improvements

### 1. USB PHY Management (High Impact: ~5-10mA)
**Concept:** The USB CDC peripheral consumes significant power even when disconnected. We can disable it when running on battery.
**Implementation:**
- Monitor `Power::getVbusVoltage()` in `loop()`.
- If `VBUS < 4.0V` (Battery Mode): Call `Serial.end()` to disable CDC/PHY.
- If `VBUS > 4.0V` (Plugged In): Call `Serial.begin(115200)` to re-enable logging.
**Safety:** Recovery is always possible via hardware bootloader mode (Boot button + Reset).

### 2. Intelligent Radio Sleep (High Impact: ~5mA)
**Concept:** Satellites are only visible for small windows of time. Listening to static 95% of the time wastes power.
**Implementation:**
- Utilize the internal TLE propagator.
- If no configured satellite is above the horizon (Elevation < 0), put the SX1262 radio into `SLEEP` mode.
- Wake up radio X minutes before AOS (Acquisition of Signal).
**Challenge:** Requires robust TLE propagation logic and handling of "unknown" or "new" satellites which might require constant scanning.

### 3. ESP32 Light Sleep (Medium Impact: ~10-20mA)
**Concept:** The ESP32-S3 is dual-core 240MHz. It spends most time waiting for interrupts (Radio Rx) or network events.
**Implementation:**
- Enable `Automatic Light Sleep` in ESP-IDF configuration.
- Allows the CPU to pause clock/voltage when idle, waking instantly on interrupts (WiFi, Radio DIO, Timer).
- **Constraint:** WiFi connection must be maintained (DTIM beacon interval).

### 4. CPU Frequency Scaling (Low Impact: ~5-10mA)
**Concept:** 240MHz is overkill for waiting.
**Implementation:**
- Drop CPU frequency to 80MHz or 160MHz when not processing heavy tasks (like OTA or heavy JSON parsing).
- **Status:** Currently hardcoded to 240MHz in `setup()`.

### 5. Server-Driven Schedule (Protocol Change)
**Concept:** Instead of the station calculating passes or staying awake, the server tells the station exactly when to wake up.
**Implementation:**
- **Protocol:** Add `next_pass_epoch` field to the MQTT configuration packet.
- **Behavior:**
    - Station receives config + `next_pass_epoch`.
    - If `next_pass_epoch` is > 10 minutes away:
        - Calculate duration.
        - Send `!d <duration>` (Deep Sleep) command internally.
        - Station powers down WiFi/CPU/Radio completely (~7µA).
        - Wakes up 2 minutes before `next_pass_epoch` to reconnect and tune.
- **Trade-off:** The station becomes "deaf" to new commands during sleep. It cannot be dynamically re-tasked until it wakes up.
- **Savings:** Massive. Reduces average current from ~100mA to <1mA for sparse satellite schedules.

## Roadmap
1. **Phase 1 (Safe):** Implement USB PHY switching based on VBUS.
2. **Phase 2 (Complex):** Prototype Radio Sleep logic based on elevation.
3. **Phase 3 (System):** Experiment with Light Sleep and CPU frequency scaling (requires stability testing with WiFi).
4. **Phase 4 (Protocol):** Implement Server-Driven Deep Sleep for ultimate power saving.
