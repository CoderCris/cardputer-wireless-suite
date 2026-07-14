# Referencias externas

Fuentes autoritativas. El **ESP32-S3 Technical Reference Manual (TRM)** es la
referencia principal para todo lo que sea registros y periféricos.

> ⚠️ **Sobre los números de sección del TRM**: en el resto de la documentación se
> citan capítulos del TRM por su **título** (p. ej. *IO MUX and GPIO Matrix*,
> *RMT*, *SPI Controller*). Los títulos son estables; **los números de sección
> cambian entre revisiones** del manual. Antes de citar un "§X.Y" en tu código,
> confírmalo contra la revisión concreta del PDF que tengas abierto. No des por
> bueno un número que no hayas visto en tu copia.

## Silicio y periféricos

- **ESP32-S3 Technical Reference Manual** (Espressif) — registros y periféricos:
  GPIO Matrix, SPI, I2C, I2S, RMT, USB, RTC.
  https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
- **ESP32-S3 Datasheet** (Espressif) — características eléctricas, pinout del SoC.
  https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

## Framework

- **ESP-IDF Programming Guide** — API de nivel 2 (HAL/drivers):
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
  - GPIO: `api-reference/peripherals/gpio`
  - SPI master: `api-reference/peripherals/spi_master`
  - I2C: `api-reference/peripherals/i2c`
  - I2S: `api-reference/peripherals/i2s`
  - RMT: `api-reference/peripherals/rmt`
  - WiFi (promiscuo): `api-reference/network/esp_wifi`
- **Arduino-ESP32** — capa Arduino sobre ESP-IDF:
  https://docs.espressif.com/projects/arduino-esp32/en/latest/

## M5Stack / Cardputer

- **M5Stack Cardputer — documentación oficial** (pinout, esquemático, modo de
  flasheo): https://docs.m5stack.com/en/core/Cardputer
- **M5Unified** (librería): https://github.com/m5stack/M5Unified
- **M5Cardputer** (librería): https://github.com/m5stack/M5Cardputer
- **M5GFX** (motor de display que usa M5Unified):
  https://github.com/m5stack/M5GFX

## Componentes

- **Sitronix ST7789V2** — datasheet del controlador de display (comandos de init,
  timing SPI). Buscar "ST7789V2 datasheet" (Sitronix).
- **AXP2101** — PMIC, registros de energía y carga. Datasheet de X-Powers.
- **SPM1423** — micrófono PDM/I2S.
- **NS4168** — amplificador de audio I2S (altavoz).

## PlatformIO

- **PlatformIO — plataforma espressif32**:
  https://docs.platformio.org/en/latest/platforms/espressif32.html
- **PlatformIO — comandos `pio run` / `device monitor`**:
  https://docs.platformio.org/en/latest/core/userguide/

## Mantenimiento

Cuando uses una fuente que resuelva una duda real, añádela aquí con una nota de
para qué sirvió. Prioriza fuentes primarias (datasheet, TRM, doc oficial) sobre
blogs.
