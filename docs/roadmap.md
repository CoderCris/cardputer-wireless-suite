# Roadmap de hitos

Reescrito el 2026-11-16. El roadmap anterior ordenaba los hitos por la
[escalera de abstracción](02-abstraction-ladder.md): cada paso estaba bloqueado
hasta dominar un nivel. Desde el cambio de enfoque del 2026-11-06 el objetivo es
**tener la herramienta**, así que el orden lo fija ahora una sola cosa: **qué
tiene que existir y estar comprobado en el dispositivo para que la pieza
siguiente pueda construirse**. Una dependencia es técnica si responde a una de
estas cuatro preguntas: ¿necesita código que aún no existe?, ¿necesita un
recurso físico que otra pieza ocupa?, ¿necesita una medida que aún no se ha
tomado en hardware?, ¿necesita hardware que no se tiene?

Cada hito sigue produciendo una **herramienta autónoma**. La numeración se
conserva porque los diarios, las ramas y el resto de documentos la citan; por eso
hay huecos (2b y 6, retirados más abajo).

El estado de esta página es un resumen con fecha. El estado real de cada hito
vive en su diario de [`journal/`](journal/).

## Hitos de herramienta

| # | Herramienta | Periférico o protocolo | Depende de | Estado (2026-11-16) |
|---|-------------|------------------------|------------|---------------------|
| 1 | Terminal interactiva | Display ST7789 por GP-SPI3, teclado matricial por GPIO | Nada | Comprobado en hardware |
| 2 | Logger a MicroSD | SD en modo SPI por GP-SPI2, FatFs | Hito 1 | Montaje escrito; sin flashear con tarjeta; nunca se ha escrito un fichero |
| 3 | Sniffer WiFi | `esp_wifi` en modo promiscuo, radio de 2,4 GHz | Hito 1; el hito 2 solo para la pieza de persistencia | Pieza 1 compilada, sin flashear |
| 4a | Emisor IR | RMT sobre el LED de `GPIO44` | Hito 1; lanzador | Sin empezar |
| 4b | Captura y replay IR | RMT en recepción | 4a; **receptor IR externo por el Grove** (no se tiene) | Bloqueado por hardware |
| 5 | Scanner BLE | Advertising BLE, mismo radio que el WiFi | Lanzador con liberación de radio; medida de heap | Sin empezar |
| 7 | Side-channel de audio | Micrófono PDM por I2S (`GPIO43`/`GPIO46`) | Hito 1; lanzador | Sin empezar, experimental |
| 8 | Navegador | Vista sobre el store | Store con datos de al menos dos herramientas | Sin empezar |

## Capas de plataforma

No son herramientas: son el código común que las herramientas necesitan. Cada
una se construye **dentro del hito que primero la necesita**, no antes ni en un
hito propio.

| Capa | Qué es | Depende de | Se construye en |
|------|--------|------------|-----------------|
| Tubería de captura | Cola entre el productor (callback o interrupción) y una tarea consumidora que reparte a pantalla y SD | Nada | Hito 3, pieza 2 |
| Contrato de salida | Las herramientas emiten registros; la pantalla es un consumidor | Tubería | Hito 3, pieza 2 |
| Persistencia de sesión | Un fichero append-only por arranque en la SD | Hito 2 comprobado en hardware; tubería; formato decidido | Hito 3, pieza 5 |
| Lanzador y ciclo de vida | Cada herramienta tiene arranque, ciclo y parada, y declara qué recursos ocupa | Dos herramientas que arrancar | Antes de la segunda herramienta (4a o 5) |
| Proyecciones en RAM | Tablas acotadas de entidades (AP, estación, dispositivo BLE) derivadas de las observaciones | Tubería; medida de heap con el radio activo | Hito 3, pieza 3; se amplía en el 5 |
| Fases y valoración | Sense→Identify→Assess→Interact→Evidence ([`06-model/phases.md`](06-model/phases.md)) | Proyecciones de al menos dos fuentes | Hito 8 |
| Enlace inter-MCU | Protocolo propio sobre I2C o UART por el Grove | Segundo microcontrolador (no se tiene); Grove libre; una herramienta que lo exija | Sin fecha |
| Carcasa / cyberdeck | Integración física y alimentación | Enlace inter-MCU | Sin fecha |

## Grafo de dependencias

```
Hito 1 (display + teclado) ── comprobado
   │
   ├─> Hito 2 (SD: montar y escribir) ───────────────┐
   │                                                 │
   ├─> Hito 3 (sniffer WiFi)                         │
   │      pieza 1  contadores                        │
   │      pieza 2  tubería + contrato de salida      │
   │      pieza 3  cabecera 802.11 + tabla de AP     │
   │      pieza 4  salto de canal                    │
   │      pieza 5  persistencia de sesión <──────────┘
   │        │
   │        └─> Lanzador y ciclo de vida (parar el sniffer, soltar el radio)
   │               │
   │               ├─> Hito 4a (emisor IR) ──> Hito 4b (captura; receptor externo)
   │               ├─> Hito 5 (BLE; comparte radio con el hito 3)
   │               └─> Hito 7 (audio; experimental)
   │
   └─────────────> Hito 8 (navegador) <── store con datos de dos herramientas
```

