# TinyGS Contribution Plan

This document outlines a staged approach to submitting the recent improvements and features for the LilyGo T-Beam Supreme and general codebase to the upstream TinyGS repository.

## Overview

The changes have been grouped into 7 logical Pull Requests (PRs) to ensure atomic reviews, easier merging, and correct dependency management.

---

## PR 1: Core Stability & Critical Bug Fixes
**Objective:** Fix crashes and stability issues affecting all boards. Independent of new features.

*   **Radio Stability:** Initialize `radioHal` to `nullptr` and add null pointer checks in critical `Radio.cpp` methods (`begin`, `clearPacketReceivedAction`, etc.) to prevent `LoadProhibited` panics when MQTT commands arrive early.
*   **Startup Sequence:** Call `radio.init()` earlier in `setup()` to ensure the HAL is ready before network connections.
*   **Timezone Safety:** Add bounds checking to `getTZ()` in `ConfigManager.h` to prevent crashes with unconfigured timezones.
*   **Web Interface Bug:** Fix the `wmx.status` check in `html.h` (was incorrectly checking `x.status`) which caused dashboard refresh failures.

**Files:** `src/Radio/Radio.cpp`, `tinyGS.ino`, `src/ConfigManager/ConfigManager.h`, `src/ConfigManager/html.h`

---

## PR 2: Power Management Refactor
**Objective:** Modernize the Power module to support newer PMUs and unify battery reading logic.

*   **AXP2101:** Add support for the X-Powers AXP2101 PMU (used in T-Beam Supreme and T-Beam S3 Core).
*   **Unified Battery Logic:** Refactor `Power.cpp` to use a consistent `LEGACY_BATT_PIN` fallback for non-PMU boards, removing hardcoded magic numbers from board definitions.
*   **Voltage Precision:** Improve battery voltage reporting precision (1mV step).
*   **GNSS Power Control:** Add `setGnssPower(bool on)` method stub/implementation to `Power` class (to be used by GNSS Manager).

**Files:** `src/Power/Power.h`, `src/Power/Power.cpp`

---

## PR 3: Configuration Structure Updates
**Objective:** Update configuration structures and enums to support future features (GNSS, New Boards) without implementing the logic yet. This resolves circular dependencies.

*   **Board Struct:** Update `board_t` in `ConfigManager.h` to include `GNSS_RX`, `GNSS_TX`, and `GNSS_WAKEUP` fields.
*   **Board Enum:** Add `LILYGO_TBEAM_SUPREME` to `boardNum` enum.
*   **Parameters:** Add `autoLocation` and `gnssInterval` buffer and parameter declarations to `ConfigManager` class (initialized but logic not active).
*   **Constants:** Add T-Beam Supreme pin definitions.

**Files:** `src/ConfigManager/ConfigManager.h`

---

## PR 4: GNSS Manager Core
**Objective:** Introduce the GNSS management logic. Depends on PR 2 and PR 3.

*   **New Module:** Add `src/GnssManager/` class to handle GPS serial communication, time syncing, and power cycling logic.
*   **Loop Integration:** Hook `GnssManager::loop()` into `tinyGS.ino` main loop.
*   **Time Sync:** Implement `setupNTP()` and GNSS time sync logic.

**Files:** `src/GnssManager/*`, `tinyGS.ino`

---

## PR 5: Dashboard & Data Stream Alignment
**Objective:** Fix data misalignment and display new sensors. Depends on PR 4 (GNSS Manager).

*   **Data Stream Alignment:** Rewrite `ConfigManager::handleRefreshWorldmap` to ensure the CSV data stream (AJAX) perfectly matches the expected indices in the JavaScript.
*   **JavaScript Sync:** Update `IOTWEBCONF_WORLDMAP_SCRIPT` in `html.h` to correctly map the new data fields (Battery, GNSS status) to the HTML tables.
*   **Visuals:** Move "Noise Floor" to the Modem Config table and add specific rows for "GNSS Status" and "Battery" in the Groundstation Status table.
*   **Fixes:** Correct the "Coding Rate" duplicate label issue for LoRa modems.

**Files:** `src/ConfigManager/ConfigManager.cpp`, `src/ConfigManager/html.h`

---

## PR 6: New Board Support - LilyGo T-Beam Supreme
**Objective:** Add official support for the LilyGo T-Beam Supreme (S3 Core). Depends on PR 3 (Structs).

*   **Board Array:** Update `boards` array in `ConfigManager.cpp` to include the T-Beam Supreme definition using constants from PR 3.
*   **Auto-Detection:** Add logic in `boardDetection()` to identify the board via the AXP2101 on Wire1.
*   **PlatformIO:** Add `lilygo-t-beam-supreme` environment to `platformio.ini`.

**Files:** `src/ConfigManager/ConfigManager.cpp`, `platformio.ini`

---

## PR 7: Auto-Location & Mobile Features
**Objective:** Enable dynamic location updates. Depends on PR 4, 5, 6.

*   **Configuration Logic:** Update `ConfigManager.cpp` to register `autoLocationParam` and `gnssIntervalParam` in the web interface and handle their storage.
*   **Logic:** Update `GnssManager` to respect update intervals and trigger runtime location updates.
*   **MQTT Push:** Add logic in `tinyGS.ino` to detect location changes and call `mqtt.sendStatus()` immediately.
*   **Defaults:** Enable Auto Location by default for GPS-equipped boards.

**Files:** `src/ConfigManager/ConfigManager.cpp`, `src/GnssManager/GnssManager.cpp`, `tinyGS.ino`, `src/ConfigManager/html.h` (CSS updates)
