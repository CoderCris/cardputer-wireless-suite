# Roadmap de hitos

Cada hito produce una **herramienta autónoma** y **reutiliza/extiende** las
anteriores. La conexión entre hitos es explícita: el sniffer del hito 3 usa el
display del hito 1 y el logging del hito 2.

Los niveles se refieren a la [escalera de abstracción](02-abstraction-ladder.md).

El roadmap tiene **dos ejes**: el de **herramientas** (hitos 1-8, abajo) y el de
**plataforma** (store, navegación, enlace inter-MCU), que no avanza por hitos
propios sino cruzando con los anteriores. Ver [eje de plataforma](#eje-de-plataforma)
y [`05-vision.md`](05-vision.md).

| # | Herramienta | Introduce | Nivel | Vínculo blue team |
|---|-------------|-----------|-------|-------------------|
| 1 | Terminal interactiva | Display SPI + teclado matricial | 1 | Patrón entrada→proceso→salida |
| 2 | Logger a MicroSD | Filesystem + init manual de hardware (quitar M5Unified) | 1→2 | Persistencia de datos capturados |
| 3 | Sniffer WiFi (promiscuous) | `esp_wifi` modo promiscuo, callbacks | 2 | Captura 802.11 |
| 4 | Decodificador/replay IR | Timing de protocolo, RMT | 2→4 | Captura y replay de señales |
| 5 | Scanner BLE | Análisis de advertisement packets | 2 | Recon BLE |
| 6 | Reescritura WiFi en ESP-IDF | Transición Arduino → ESP-IDF puro | 2→3 | Misma herramienta, otro nivel |
| 7 | Side-channel de audio I2S | Análisis de señal (experimental) | 3 | Side-channel |
| 8 | Navegador del grafo | Navegación sobre el store compartido | mixto | Correlación de hallazgos |

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

Hito 8 (navegador) ── navega el store que 2..7 han ido poblando
```

## Notas por hito

- **Hito 1 — Terminal**: base de todo. Entrada por teclado, eco a display. Es el
  "hola mundo" del ciclo editar→compilar→flashear→observar. Diario:
  [`journal/hito-01-terminal.md`](journal/hito-01-terminal.md).
- **Hito 2 — Logger SD**: primer contacto con quitar M5Unified. Display y SD no
  comparten pines, pero agotan entre los dos los controladores SPI de uso general
  del S3 (GP-SPI3 el display, GP-SPI2 la SD). La lección es elegir y configurar un
  controlador sabiendo que ya no queda otro libre, ver
  [`01-hardware/spi-pads-controladores.md`](01-hardware/spi-pads-controladores.md).
  (Una versión anterior hablaba de un conflicto `GPIO12` que resultó no existir.)
  **Desde aquí
  entra en vigor el contrato de salida**: las herramientas emiten registros
  tipados al store, no imprimen a pantalla como salida primaria
  ([`06-model/data-model.md`](06-model/data-model.md)).
- **Hito 3 — Sniffer WiFi**: modo promiscuo de `esp_wifi`, callback de RX. Primer
  hito de captura de red real. Concepto clave: `promiscuous mode` (ver
  [glosario](glossary.md)).
- **Hito 4 — IR**: periférico **RMT** del ESP32-S3 para medir/generar pulsos con
  precisión de hardware. El Cardputer V1 solo tiene **emisor** IR (`GPIO44`): para
  capturar hace falta un receptor externo por el Grove.
- **Hito 5 — BLE**: parseo de advertisement packets. Recon pasivo.
- **Hito 6 — WiFi en ESP-IDF**: no es una herramienta nueva, es **bajar de nivel**
  el hito 3. Ejercicio puro de escalera de abstracción.
- **Hito 7 — Audio I2S**: experimental. Conflicto `GPIO43` (mic CLK ↔ speaker LRCK).
- **Hito 8 — Navegador**: **no es un menú de herramientas**. La UI lista *nodos*
  del store y ofrece las herramientas cuyo tipo de entrada casa con el nodo
  seleccionado (ver [`06-model/phases.md`](06-model/phases.md)). Incluye la gestión
  de recursos compartidos: qué periféricos pueden coexistir, ver
  [`01-hardware/pin-conflicts.md`](01-hardware/pin-conflicts.md).

## Eje de plataforma

No son hitos: son capas que se construyen **cruzando** con los hitos de
herramienta. Cada una tiene un *gate*: el hito antes del cual no se toca.

| Capa | Qué es | Gate | Documento |
|------|--------|------|-----------|
| Contrato de salida | Toda herramienta emite registros tipados | Hito 2 | [`06-model/data-model.md`](06-model/data-model.md) |
| Store persistente | Log append-only en SD + índice en RAM | Hito 2 | [`06-model/data-model.md`](06-model/data-model.md) |
| Grafo de fases | Sense→Identify→Assess→Interact→Evidence | Hito 5 | [`06-model/phases.md`](06-model/phases.md) |
| Navegador | UI sobre nodos, no sobre herramientas | Hito 8 | [`06-model/phases.md`](06-model/phases.md) |
| Enlace inter-MCU | Protocolo propio sobre I2C o UART | **Hito 6** | [`05-vision.md`](05-vision.md) |
| Coprocesadores | Display server, frontend RF, analizador lógico | Hito 6 | [`05-vision.md`](05-vision.md) |
| Carcasa / cyberdeck | Integración física y alimentación | Todo lo anterior | [`05-vision.md`](05-vision.md) |

### Por qué esos gates

El contrato de salida entra en el hito 2 porque es cuando aparece el primer
destino de datos que no es la pantalla. El enlace inter-MCU está gateado al hito 6
por la [escalera](02-abstraction-ladder.md): diseñar un protocolo sobre un flujo de
bytes crudo es **nivel 4**, y antes del hito 6 no se ha trabajado a nivel ESP-IDF.
Intentarlo antes no enseña protocolo — lleva a copiar una librería y volver al
nivel 1 con más cables.

**Regla**: ninguna capa de plataforma se aborda antes de su gate, por atractiva que
sea. La visión condiciona interfaces; no autoriza saltarse hitos.

## Regla de hardware

El usuario posee **solo el Cardputer base**. No se recomienda comprar módulos
hasta agotar el Cardputer para un objetivo concreto.

Orden de prioridad si un hito lo exige, con justificación:

1. **Segundo microcontrolador** (coprocesador). Es lo más barato y lo único que
   desbloquea un eje entero. Gateado al hito 6.
2. **Radio** (CC1101, nRF24L01) — amplía el espectro cubierto.
3. **NFC** (PN532) — I2C, convive con el Grove.
4. **CAN bus** o **analizador lógico**.

Aviso de recurso: el puerto Grove (`G1`/`G2`) es el **único** punto de expansión
externo del Cardputer. Gastarlo condiciona todo lo demás — ver
[`05-vision.md`](05-vision.md).
