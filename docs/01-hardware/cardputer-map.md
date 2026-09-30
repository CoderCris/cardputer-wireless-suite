# Mapa de hardware — M5Stack Cardputer V1

Fuente única de verdad del pinout y los periféricos. Cualquier otro documento que
necesite un número de pin **enlaza aquí**, no lo copia.

> Estos valores son los del Cardputer V1 con módulo StampS3. Antes de dar por
> bueno un pin para un caso nuevo, contrástalo con el esquemático oficial de
> M5Stack (enlace en [`../references.md`](../references.md)). Si observas en
> hardware un comportamiento que contradice esta tabla, gana el hardware: corrige
> aquí y anota el porqué.

## SoC y módulo

- **SoC**: ESP32-S3 (dual-core Xtensa LX7, WiFi 802.11 b/g/n + Bluetooth 5 LE).
- **Módulo**: M5Stack StampS3.
- **Chip**: ESP32-S3**FN8**. La `F` indica flash dentro del encapsulado y el `8`
  su tamaño; no lleva `R`, que es la letra de la PSRAM.
- **Flash**: 8 MB. Lo confirma `"maximum_size": 8388608` en
  `~/.platformio/platforms/espressif32/boards/m5stack-stamps3.json`. Tabla de
  particiones: `default_8MB.csv`.
- **PSRAM**: **probablemente ninguna** (leído, pendiente de observar). Esta tabla
  decía "8 MB", seguramente por confundirla con la flash. Hay dos indicios
  independientes. El primero es el nombre del chip. El segundo está en el propio
  pinout: una PSRAM octal ocupa en el S3 los pads 33 a 37, y el display usa
  exactamente 33, 34, 35, 36 y 37. Se confirma flasheando
  `ESP.getPsramSize()` (micro-tarea en
  [`../06-model/data-model.md`](../06-model/data-model.md)).
- **USB**: USB-C nativo del ESP32-S3 (CDC/JTAG), sin chip UART externo.

## Pinout por bus/periférico

### SPI — Display ST7789V2 (240×135)

Controlador: **GP-SPI3** (`SPI3_HOST`), elegido por M5GFX. Fuente:
`.pio/libdeps/cardputer/M5GFX/src/M5GFX.cpp`, rama de autodetección del
Cardputer (`bus_cfg.pin_* = ...`, `cfg.pin_cs = GPIO_NUM_37`).

| Señal | GPIO | Rol |
|-------|------|-----|
| CS    | 37   | Chip select (seleccionar el display en el bus) |
| DC    | 34   | Data/Command. No es una señal SPI: es un GPIO normal que M5GFX mueve a mano |
| RST   | 33   | Reset del controlador |
| BL    | 38   | Backlight (retroiluminación) |
| SCLK  | 36   | Reloj SPI |
| SDA   | 35   | Datos **bidireccionales** (SPI de 3 hilos) |

No hay MISO (`pin_miso = -1`), pero eso no hace al display de solo escritura:
M5GFX lo configura con `spi_3wire = true` y `readable = true`. En SPI de 3 hilos,
la misma línea de datos cambia de dirección para leer (half-duplex). Que la
lectura de la GRAM funcione de verdad en esta unidad está **pendiente de
observar**.

### SPI — MicroSD

Controlador: **GP-SPI2** (FSPI) si se usa el objeto `SPI` de Arduino, que en el S3
es `FSPI` = índice 0 = `DR_REG_SPI2_BASE` (`cores/esp32/esp32-hal-spi.h` y
`esp32-hal-spi.c`, en `~/.platformio/packages/framework-arduinoespressif32/`).
Pines tomados del ejemplo oficial
`.pio/libdeps/cardputer/M5Cardputer/examples/Basic/sdcard/sdcard.ino`.

| Señal | GPIO | Rol |
|-------|------|-----|
| CS    | 12   | Chip select de la tarjeta |
| SCLK  | 40   | Reloj SPI |
| MISO  | 39   | Master In Slave Out (datos desde la SD) |
| MOSI  | 14   | Master Out Slave In (datos hacia la SD) |

Display y SD **no comparten ningún pin**. Una versión anterior de esta tabla
tenía intercambiados el CS y el MOSI de la SD, y además el CS del display mal
puesto, lo que creaba un "conflicto `GPIO12`" que no existe. Pad, bus y
controlador se explican en [`spi-pads-controladores.md`](spi-pads-controladores.md).

### I2C — Grove y gestión de energía

| Señal | GPIO | Nota |
|-------|------|------|
| SDA   | 2    | Bus de datos I2C (Grove) |
| SCL   | 1    | Reloj I2C (Grove) |

El **AXP2101** (PMIC, gestión de energía y carga de la LiPo) cuelga del bus I2C
interno. Controla rieles de alimentación y la carga de batería.

### I2S — Audio

| Periférico | Señal | GPIO |
|------------|-------|------|
| Mic SPM1423 | DATA | 43 |
| Mic SPM1423 | CLK  | 46 |
| Altavoz NS4168 | BCLK | 41 |
| Altavoz NS4168 | LRCK | 43 |
| Altavoz NS4168 | DIN  | 42 |

⚠️ `GPIO43` (mic DATA / speaker LRCK) y `GPIO46` (mic CLK / IR RX) están
compartidos. Ver [`pin-conflicts.md`](pin-conflicts.md).

### GPIO directo

| Función | GPIO | Nota |
|---------|------|------|
| Teclado — selección | 8, 9, 11 | Salidas. Número binario de 3 bits → decodificador 3→8 |
| Teclado — lectura | 13, 15, 3, 4, 5, 6, 7 | Entradas con pull-up interno. Reposo 1, pulsada 0 |
| IR TX | 44 | Emisor infrarrojo |
| IR RX | 46 | Receptor infrarrojo (compartido con mic CLK) |
| LED de estado | 21 | LED direccionable/simple |

El teclado no es un pin único: es una **matriz** de 8 selecciones × 7 lecturas
(56 teclas) gobernada por 10 GPIO. Los tres pines de selección no son tres líneas:
llevan un número de 0 a 7 a un decodificador 3→8 que baja una sola salida. Pines
extraídos de `IOMatrix.h` del driver, **no del esquemático**. Mecanismo completo en
[`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md).

### Radio y alimentación

- **WiFi + Bluetooth 5 (LE)**: integrados en el ESP32-S3, sin pines externos
  (antena en el módulo).
- **USB-C**: USB nativo CDC/JTAG del S3 (programación + Serial).
- **Batería**: LiPo, carga gestionada por el AXP2101 vía I2C.

## Regla de oro

Nunca asumas un pin de memoria al escribir código: vuelve a esta tabla. Un número
mal copiado en un `gpio_config()` no da error de compilación, pero deja el
periférico muerto o, peor, provoca un conflicto en un pin compartido.
