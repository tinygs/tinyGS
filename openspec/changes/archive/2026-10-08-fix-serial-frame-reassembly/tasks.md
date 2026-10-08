# Tasks

## 1. Improv frame reassembly

- [x] 1.1 Expose parser state through a public predicate that keeps encapsulation (`isParsing()`) and route continuation bytes of an in-progress frame to the parser in `handleSerial()`, in addition to the `'I'` byte; verify on the reference board that a ~50-byte `WIFI_SETTINGS` sent at 115200 bps with the board running its main loop (arrival spread across passes) is parsed whole and the board answers with state and local URL
- [x] 1.2 Implement the partial-frame timeout (the parser discards its half-done state after a short inactivity gap of ~100 ms with no bytes) and verify that a truncated frame followed by a CLI command runs the command normally, and that a complete frame sent after the truncated one is parsed and answered

## 2. Coexistence with the maintenance CLI

- [x] 2.1 Verify on hardware that the CLI does not consume bytes of an in-progress Improv frame (a CLI command right after the start of a frame: the frame completes and the CLI is only served if there is real input) and that the standalone CLI keeps its current behavior (`!o`, `!b`, with its grouping and garbage flush)

## 3. End-to-end provisioning verification

- [x] 3.1 Provision WiFi over Improv with the board already booted and running its main loop —no prior reset, no boot window— and verify that it joins the network, persists the credentials and answers with its local URL (HIL bench from branch `feature/hardware-integration-tests`, or an equivalent exchange probe)
- [x] 3.2 Run the HIL bench's full provisioning path (`tests/hil/tests/test_wifi_setup.py`, including persistence across a reset) on the reference board with this firmware and verify it passes with the boot-window workaround unused
