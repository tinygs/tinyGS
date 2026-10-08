# Proposal

## Why

Applying configuration on the station is not reliable in two flows that any client —the board console in a browser, or an Improv-serial provisioning tool— goes through:

- **Saving the configuration cuts the answer.** `ConfigManager::configSavedCallback()` restarts the station with `ESP.restart()` as soon as the station name changes, and it runs inside `saveConfig()` — that is, *before* `handleConfig` sends the "Configuration saved." page. The browser sees the connection drop instead of the confirmation, and a client cannot tell a successful save from a failure (observed with the HIL bench: the form POST never completes; only the reboot explains it).
- **Provisioning WiFi over Improv fails if the station is already connecting.** `TinyGSImprov::connectWifi()` calls `WiFi.disconnect()` (asynchronous) and `WiFi.begin()` immediately. If a connection attempt is in flight — typical right after boot, which is exactly when the HIL bench provisions — the ESP-IDF station layer rejects the new configuration: `E wifi:sta is connecting, cannot set config`, `Wifi status: 6`, provisioning answers with `ERROR_UNABLE_TO_CONNECT` even though the credentials are valid.

Both are robustness bugs in flows this bench exercises on every run, and both are fixed without changing any protocol, config format or user-visible setting.

## What Changes

- **Save confirmation before restart**: when a configuration save requires a restart (station-name change), the station delivers the "Configuration saved." response first and restarts shortly after, so the console shows the confirmation and the client knows the save succeeded.
- **Improv provisioning robust to an in-flight connection**: the WiFi credentials command applies the new configuration even when the station is already attempting a connection, so provisioning right after boot (or during any retry) joins the network and answers with its local URL as usual.

## Capabilities

### New Capabilities

- `config-apply`: applying the station's configuration — saving from the web console and provisioning WiFi credentials over Improv-serial — completes cleanly, without cutting the confirmation or failing because of an in-flight connection attempt.

### Modified Capabilities

<!-- None: `serial-input-dispatch` (frame reassembly) and `mqtt-remote-commands` are unchanged. -->

## Impact

- `tinyGS/src/ConfigManager/ConfigManager.{h,cpp}`: the deferred restart for a saved configuration.
- `tinyGS/src/Improv/tinygs_improv.cpp`: how WiFi credentials are applied.
- No changes to the Improv protocol, the config format, the console form or any stored setting.
- Verified on hardware with the HIL bench (`tests/hil`): the console save cycle and the WiFi provisioning suite.
