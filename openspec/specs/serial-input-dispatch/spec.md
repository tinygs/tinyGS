# serial-input-dispatch Specification

## Purpose

Dispatch the station's USB serial input between the Improv protocol and the maintenance CLI: receive frames whole regardless of arrival timing, prevent either consumer from destroying the other's input, and resynchronize after truncated input.

## Requirements

### Requirement: Improv protocol frames are received whole under arbitrary arrival timing

The station SHALL accept Improv protocol frames on the USB serial port even when their bytes arrive spread over time —across passes of its main loop or with inter-byte gaps shorter than the partial-frame inactivity period— and SHALL answer with the corresponding result frame, regardless of the frame size.

#### Scenario: Long frame arriving split

- **WHEN** an Improv WiFi credentials frame (~50 bytes at 115200 bps) is transmitted with its bytes spread across several passes of the main loop
- **THEN** the station parses it whole and answers with its state and local URL after provisioning, or with the corresponding error

#### Scenario: Short frame without regression

- **WHEN** an Improv state query (a ~12-byte frame) is sent
- **THEN** the station answers with its current state, as it does today

### Requirement: The maintenance CLI does not interrupt in-progress Improv frames

Serial input SHALL be split between the Improv protocol and the maintenance CLI without either consumer destroying the other's input: while an Improv frame is in progress, those bytes SHALL NOT be treated as CLI commands nor discarded by its pending-input flush.

#### Scenario: CLI command while a frame is in progress

- **WHEN** a CLI command (`!o`, `!b`, …) arrives right after the first bytes of an Improv frame whose remainder is still on its way
- **THEN** the remaining bytes are delivered to the Improv parser and the frame completes; the CLI command is served afterwards only if it is real input

#### Scenario: CLI command with no frame in progress

- **WHEN** a CLI command arrives with no Improv frame in progress
- **THEN** the CLI runs it with its current behavior, including grouping of trailing characters and flushing of pending garbage

### Requirement: A truncated frame does not degrade serial input

The station SHALL discard the partial state of an Improv frame after a period of inactivity with no new bytes, and SHALL interpret serial input normally again.

#### Scenario: Frame abandoned mid-transfer

- **WHEN** an Improv frame is left incomplete (the sender disappears mid-transfer) and a CLI command arrives afterwards
- **THEN** the command runs as if the truncated frame had never existed

#### Scenario: Complete frame after a truncated one

- **WHEN** a complete Improv frame arrives after a truncated one
- **THEN** the station parses it and answers correctly

### Requirement: WiFi provisioning over serial works at any moment

The station SHALL accept the Improv WiFi credentials command at any moment of its life —right after boot, in configuration mode, or during normal operation— without special windows or a prior reset.

#### Scenario: Provisioning during normal operation

- **WHEN** the host sends WiFi credentials over serial with the board already booted and running its main loop
- **THEN** the board joins the network, persists the credentials in its configuration and answers with its local URL, exactly as if the frame had arrived in one piece
