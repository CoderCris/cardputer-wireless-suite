# Glosario

Términos que aparecen en el proyecto. Se mantienen en inglés cuando son estándar
del campo; la definición va en español.

## Hardware y buses

**register** — Posición de memoria dentro de un periférico que controla o refleja
su estado. Escribir un valor en un registro configura o acciona el hardware. El
TRM del ESP32-S3 documenta cada registro por dirección y función.

**GPIO matrix / IO MUX** — Sistema del ESP32-S3 que enruta señales de periféricos
internos a pines físicos casi arbitrarios. Por eso el mismo periférico puede salir
por pines distintos según la placa. TRM: *IO MUX and GPIO Matrix*.

**CS (chip select)** — Línea que selecciona a qué esclavo del bus SPI se dirige el
maestro. Activa (normalmente en bajo) durante la transacción.

**DC (data/command)** — Línea del display que indica si el byte enviado es un
comando (bajo) o un dato/píxel (alto).

**MOSI / MISO** — Master Out Slave In / Master In Slave Out: las dos líneas
direccionales de datos en SPI.

**RGB565** — Formato de color de 16 bits por píxel (5 bits rojo, 6 verde, 5 azul)
que usa el display ST7789.

**PSRAM** — RAM externa (8 MB en el StampS3) para buffers grandes que no caben en
la SRAM interna del SoC.

**PMIC (AXP2101)** — Power Management IC: gestiona rieles de alimentación y la
carga de la batería LiPo. Se controla por I2C.

## Firmware y sistema

**HAL (Hardware Abstraction Layer)** — Capa de ESP-IDF que expone el hardware con
funciones (`gpio_config`, `spi_bus_initialize`) en vez de escrituras a registros.

**FreeRTOS task** — Hilo de ejecución del sistema operativo de tiempo real que
corre el ESP32-S3. Permite concurrencia (p. ej. capturar y pintar a la vez).

**interrupt (ISR)** — Rutina que el hardware dispara ante un evento (llega un
paquete, cambia un pin), interrumpiendo el flujo normal. Debe ser corta.

**RMT** — Periférico del ESP32-S3 (*Remote Control*) que genera y mide secuencias
de pulsos con precisión de hardware. Es la vía correcta para IR (hito 4). TRM:
*RMT (Remote Control Transceiver)*.

**CDC (USB Communications Device Class)** — Clase USB que hace que el S3 aparezca
como puerto serie. Es lo que transporta `Serial.print()` por el USB-C.

**bit-banging** — Implementar un protocolo moviendo los pines a mano (en software)
en vez de usar el periférico dedicado. Máximo control del timing, nivel 4.

**debounce** — Filtrado temporal que descarta los rebotes de un pulsador mecánico
para no leer una pulsación como varias. Aviso: el driver del teclado del Cardputer
**no lo implementa**; ver [`04-protocols/gpio-keyboard.md`](04-protocols/gpio-keyboard.md).

**ghosting** — Tecla fantasma que aparece en una matriz sin diodos cuando se
pulsan tres teclas que forman tres esquinas de un rectángulo en la rejilla: la
corriente encuentra un camino de vuelta y la cuarta esquina se lee como pulsada.
Se evita con un diodo por tecla. Pendiente de comprobar en el Cardputer.

**pull-up / pull-down** — Resistencia que fija el nivel en reposo de un pin de
entrada para que no quede flotando (leyendo ruido). El ESP32-S3 las lleva
internas y se activan por software (`INPUT_PULLUP`). Con pull-up, reposo = 1 y
contacto a masa = 0.

**decodificador (demux) 3→8** — Chip que recibe un número binario de 3 bits por
tres líneas y activa exactamente una de sus 8 salidas. Permite gobernar 8 líneas
con 3 GPIO; es lo que hace posible el barrido del teclado del Cardputer.

## Ciberseguridad / blue team

**promiscuous mode** — Modo del WiFi en el que la radio entrega **todos** los
frames 802.11 que oye, no solo los dirigidos a este dispositivo. Base del sniffer
(hito 3). Interés blue team: entender qué es capturable para saber qué proteger.

**802.11** — Familia de estándares WiFi. Los frames de gestión (beacons, probe
requests) son visibles sin asociarse a la red.

**BLE advertisement packet** — Paquete que un dispositivo BLE emite periódicamente
para anunciarse. Legible de forma pasiva; base del recon BLE (hito 5).

**side-channel** — Fuga de información por un canal no previsto (consumo, tiempo,
sonido, EM). El hito 7 explora side-channels de audio.

**replay** — Volver a emitir una señal capturada (p. ej. IR) para reproducir su
efecto. Captura + replay es el patrón del hito 4.
