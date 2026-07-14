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
- **PSRAM**: 8 MB (necesaria para buffers grandes; habilitada con
  `-DBOARD_HAS_PSRAM`, ver [`../00-toolchain.md`](../00-toolchain.md)).
- **Flash**: 16 MB (tabla de particiones `default_16MB.csv`).
- **USB**: USB-C nativo del ESP32-S3 (CDC/JTAG), sin chip UART externo.

## Pinout por bus/periférico

### SPI — Display ST7789V2 (240×135)

| Señal | GPIO | Rol |
|-------|------|-----|
| CS    | 12   | Chip select (seleccionar el display en el bus) |
| DC    | 34   | Data/Command (distingue byte de comando vs. de datos) |
| RST   | 33   | Reset del controlador |
| BL    | 38   | Backlight (retroiluminación) |
| SCLK  | 36   | Reloj SPI |
| MOSI  | 35   | Master Out Slave In (datos hacia el display) |

El display no tiene MISO cableado: es un periférico de solo escritura desde el
punto de vista habitual (write-only display).

### SPI — MicroSD

| Señal | GPIO | Rol |
|-------|------|-----|
| CS    | 14   | Chip select de la tarjeta |
| SCLK  | 40   | Reloj SPI |
| MISO  | 39   | Master In Slave Out (datos desde la SD) |
| MOSI  | 12   | Master Out Slave In (datos hacia la SD) |

⚠️ **`GPIO12` se comparte con el CS del display.** Ver
[`pin-conflicts.md`](pin-conflicts.md).

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
| Teclado matricial | varios | 56 teclas, escaneo por matriz (selección de columna + lectura de filas) |
| IR TX | 44 | Emisor infrarrojo |
| IR RX | 46 | Receptor infrarrojo (compartido con mic CLK) |
| LED de estado | 21 | LED direccionable/simple |

El teclado no es un pin único: es una **matriz** que se escanea seleccionando una
columna y leyendo el estado de las filas. El detalle del mecanismo va en
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
