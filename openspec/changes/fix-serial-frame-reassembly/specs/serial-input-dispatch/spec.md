# Spec Delta

## Purpose

Despachar la entrada serie USB de la estación entre el protocolo Improv y la CLI de mantenimiento: recibir los frames completos aunque lleguen repartidos en el tiempo, impedir que un consumidor destruya la entrada del otro y resincronizar ante entradas truncadas.

## ADDED Requirements

### Requirement: Los frames del protocolo Improv se reciben completos con llegada arbitraria

La estación SHALL aceptar frames del protocolo Improv por el puerto serie USB aunque sus bytes lleguen repartidos en el tiempo —entre pasadas de su bucle principal o con huecos entre bytes— y SHALL responder con el frame de resultado correspondiente, con independencia del tamaño del frame.

#### Scenario: Frame largo llega partido

- **WHEN** se transmite un frame Improv de credenciales WiFi (~50 bytes a 115200 bps) cuyos bytes se reparten entre varias pasadas del bucle principal
- **THEN** la estación lo parsea completo y responde con su estado y su URL local tras provisionar, o con el error correspondiente

#### Scenario: Frame corto sin regresión

- **WHEN** se envía una consulta de estado Improv (frame de ~12 bytes)
- **THEN** la estación responde con su estado actual, como hasta ahora

### Requirement: La CLI de mantenimiento no interrumpe frames Improv en curso

La entrada serie SHALL distribuirse entre el protocolo Improv y la CLI de mantenimiento sin que un consumidor destruya los bytes del otro: mientras hay un frame Improv en curso, esos bytes SHALL NOT tratarse como comandos de la CLI ni descartarse por su limpieza de entrada pendiente.

#### Scenario: Comando de CLI mientras hay un frame en curso

- **WHEN** llega un comando de la CLI (`!o`, `!b`, …) inmediatamente después de los primeros bytes de un frame Improv cuyo resto está en camino
- **THEN** esos bytes restantes se entregan al parser Improv y el frame se completa; el comando de la CLI se atiende a continuación si es entrada real

#### Scenario: Comando de CLI sin frame en curso

- **WHEN** llega un comando de la CLI sin frame Improv en curso
- **THEN** la CLI lo ejecuta con su comportamiento actual, incluida la agrupación de caracteres posteriores y el descarte de basura pendiente

### Requirement: Un frame truncado no degrada la entrada serie

La estación SHALL descartar el estado parcial de un frame Improv tras un tiempo de inactividad sin nuevos bytes, y SHALL volver a interpretar la entrada serie con normalidad.

#### Scenario: Frame abandonado a medias

- **WHEN** un frame Improv se queda incompleto (el emisor desaparece a mitad de envío) y después llega un comando de la CLI
- **THEN** el comando se ejecuta como si el frame truncado no hubiera existido

#### Scenario: Frame completo tras uno truncado

- **WHEN** tras un frame truncado llega un frame Improv completo
- **THEN** la estación lo parsea y responde correctamente

### Requirement: El provisionado WiFi por serie funciona en cualquier momento

La estación SHALL aceptar el comando Improv de credenciales WiFi en cualquier momento de su vida —recién arrancada, en modo de configuración o en operación normal— sin necesidad de ventanas especiales ni de reinicio previo.

#### Scenario: Provisionado durante la operación normal

- **WHEN** el host envía credenciales WiFi por serie con la placa ya arrancada y ejecutando su bucle principal
- **THEN** la placa se conecta a la red, persiste las credenciales en su configuración y responde con su URL local, igual que si el frame hubiera llegado de golpe
