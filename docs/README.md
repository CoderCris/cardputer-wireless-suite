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
| [`01-hardware/kit-arduino.md`](01-hardware/kit-arduino.md) | Inventario del kit Arduino: qué es cada pieza, datos clave, niveles 5 V vs 3,3 V | Antes de cablear algo externo al Cardputer |
| [`02-abstraction-ladder.md`](02-abstraction-ladder.md) | Los 4 niveles Arduino→registros | Para decidir cómo abordar cada tarea |
| [`03-libraries/m5unified.md`](03-libraries/m5unified.md) | API de M5Unified/M5Cardputer y qué abstrae | Nivel 1 |
| [`03-libraries/fuentes-locales.md`](03-libraries/fuentes-locales.md) | Dónde viven en disco las librerías y el framework, y cómo leerlas | Cuando necesites una firma o una declaración |
| [`04-protocols/`](04-protocols/) | Notas por bus/protocolo (SPI, teclado, WiFi, IR...) | Al bajar de nivel |
| [`05-vision.md`](05-vision.md) | Objetivo cyberdeck: coprocesadores, enlace, carcasa | Para orientarse a largo plazo |
| [`06-model/data-model.md`](06-model/data-model.md) | Contrato del store compartido | **Antes del hito 2** |
| [`06-model/phases.md`](06-model/phases.md) | Fases Sense→Identify→Assess→Interact→Evidence | Al diseñar navegación |
| [`07-memory-model.md`](07-memory-model.md) | Pila, `.data`/`.bss`, heap: dónde vive cada variable y qué cuesta | Cuando necesites estado que sobreviva a `loop()` |
| [`08-arrays-y-cadenas.md`](08-arrays-y-cadenas.md) | Arrays fijos, terminador `'\0'`, `<cstring>`, fuera-por-uno y enteros sin signo | Al manipular cualquier buffer |
| [`roadmap.md`](roadmap.md) | Los 8 hitos, el eje de plataforma y sus gates | Para planificar |
| [`journal/`](journal/) | Diario de decisiones y aprendizajes por hito | Al terminar cada hito |
| [`deuda-conceptual.md`](deuda-conceptual.md) | Conceptos aparcados a propósito y cuándo retomarlos | Cuando te tropieces con uno |
| [`glossary.md`](glossary.md) | Términos (register, promiscuous, RMT...) | Referencia rápida |
| [`references.md`](references.md) | TRM, ESP-IDF, datasheets, enlaces | Cuando necesites la fuente |

## Cómo mantener esta carpeta

- **Una sola fuente de verdad por dato.** El pinout vive solo en `cardputer-map.md`;
  el resto enlaza, no copia.
- **Distingue lo leído de lo observado.** Lo que sale de leer código o datasheets
  se documenta como tal; lo que requiere flashear y mirar va como *pendiente de
  comprobar* hasta que lo compruebes.
- Al terminar un hito, escribe su entrada en `journal/` y crea/actualiza la nota
  de protocolo correspondiente en `04-protocols/`.
- **Las firmas y declaraciones se citan del código instalado**, no de memoria
  ni de un tutorial. Dónde está ese código:
  [`03-libraries/fuentes-locales.md`](03-libraries/fuentes-locales.md).
- Cita el TRM por sección concreta (§ + título), nunca "consulta la doc".
- La visión (`05-vision.md`) describe el destino; el `roadmap.md` manda sobre ella.
  Si se contradicen, gana el roadmap.
