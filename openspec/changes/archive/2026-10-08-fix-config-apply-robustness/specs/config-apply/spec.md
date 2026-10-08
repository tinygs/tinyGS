# Spec Delta

## Purpose

Applying the station's configuration —saving it from the web console and provisioning WiFi credentials over Improv-serial— must complete cleanly: the save confirmation reaches the client before any restart, and credentials are applied even while a connection attempt is in flight.

## ADDED Requirements

### Requirement: The save confirmation is delivered before the restart

When a configuration save requires restarting the station —for example because the station name changed— the station SHALL deliver the save confirmation to the client and SHALL restart afterwards to apply the configuration.

#### Scenario: Saving a new station name from the console

- **WHEN** the configuration form is submitted with a station name different from the current one
- **THEN** the browser receives the confirmation page and, shortly after, the station restarts with the new configuration applied

#### Scenario: Saving without a restart

- **WHEN** the configuration form is submitted without changes that force a restart
- **THEN** the confirmation page is delivered as usual and the station keeps running

### Requirement: WiFi credentials are applied even with a connection attempt in flight

When the Improv WiFi credentials command arrives while the station is already attempting to connect —for example right after boot— the station SHALL apply the received credentials; if they are valid, it SHALL join the network and answer with its local URL.

#### Scenario: Provisioning while the station is connecting

- **WHEN** credentials are sent over serial while the station is still connecting with its stored ones
- **THEN** the new credentials are applied, the station joins the network and answers with its local URL

#### Scenario: Invalid credentials

- **WHEN** the credentials received over Improv cannot connect to the network
- **THEN** the station answers with the corresponding connection error, as it does today
