# Tasks

## 1. Reensamblado de frames Improv

- [ ] 1.1 Exponer el estado del parser con un predicado público manteniendo el encapsulamiento (`isParsing()`) y enrutar en `handleSerial()` los bytes de continuidad de un frame en curso al parser, además del byte `'I'`; verificar sobre la placa de referencia que un `WIFI_SETTINGS` de ~50 bytes enviado a 115200 bps con la placa en su bucle normal (llegada repartida entre pasadas) se parsea completo y la placa responde con estado y URL local
- [ ] 1.2 Implementar el timeout de frame parcial (el parser descarta su estado a medias tras ~2 s sin bytes) y verificar que un frame truncado seguido de un comando de CLI la ejecuta con normalidad y que un frame completo enviado después del truncado se parsea y responde

## 2. Convivencia con la CLI de mantenimiento

- [ ] 2.1 Verificar sobre hardware que la CLI no consume bytes de un frame Improv en curso (comando de CLI justo tras el inicio de un frame: el frame se completa y solo se atiende la CLI si hay entrada real) y que la CLI aislada conserva su comportamiento actual (`!o`, `!b`, con su agrupación y limpieza de basura)

## 3. Verificación de extremo a extremo del provisionado

- [ ] 3.1 Provisionar el WiFi por Improv con la placa ya arrancada y en su bucle normal —sin reinicio previo ni ventana de arranque— y verificar que se conecta, persiste las credenciales y responde con su URL local (banco HIL de la rama `feature/hardware-integration-tests`, o sonda de intercambio equivalente)
- [ ] 3.2 Ejecutar el recorrido completo de provisionado del banco HIL (`tests/hil/tests/test_wifi_setup.py`, incluida la persistencia tras reinicio) sobre la placa de referencia con este firmware y verificar que pasa en verde con el workaround de la ventana de arranque sin usarse