## Por qué cada dependencia

**Hito 2 → persistencia.** Es la dependencia más atrasada. El código monta la
tarjeta pero nunca se ha flasheado con una dentro, y no existe ninguna escritura.
Antes de la pieza 5 del hito 3 hacen falta dos medidas en hardware: que el
montaje funciona y cuánto tarda una escritura en el peor caso (FatFs bloquea
mientras actualiza la tabla de asignación). Ese tiempo dimensiona la cola. Las
piezas 1 a 4 del sniffer no dependen de la SD.

**Tubería → todo lo que captura.** El callback promiscuo corre en la tarea del
driver WiFi, en el núcleo 0, y mientras dura no se entrega la trama siguiente.
Pintar es una transacción SPI larga y escribir en la SD puede bloquear cientos
de milisegundos: ninguna de las dos cosas puede hacerse ahí. La cola es, por
tanto, requisito de cualquier pieza que mire el contenido de una trama, y
resuelve además la lectura a medias de una MAC entre núcleos (diario del hito 3).
El mismo patrón sirve después para BLE (callback del host) y para IR
(interrupción del RMT).

**Medida de heap → proyecciones y BLE.** Sin PSRAM, la RAM libre tras arrancar
el WiFi es todo el presupuesto de la cola y de las tablas. Ese número lo da el
informe de arranque de la pieza 1 al flashear; hasta entonces los tamaños son
supuestos. El hito 5 necesita la misma medida con el host BLE cargado.

**Lanzador → hitos 4a, 5 y 7.** Hoy `main.cpp` arranca el sniffer sin
condiciones y no existe forma de pararlo. La segunda herramienta, sea cual sea,
exige elegir cuál corre. Para el hito 5 la exigencia es física: WiFi y BLE
comparten el único radio de 2,4 GHz, y dos capturas pasivas a la vez se reparten
el tiempo de antena y pierden tramas las dos. El lanzador lleva también la tabla
de recursos: radio, GP-SPI2, Grove y el pad `GPIO43` que comparten micrófono y
altavoz ([`01-hardware/pin-conflicts.md`](01-hardware/pin-conflicts.md)).

**Receptor IR → hito 4b.** El Cardputer V1 solo tiene emisor
([`01-hardware/cardputer-map.md`](01-hardware/cardputer-map.md)). Emitir códigos
conocidos (4a) no requiere nada más; capturar y reproducir (4b) requiere un
receptor por el Grove.

**Grove → 4b y coprocesador se excluyen.** El Grove (`G1`/`G2`) es el único
punto de expansión. Un receptor IR y un enlace a un coprocesador no caben a la
vez, salvo que el receptor pase a colgar del coprocesador.

**Dos fuentes → hito 8.** Un navegador sobre los datos de una sola herramienta
es la pantalla de esa herramienta. Solo tiene contenido propio cuando hay que
relacionar observaciones de orígenes distintos.

## Retirados

**2b — Logger sin M5Unified.** Era bajar de nivel el hito 2 sin cambiar lo que
hace. No produce herramienta. M5Unified se sustituye solo donde estorbe a una
pieza concreta, y se documenta entonces.

**6 — Reescritura del WiFi en ESP-IDF.** Mismo motivo. Además el sniffer ya
llama directamente a la API de ESP-IDF (`esp_wifi_set_promiscuous` y compañía);
Arduino solo aporta el arranque del driver. Queda como opción si aparece un
límite concreto del framework, por ejemplo una opción de `sdkconfig` que en
Arduino viene precompilada y no se puede cambiar.

**Los *gates* de nivel.** El enlace inter-MCU estaba bloqueado hasta el hito 6
por la escalera. Su bloqueo real es el de la tabla de capas: no hay segundo
microcontrolador, y ninguna herramienta lo exige todavía.

## Orden de trabajo propuesto

1. Flashear lo pendiente: pieza 1 del sniffer (informe de arranque y heap) y
   montaje de la SD con tarjeta.
2. Hito 3, piezas 2 a 4. No necesitan SD.
3. Hito 2: primera escritura a fichero y medida del peor caso de latencia.
4. Hito 3, pieza 5. Requiere decidir antes el formato del fichero
   ([`06-model/data-model.md`](06-model/data-model.md)).
5. Lanzador, con el sniffer y el terminal como las dos primeras herramientas.
6. Hito 4a o hito 5, a elegir.

## Regla de hardware

El usuario posee **solo el Cardputer base**. No se recomienda comprar nada hasta
agotar el Cardputer para un objetivo concreto. Si un hito lo exige:

1. **Receptor IR** (tipo TSOP38238, 38 kHz): lo único que desbloquea un hito ya
   definido (4b).
2. **Segundo microcontrolador** (coprocesador): desbloquea el enlace inter-MCU.
   Lo que aporta es temporización determinista, no pines; ver
   [`05-vision.md`](05-vision.md).
3. **Radio** (CC1101, nRF24L01): amplía el espectro cubierto.
4. **NFC** (PN532), **CAN** o **analizador lógico**.

Todas compiten por el Grove.
