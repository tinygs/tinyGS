# Design

## Context

Estado actual de la entrada serie (ver `proposal.md` para la motivación y la evidencia):

- `handleSerial()` (`tinyGS/tinyGS.ino:385`) mira `Serial.peek()`: si es `'I'` llama a `TinyGSImprov::handleImprovPacket()`, si no, a `handleRawSerial()`. La decisión se toma por pasada, no por frame.
- `TinyGSImprov::handleImprovPacket()` (`tinyGS/src/Improv/tinygs_improv.cpp:213`) drena `while (Serial.available() > 0)`: se sale en cuanto se pilla al emisor (el UART entrega ~1 byte/87 µs), dejando frames a medias. Conserva `improvBufferPosition` entre llamadas (privado, sin timeout) y descarta bytes inválidos uno a uno.
- `handleRawSerial()` (`tinyGS/tinyGS.ino:398`) consume un byte, espera 500 ms (`configManager.delay`) para agrupar y hace `while (Serial.available()) Serial.read();` sin resetear el estado del parser.
- El parser (`parse_improv_serial_byte` de `improv/Improv@^1.2.4`) ya es incremental y tolerante a ruido entre frames; el problema es que solo recibe bytes cuando el drenaje está activo.
- Sin infraestructura de pruebas unitarias en firmware (`test/` solo contiene un README): la verificación es sobre hardware con el banco HIL.

## Goals / Non-Goals

**Goals:**

- Frames Improv completos con cualquier patrón de llegada (repartidos entre pasadas de `loop()`, con huecos inter-byte), con cambio mínimo en el binario.
- Comportamiento observable de la CLI y del protocolo Improv sin cambios (mismos comandos, mismo retardo de agrupación, mismos frames).
- Resincronización garantizada: un frame truncado no deja la entrada "enganchada" al parser.

**Non-Goals:**

- Cambiar el formato o la semántica del protocolo Improv, ni añadir comandos.
- Repensar la CLI de mantenimiento (solo dejar de interferir con el parser).
- Pruebas unitarias on-target (requerirían un build de test que no certifica el binario; la verificación es HIL).
- Retirar el workaround de la ventana de arranque del banco HIL (queda como follow-up de `hardware-integration-tests`, que debe seguir funcionando con firmware antiguo en migraciones).

## Decisions

1. **Enrutar por "frame en curso", además de por el byte `'I'`**: `handleSerial()` sigue alimentando al parser mientras hay un frame a medias, aunque el siguiente byte no inicie frame. El estado del parser se consulta con un predicado público (`isParsing()`), manteniendo `improvBufferPosition` encapsulado. Alternativa descartada: tolerancia a huecos solo dentro del drenaje —sigue perdiendo frames si el hueco supera el margen y no impide que la CLI vacíe el buffer—.

2. **Timeout de frame parcial (resincronización)**: si hay un frame a medias y no llegan bytes durante un tiempo (del orden de 2 s), el parser descarta su estado parcial. Cubre al emisor que desaparece a mitad de envío, que hoy dejaría la entrada capturada por el parser para siempre. Alternativa descartada: resetear el parser solo cuando la CLI consume bytes —no cubre ese caso y acopla ambos consumidores—.

3. **`handleRawSerial()` conserva su flujo tal cual (agrupado de 500 ms + limpieza de basura)**: con la decisión 1, sus bytes ya nunca son de un frame en curso y su limpieza solo se come entrada huérfana. Alternativa descartada: eliminar la limpieza del buffer —cambiaría el comportamiento actual de la CLI, fuera de alcance—.

4. **Verificación de extremo a extremo con el banco HIL sobre hardware real**: provisionado WiFi en caliente (sin ventana de arranque), CLI operativa (`!o`, `!b`) con e sin tráfico Improv, frame truncado seguido de comando de CLI y de frame completo. Alternativa descartada: Unity on-target (sin infraestructura y no certificaría el binario de release).

## Risks / Trade-offs

- [El estado "frame en curso" podría capturar entrada de CLI si un frame se trunca] → acotado por el timeout de la decisión 2 (~2 s máximo de captura).
- [El agrupamiento de la CLI (500 ms) puede solaparse con el inicio de un frame Improv] → esos bytes quedan en el buffer y el parser los recibe al volver a enrutarse; el caso límite lo absorbe el timeout.
- [Se toca el binario que se certifica] → cambio localizado en el despacho de entrada, sin efecto en radio/MQTT/config; el banco HIL lo valida de extremo a extremo antes de salir en una release.
- [Comportamiento distinto entre firmware con y sin fix durante las migraciones probadas por HIL] → el workaround del banco (ventana de arranque) funciona con ambos y se retira después.

## Migration Plan

Sale con la release normal; no toca la configuración persistida, así que rollback = revert del commit. El banco HIL mantiene su workaround hasta verificar el fix en las versiones que migra.

## Open Questions

- El timeout exacto del frame parcial (2 s propuesto) se ajustará durante la verificación sobre hardware; no cambia specs, enfoque ni tareas.
