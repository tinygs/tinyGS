# Design

## Context

See `proposal.md` for motivation. Current state that shapes the approach:

- `ConfigManager::configSavedCallback()` (`ConfigManager.cpp:854`) runs inside `saveConfig()`, which the save branch of `IotWebConf2::handleConfig()` calls *before* sending the "Configuration saved." page. The callback restarts the station inline (`ESP.restart()`, only when the station name differs from `savedThingName`), so the response never leaves the device. The console's own `/restart` handler already does it in the right order: it sends the page, waits `delay(100)` and then restarts.
- `TinyGSImprov::connectWifi()` (`tinygs_improv.cpp:73`) calls `WiFi.disconnect()` — asynchronous — and `WiFi.begin()` immediately. The ESP-IDF station layer refuses to apply a new configuration while an attempt is in flight: `E wifi:sta is connecting, cannot set config`, and the command ends in `ERROR_UNABLE_TO_CONNECT` even with valid credentials. Right after boot the station is connecting, so provisioning in that window fails.
- The deferred restart must be consumed from the main loop: `IotWebConf2::doLoop()` is not virtual, so the check goes next to the existing `configManager.doLoop()` call in `tinyGS.ino`'s loop.
- `WiFiSTA::disconnect(bool wifioff, bool eraseap, ...)` can power the station off, ending any in-flight attempt (Arduino-ESP32 `WiFiSTA.h:169`).

## Goals / Non-Goals

**Goals:**

- The save confirmation is delivered before any restart the save triggers.
- Credentials received over Improv are applied regardless of an in-flight connection attempt.
- No changes to the Improv protocol, the console form, the config format or any stored setting.

**Non-Goals:**

- Changing the vendored IotWebConf2 library: its contract (save, then answer) stays as it is; the deferred restart lives in tinyGS's own code.
- Changing the behaviour of saves that do not require a restart.
- Adding retries or backoff to WiFi connection logic beyond what is needed to apply the credentials.

## Decisions

1. **Deferred restart with a flush margin**: the save callback records a deadline (~1 s) instead of restarting, and `ConfigManager` restarts when the main loop passes it (`checkScheduledRestart()`, called next to `configManager.doLoop()` in `tinyGS.ino`). One second is far more than the ~100 ms the console's own restart pattern relies on, so the queued response flushes first. Alternative discarded: reordering `saveConfig()` after the response inside IotWebConf2 — it modifies the vendored library and its contract for a problem local to tinyGS.

2. **Full station reset before applying credentials**: `connectWifi()` powers the station off (`WiFi.disconnect(true)`), re-arms station mode and then starts the connection, so no attempt can be in flight when the new configuration is applied. Alternative discarded: retrying `WiFi.begin()` — a retry does not clear the state that makes the driver refuse the new configuration, and it turns a deterministic failure into a flaky one.

3. **Verification on hardware with the HIL bench**: the console save cycle (write profile → restart → read back identical) proves the save flow, and the bench's WiFi provisioning path (which provisions right after a reset, while the station is connecting) reproduces the race.

## Risks / Trade-offs

- [The deferred restart could be starved if the main loop never reaches the check] → the check sits next to the periodic `configManager.doLoop()` call; worst case the station restarts on the next reboot and the configuration was already saved.
- [Powering the station off briefly drops an in-flight connection while re-provisioning] → the command is re-provisioning the network anyway, and the previous code already disconnected before starting.
- [The flush margin could be too short on a slow client] → 1 s is an order of magnitude above the margin the console itself uses (`delay(100)`); the response is queued before the deadline can fire.

## Migration Plan

Ships with the regular release; it changes no persisted configuration, so rollback is a revert of the commit. The HIL bench's provisioning workaround (boot window) remains for old firmware versions during migrations; it simply stops being needed for firmware with this fix.

## Open Questions

- None.
