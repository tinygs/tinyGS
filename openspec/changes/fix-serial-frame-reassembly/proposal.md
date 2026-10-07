# Proposal

## Why

El firmware no puede recibir frames Improv largos: `handleSerial()` (`tinyGS.ino`) solo reensambla un frame dentro de una pasada de `loop()` —enruta al parser únicamente si el siguiente byte disponible es `'I'`— y los bytes sueltos de continuidad caen en `handleRawSerial()`, que lee un byte, espera 500 ms y **vacía el buffer serie**, sin resetear el estado del parser. Los frames cortos (`GET_CURRENT_STATE`, 12 B) caben en una pasada y responden; el frame de credenciales (`WIFI_SETTINGS`, 49 B ≈ 4 ms a 115200) llega partido y muere en silencio: el provisionado WiFi por Improv-serial es imposible fuera del workaround de la ventana de arranque que usa el banco HIL. Evidencia: sonda cruda sobre placa real — `GET_CURRENT_STATE` responde `STATE_AUTHORIZED`; `WIFI_SETTINGS` no obtiene respuesta alguna en 75 s.

## What Changes

- **Reensamblado de frames Improv independiente de la cadencia de llegada**: el despacho serie continúa alimentando al parser mientras hay un frame en curso, aunque los bytes lleguen repartidos entre pasadas de `loop()` o con huecos.
- **No interferencia entre consumidores de la entrada serie**: la CLI de mantenimiento (`!e`, `!b`, `!p`, `!w`, `!o`) solo consume entrada que no pertenece a un frame Improv en curso y ya no destruye frames por su vaciado del buffer.
- **Resincronización ante frames truncados**: un frame Improv que se queda a medias (host caído a mitad de envío) no degrada ni la CLI ni los frames siguientes; el parser descarta el estado parcial tras un tiempo de inactividad.
- El provisionado WiFi por Improv-serial funciona **en cualquier momento** de la vida del dispositivo (el banco HIL podrá retirar su workaround de la ventana de arranque cuando pruebe firmware con este fix; mientras tanto lo conserva por compatibilidad con versiones antiguas en migraciones).

## Capabilities

### New Capabilities

- `serial-input-dispatch`: despacho de la entrada serie USB entre el protocolo Improv y la CLI de mantenimiento — reensamblado de frames con llegada arbitraria, separación entre consumidores y resincronización ante entradas truncadas.

### Modified Capabilities

<!-- Ninguna: `mqtt-remote-commands` no cambia (es despacho MQTT, no serie). -->

## Impact

- `tinyGS/tinyGS.ino`: `handleSerial()` y `handleRawSerial()` (enrutado y vaciado del buffer).
- `tinyGS/src/Improv/tinygs_improv.{h,cpp}`: `handleImprovPacket()` y el estado del parser (`improvBufferPosition`), hoy privado y sin timeout.
- Sin cambios en comandos de la CLI ni en comandos/frames Improv existentes (formato y semántica intactos); sin tocar la configuración persistida.
- Verificación sobre hardware con el banco HIL (`tests/hil/tests/test_wifi_setup.py` y sonda de intercambio Improv), sin infraestructura de pruebas unitarias en firmware (`test/` no la tiene).
