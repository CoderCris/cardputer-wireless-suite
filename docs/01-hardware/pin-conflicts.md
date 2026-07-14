# Conflictos de pines compartidos

El Cardputer reutiliza pines entre periféricos. Un pin compartido no es un error
de diseño: significa que **no puedes usar los dos periféricos que lo comparten al
mismo tiempo** sin reconfigurar el pin. Señala esto siempre que sea relevante para
el hito en curso.

Los números de pin son los de [`cardputer-map.md`](cardputer-map.md).

## Los tres conflictos conocidos

### `GPIO12` — Display CS ↔ SD MOSI

El chip select del display y el MOSI de la MicroSD comparten pin. En la práctica,
si vas a usar display y SD a la vez, tienes que ser consciente de que una
operación mal secuenciada puede dejar al display "escuchando" datos destinados a
la SD. Con M5Unified esto está resuelto por la librería; **cuando bajes a manejar
los buses tú mismo (nivel 2+), la gestión del CS es responsabilidad tuya**.

Relevante en: **hito 2 (logger a MicroSD)**, porque ahí conviven display y SD.

### `GPIO43` — Mic DATA ↔ Speaker LRCK

Micrófono y altavoz comparten una línea I2S. No puedes capturar audio y
reproducir simultáneamente sobre esa configuración sin multiplexar. Para la
mayoría de casos (grabar *o* reproducir, no las dos cosas a la vez) no es
problema, pero condiciona cualquier idea de "escuchar y responder en tiempo real".

Relevante en: **hito 7 (side-channel de audio I2S)**.

### `GPIO46` — IR RX ↔ Mic CLK

El receptor de infrarrojos y el reloj del micrófono comparten pin. **No puedes
usar micrófono e IR RX simultáneamente.** Si tu herramienta captura IR, el mic
queda inutilizable mientras tanto, y viceversa.

Relevante en: **hito 4 (decodificador/replay IR)** si en algún momento coincide
con audio.

## Cómo razonar sobre esto al programar

Un pin es un recurso físico único. Configurarlo para un periférico (dirección,
función en la matriz GPIO, pull-up/down) sobrescribe la configuración anterior. En
nivel Arduino la librería te oculta esto; en nivel 2+ (ESP-IDF / registros) eres
tú quien llama a `gpio_config()` o escribe la matriz GPIO, así que un conflicto se
manifiesta como "el periférico B dejó de funcionar cuando inicialicé A".

Antes de inicializar un periférico que use un pin compartido, pregúntate: ¿está el
otro periférico activo ahora mismo? Si sí, necesitas des-inicializarlo o
multiplexar en el tiempo.

## Tabla resumen

| GPIO | Periférico A | Periférico B | Regla |
|------|--------------|--------------|-------|
| 12 | Display CS | SD MOSI | Gestionar CS con cuidado al usar ambos |
| 43 | Mic DATA | Speaker LRCK | No capturar y reproducir a la vez |
| 46 | IR RX | Mic CLK | No usar micro e IR RX simultáneamente |
