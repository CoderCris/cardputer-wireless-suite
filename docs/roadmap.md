# Roadmap de hitos

Cada hito produce una **herramienta autónoma** y **reutiliza/extiende** las
anteriores. La conexión entre hitos es explícita: el sniffer del hito 3 usa el
display del hito 1 y el logging del hito 2.

Los niveles se refieren a la [escalera de abstracción](02-abstraction-ladder.md).

| # | Herramienta | Introduce | Nivel | Vínculo blue team |
|---|-------------|-----------|-------|-------------------|
| 1 | Terminal interactiva | Display SPI + teclado matricial | 1 | Patrón entrada→proceso→salida |
| 2 | Logger a MicroSD | Filesystem + init manual de hardware (quitar M5Unified) | 1→2 | Persistencia de datos capturados |
| 3 | Sniffer WiFi (promiscuous) | `esp_wifi` modo promiscuo, callbacks | 2 | Captura 802.11 |
| 4 | Decodificador/replay IR | Timing de protocolo, RMT | 2→4 | Captura y replay de señales |
| 5 | Scanner BLE | Análisis de advertisement packets | 2 | Recon BLE |
| 6 | Reescritura WiFi en ESP-IDF | Transición Arduino → ESP-IDF puro | 2→3 | Misma herramienta, otro nivel |
| 7 | Side-channel de audio I2S | Análisis de señal (experimental) | 3 | Side-channel |
| 8 | Multitool integrada | Menú + módulos seleccionables | mixto | Integración final |

## Dependencias entre hitos

```
Hito 1 (display + teclado)
   │
   ├─> Hito 2 (logger SD) ── reutiliza display, añade FS y init manual
   │        │
   │        ├─> Hito 3 (sniffer WiFi) ── display + logging de paquetes
   │        │        │
   │        │        └─> Hito 6 (WiFi en ESP-IDF) ── baja de nivel el hito 3
   │        │
   │        ├─> Hito 4 (IR) ── display + logging de señales
   │        │
   │        └─> Hito 5 (BLE) ── display + logging de advertisements
   │
   └─> Hito 7 (audio I2S) ── side-channel, más independiente

Hito 8 (multitool) ── integra 1..7 bajo un menú
```

## Notas por hito

- **Hito 1 — Terminal**: base de todo. Entrada por teclado, eco a display. Es el
  "hola mundo" del ciclo editar→compilar→flashear→observar. Diario:
  [`journal/hito-01-terminal.md`](journal/hito-01-terminal.md).
- **Hito 2 — Logger SD**: primer contacto con quitar M5Unified. Aquí aparece el
  conflicto `GPIO12` (display CS ↔ SD MOSI), ver
  [`01-hardware/pin-conflicts.md`](01-hardware/pin-conflicts.md).
- **Hito 3 — Sniffer WiFi**: modo promiscuo de `esp_wifi`, callback de RX. Primer
  hito de captura de red real. Concepto clave: `promiscuous mode` (ver
  [glosario](glossary.md)).
- **Hito 4 — IR**: periférico **RMT** del ESP32-S3 para medir/generar pulsos con
  precisión de hardware. Ojo al conflicto `GPIO46` (IR RX ↔ mic CLK).
- **Hito 5 — BLE**: parseo de advertisement packets. Recon pasivo.
- **Hito 6 — WiFi en ESP-IDF**: no es una herramienta nueva, es **bajar de nivel**
  el hito 3. Ejercicio puro de escalera de abstracción.
- **Hito 7 — Audio I2S**: experimental. Conflictos `GPIO43` y `GPIO46`.
- **Hito 8 — Multitool**: menú que integra los módulos. Diseño de navegación y
  gestión de recursos compartidos (¿qué periféricos coexisten?).

## Regla de hardware

El usuario posee **solo el Cardputer base**. No se recomienda comprar módulos
hasta agotar el Cardputer para un objetivo concreto. Si un hito lo exige, se
prioriza (con justificación): radio (CC1101, nRF24L01), NFC (PN532), CAN bus o
logic analyzer.
