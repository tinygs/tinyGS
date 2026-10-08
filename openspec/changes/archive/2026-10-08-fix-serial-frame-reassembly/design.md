# Design

## Context

Current state of the serial input path (see `proposal.md` for motivation and evidence):

- `handleSerial()` (`tinyGS/tinyGS.ino:385`) checks `Serial.peek()`: if it is `'I'` it calls `TinyGSImprov::handleImprovPacket()`, otherwise `handleRawSerial()`. The routing decision is made per pass, not per frame.
- `TinyGSImprov::handleImprovPacket()` (`tinyGS/src/Improv/tinygs_improv.cpp:213`) drains `while (Serial.available() > 0)`: it exits as soon as it catches up with the sender (the UART delivers ~1 byte/87 µs), leaving frames half-received. It keeps `improvBufferPosition` across calls (private, with no timeout) and discards invalid bytes one by one.
- `handleRawSerial()` (`tinyGS/tinyGS.ino:398`) consumes one byte, waits 500 ms (`configManager.delay`) to group input and runs `while (Serial.available()) Serial.read();` without resetting parser state.
- The parser (`parse_improv_serial_byte` from `improv/Improv@^1.2.4`) is already incremental and noise-tolerant between frames; the problem is that it only receives bytes while the drain is active.
- The firmware has no unit-test infrastructure (`test/` holds only a README): verification runs on hardware with the HIL bench.

## Goals / Non-Goals

**Goals:**

- Whole Improv frames under any arrival pattern (spread across `loop()` passes, with inter-byte gaps), with a minimal change to the shipped binary.
- No observable behavior change for the CLI or the Improv protocol (same commands, same grouping delay, same frames).
- Guaranteed resynchronization: a truncated frame never leaves input "stuck" on the parser.

**Non-Goals:**

- Changing the format or semantics of the Improv protocol, or adding commands.
- Redesigning the maintenance CLI (it only has to stop interfering with the parser).
- On-target unit tests (they would need a test build that does not certify the release binary; verification is HIL).
- Removing the HIL bench boot-window workaround (left as a follow-up of `hardware-integration-tests`, which must keep working against old firmware during migrations).

## Decisions

1. **Route by "frame in progress", in addition to the `'I'` byte**: `handleSerial()` keeps feeding the parser while a half-received frame is pending, even if the next byte does not start a frame. Parser state is queried through a public predicate (`isParsing()`), keeping `improvBufferPosition` encapsulated. The parser also resets its position as soon as a frame completes or fails, handing the buffered remainder back to the dispatcher so CLI input right after a frame is served instead of being swallowed. Alternative discarded: gap tolerance inside the drain only — it still loses frames when the gap exceeds the margin, and it does not stop the CLI from flushing the buffer.

2. **Partial-frame timeout (resynchronization)**: if a frame is half-received and no byte arrives for a short inactivity gap (~100 ms; the wire gap at 115200 bps is 87 µs per byte), the parser discards its partial state and the dispatcher handles the following input fresh. The timeout is the only usable resynchronization signal: inside the payload region the parser accepts any byte, so a fresh frame's magic gets absorbed by a dead partial frame and cannot be distinguished at byte level. A short gap recovers truncated senders, quick retries and the CLI immediately, and still tolerates frames split across `loop()` passes (tens of ms). Alternatives discarded: a long timeout (~2 s) — a retry or CLI command sent within 2 s of a truncated frame would be swallowed (verified on hardware); resetting the parser only when the CLI consumes bytes — it misses senders that vanish and couples both consumers.

3. **`handleRawSerial()` keeps its flow as-is (500 ms grouping + garbage flush)**: with decision 1 its bytes can never belong to an in-progress frame, so its flush only eats orphan input. Alternative discarded: removing the buffer flush — it would change the CLI's current behavior, out of scope.

4. **End-to-end verification with the HIL bench on real hardware**: hot WiFi provisioning (no boot window), CLI working (`!o`, `!b`) with and without Improv traffic, a truncated frame followed by a CLI command and by a complete frame. Alternative discarded: on-target Unity tests (no infrastructure, and they would not certify the release binary).

## Risks / Trade-offs

- [Host-side USB serial buffering can coalesce two writes made a few hundred ms apart into one wire burst, so a quick retry merges into the dead partial frame and is lost] → senders must write each frame in a single write and leave a gap above the inactivity period before retrying; verified on hardware that nominal gaps ≥ 400 ms recover reliably.

- [The "frame in progress" state could capture CLI input if a frame is truncated] → bounded by the inactivity gap of decision 2 (~100 ms of capture at most).
- [The CLI grouping delay (500 ms) may overlap the start of an Improv frame] → those bytes stay buffered and reach the parser when routing resumes; the edge case is absorbed by the timeout.
- [The certified binary is touched] → the change is localized to input dispatch, with no effect on radio/MQTT/config; the HIL bench validates it end to end before it ships in a release.
- [Behavior differs between firmware with and without the fix during HIL-tested migrations] → the bench workaround (boot window) works with both and is retired afterwards.

## Migration Plan

Ships with the regular release; it does not touch persisted configuration, so rollback = revert of the commit. The HIL bench keeps its workaround until the fix is verified on the versions it migrates.

## Open Questions

- None — the partial-frame timeout is resolved at ~100 ms of inter-byte inactivity (hardware verification showed a long timeout conflicts with recovery of quick retries after a truncated frame).
