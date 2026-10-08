# Proposal

## Why

The firmware cannot receive long Improv frames: `handleSerial()` (`tinyGS.ino`) only reassembles a frame within a single `loop()` pass — it routes to the parser only if the next available byte is `'I'` — and stray continuation bytes fall into `handleRawSerial()`, which reads one byte, waits 500 ms and **flushes the serial buffer** without resetting parser state. Short frames (`GET_CURRENT_STATE`, 12 B) fit in one pass and get answered; the credentials frame (`WIFI_SETTINGS`, 49 B ≈ 4 ms at 115200) arrives split and dies silently, making WiFi provisioning over Improv-serial impossible outside the boot-window workaround the HIL bench uses. Evidence: raw probe on a real board — `GET_CURRENT_STATE` answered `STATE_AUTHORIZED`; `WIFI_SETTINGS` received no answer at all for 75 s.

## What Changes

- **Improv frame reassembly independent of arrival timing**: serial dispatch keeps feeding the parser while a frame is in progress, even when bytes arrive spread across `loop()` passes or with inter-byte gaps.
- **No interference between serial input consumers**: the maintenance CLI (`!e`, `!b`, `!p`, `!w`, `!o`) only consumes input that does not belong to an in-progress Improv frame and no longer destroys frames through its pending-input flush.
- **Resynchronization after truncated frames**: an Improv frame left half-sent (the sender dies mid-transfer) degrades neither the CLI nor the frames that follow; the parser discards its partial state after an inactivity timeout.
- WiFi provisioning over Improv-serial works **at any moment** of the device's life (the HIL bench can retire its boot-window workaround once it tests firmware with this fix; until then it keeps it for compatibility with older versions during migrations).

## Capabilities

### New Capabilities

- `serial-input-dispatch`: dispatch of the USB serial input between the Improv protocol and the maintenance CLI — frame reassembly under arbitrary arrival patterns, separation between consumers, and resynchronization after truncated input.

### Modified Capabilities

<!-- None: `mqtt-remote-commands` is unchanged (MQTT dispatch, not serial). -->

## Impact

- `tinyGS/tinyGS.ino`: `handleSerial()` and `handleRawSerial()` (routing and buffer flushing).
- `tinyGS/src/Improv/tinygs_improv.{h,cpp}`: `handleImprovPacket()` and the parser state (`improvBufferPosition`), currently private and without a timeout.
- No changes to CLI commands or existing Improv commands/frames (format and semantics intact); persisted configuration untouched.
- End-to-end verification on hardware with the HIL bench (`tests/hil/tests/test_wifi_setup.py` and an Improv exchange probe); the firmware has no unit-test infrastructure (`test/` holds only a README).
