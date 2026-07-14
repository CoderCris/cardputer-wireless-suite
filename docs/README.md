# Documentación — Cardputer Wireless Suite

Base de conocimiento propia para desarrollar sobre el **M5Stack Cardputer V1**
(ESP32-S3, módulo StampS3). El proyecto se construye sobre hardware, lenguaje y
librerías que no controlamos; esta carpeta es la referencia para poder desarrollar
de forma autónoma sin depender de tener cada datasheet abierto.

Es una actividad de **blue team**: el objetivo es entender a bajo nivel cómo
funcionan estas técnicas (WiFi/BLE sniffing, IR, buses) para saber detectarlas y
defenderse.

> El contrato de comportamiento del asistente (mentoría) **no** vive aquí, sino
> en `CLAUDE.md` en la raíz del repo.

## Por dónde empezar

Lee en orden numérico. Los archivos `04-protocols/` y `journal/` crecen a medida
que avanzas por el roadmap.

| Archivo | Qué contiene | Cuándo leerlo |
|---------|--------------|---------------|
| [`00-toolchain.md`](00-toolchain.md) | PlatformIO, compilar/flashear/monitor, particiones, USB-CDC | Antes de nada |
| [`01-hardware/cardputer-map.md`](01-hardware/cardputer-map.md) | Pinout completo, periféricos, energía | Al empezar cualquier hito |
| [`01-hardware/pin-conflicts.md`](01-hardware/pin-conflicts.md) | Pines compartidos y sus conflictos | Antes de usar SD, mic o IR |
| [`01-hardware/buses.md`](01-hardware/buses.md) | Qué cuelga de SPI / I2C / I2S | Al tocar cualquier periférico |
| [`02-abstraction-ladder.md`](02-abstraction-ladder.md) | Los 4 niveles Arduino→registros | Para decidir cómo abordar cada tarea |
| [`03-libraries/m5unified.md`](03-libraries/m5unified.md) | API de M5Unified/M5Cardputer y qué abstrae | Nivel 1 |
| [`04-protocols/`](04-protocols/) | Notas por bus/protocolo (SPI, teclado, WiFi, IR...) | Al bajar de nivel |
| [`roadmap.md`](roadmap.md) | Los 8 hitos y sus dependencias | Para planificar |
| [`journal/`](journal/) | Diario de decisiones y aprendizajes por hito | Al terminar cada hito |
| [`glossary.md`](glossary.md) | Términos (register, promiscuous, RMT...) | Referencia rápida |
| [`references.md`](references.md) | TRM, ESP-IDF, datasheets, enlaces | Cuando necesites la fuente |

## Cómo mantener esta carpeta

- **Una sola fuente de verdad por dato.** El pinout vive solo en `cardputer-map.md`;
  el resto enlaza, no copia.
- Al terminar un hito, escribe su entrada en `journal/` y crea/actualiza la nota
  de protocolo correspondiente en `04-protocols/`.
- Cita el TRM por sección concreta (§ + título), nunca "consulta la doc".
