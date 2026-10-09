# Tasks

## 1. Configuration save: confirmation before restart

- [x] 1.1 Implement the deferred restart (the save callback records a deadline and the main loop restarts once past it, next to `configManager.doLoop()`) and verify on the reference board that saving a new station name from the console delivers the confirmation page and then restarts applying the configuration

## 2. Improv provisioning: apply credentials with a connection in flight

- [x] 2.1 Implement the full station reset before applying credentials in `connectWifi()` (`WiFi.disconnect(true)` before re-arming station mode and connecting) and verify on the reference board that credentials sent over Improv while the station is connecting —including right after a reset— are applied, the station joins the network and answers with its local URL

## 3. End-to-end verification with the HIL bench

- [x] 3.1 Run the bench's WiFi provisioning suite (`tests/hil/tests/test_wifi_setup.py`) and the console save cycle (`tests/hil/tests/test_device_console.py`) on this firmware and verify both pass — provisioning without the boot-window workaround and the save cycle reading the profile back identical
