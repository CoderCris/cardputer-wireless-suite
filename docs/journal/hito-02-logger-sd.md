# Diario — Hito 2: Logger a MicroSD

> Diario de hito. Rellénalo **tú** a medida que avanzas: es tu registro de
> decisiones y aprendizajes, no una spec.

**Objetivo**: persistir en la MicroSD lo que hoy solo vive en pantalla. Primer
destino de datos que no es el display, y por eso el punto donde entra el
contrato de salida ([`../06-model/data-model.md`](../06-model/data-model.md)).

**Estado**: en 2a. `TODO(sd 1..3)` implementados; `TODO(sd 4)` parcial y `TODO(heap)` pendientes. Compila, sin flashear.

## Estado al guardar partida (2026-10-15)

**Al retomar (otro equipo):** `git pull` en la rama `hito-02-logger-sd`, y
`pio run` para comprobar que compila (compilaba al guardar; sigue sin flashear).

**Hecho en `src/main.cpp`:**

- `TODO(sd 1)`: cuatro `constexpr uint8_t` con SCK 40, MISO 39, MOSI 14, CS 12.
  Corregidos tras dos errores: tipo `bool` (los cuatro valían 1) y MISO/MOSI
  intercambiados.
- `TODO(sd 2)`: `SPI.begin(sck, miso, mosi, cs)`; el orden es SCK, **MISO**,
  MOSI, SS (`SPI/src/SPI.h:63`).
- `TODO(sd 3)`: `SD.begin(SD_CS_PIN, SPI, 4000000)`.
- `TODO(sd 4)`: parcial. Guarda el `bool` y pinta `WARN :: SD init failed` si
  falla.

**Falta en el código:**

- `TODO(sd 3)`: comentario que justifique los 4 MHz (el TODO lo pedía).
- `TODO(sd 4)`: rama de éxito ("SD ok") y tamaño con `SD.cardSize()`
  (`uint64_t`, bytes: decidir la unidad al imprimir y qué pasaría con un `int`).
- `TODO(heap)`: pintar `ESP.getFreeHeap()`.

**Conceptos de la sesión.** Papel y sentido de cada línea SPI (SCK, MOSI, MISO,
CS); `SD.begin` es un diálogo con tiempo límite, no un bucle infinito, y su
`false` no distingue cableado, formato o ausencia de tarjeta; `uint8_t` no
protege de negativos (`-1` pasa a 255), la protección sería `static_assert`;
límite de 25 MHz del modo por defecto (recorte en `sd_diskio.cpp:605`) y límite
práctico por integridad de señal; inicialización a 400 kHz como máximo porque el
host no conoce aún a la tarjeta (la librería ya lo hace, `sd_diskio.cpp:502`).
Esto último viene de la especificación SD Physical Layer, no verificado en el
repo.

**Pregunta abierta para empezar:** si `cardSize()` (`uint64_t`, bytes) de una
tarjeta de 16 GB se guarda en un `int`, ¿qué valor aparece y por qué?

## Estado al guardar partida (2026-10-14) — histórico

**Al retomar, la micro-tarea:** rellenar `TODO(sd 1..4)` y `TODO(heap)` en
`setup()` de `src/main.cpp`, compilar y flashear (`pio run -t upload`). Anotar
lo que salga en pantalla: si el montaje dice ok o fallo, el tamaño de la
tarjeta y el heap libre. Si el montaje falla, mirar el formato de la tarjeta en
el PC con `lsblk -f` antes de tocar el código.

**Hecho en la sesión del 2026-10-14:** las tres decisiones pendientes de la
partida anterior quedaron cerradas (ver "Decisiones tomadas"). El modelo de
datos queda en [`../06-model/data-model.md`](../06-model/data-model.md),
incluida la lista de lecturas para profundizar. Conceptos nuevos: proyección,
reloj lógico, NVS y pérdida declarada frente a pérdida silenciosa.

**Pendiente de diseño, sin bloquear la micro-tarea:**

- empates dentro del mismo milisegundo;
- desbordamiento de `millis()`;
- versión de esquema o de firmware en cada registro;
- formato del log (JSONL o TLV);
- campos finales del evento de resumen.

Siguen pendientes también las pruebas del hito 1 en hardware: wrap, ghosting,
rebote y barrido.

## Estado al guardar partida (2026-10-13) — histórico, decisiones ya cerradas

**Rama**: `hito-02-logger-sd`, creada desde `hito-01-terminal`.

**Arrastrado del hito 1.** El ajuste de línea está implementado y compila, pero
**sin flashear ni observar**. Las cuatro pruebas de wrap y las de ghosting,
rebote y barrido siguen en "Pendiente de comprobar en hardware" de
[`hito-01-terminal.md`](hito-01-terminal.md).

**Pendiente de decidir (lo primero al retomar).**

1. ¿Se parte el hito en dos? Propuesta:
   - **2a**: SD con la librería `SD` de Arduino, manteniendo M5Unified. Nivel 1;
     el concepto nuevo es el filesystem.
   - **2b**: quitar M5Unified e inicializar el hardware a mano. Nivel 2. Su gate
     es el checklist de dominio de nivel 1 del diario del hito 1, con las cuatro
     casillas aún sin marcar.

   Si se acepta, hay que reflejarlo en [`../roadmap.md`](../roadmap.md).
2. ¿Hay tarjeta MicroSD, y en qué formato? Tiene que ser FAT32. Una de 64 GB o
   más suele venir en exFAT y lo esperable es que el montaje falle (por
   comprobar).
3. Las dos decisiones abiertas de `data-model.md`: si el nodo primitivo es la
   entidad o la observación, y qué campo lleva todo registro. Hacen falta antes
   de escribir el primer registro, no antes de montar la tarjeta.

**Primer incremento propuesto: montar la tarjeta y nada más.** Esqueleto para
`setup()` en `src/main.cpp`, después de la cabecera. Aún no está en el código.
Pines: SCK 40, MISO 39, MOSI 14, CS 12
([`../01-hardware/spi-pads-controladores.md`](../01-hardware/spi-pads-controladores.md)).

```cpp
#include <SPI.h>
#include <SD.h>

// TODO(sd 1): cuatro constantes con los pines de la SD.

// En setup(), tras pintar la cabecera:

// TODO(sd 2): arrancar el bus con SPI.begin(sck, miso, mosi, cs).
// El objeto global SPI es el controlador GP-SPI2; el display ya ocupa GP-SPI3.
// Ojo al orden de los argumentos.

// TODO(sd 3): montar con SD.begin(cs, SPI, frecuencia). Devuelve bool.
// Empieza con una frecuencia conservadora y justifica el número en el comentario.

// TODO(sd 4): pintar en pantalla "SD ok" o "SD fallo" según ese bool, y si
// montó, el tamaño con SD.cardSize() (devuelve bytes en uint64_t: piensa en qué
// unidad lo imprimes y qué pasa si lo metes en un int).

// TODO(heap): pintar ESP.getFreeHeap(). Es el presupuesto real del store.
```

**Qué abstrae `SD`** (ya explicado, no repetir). Envía por SPI los comandos de
la tarjeta en su modo SPI (`CMD0` para resetearla, `CMD17`/`CMD24` para leer y
escribir bloques de 512 bytes) y monta FatFs encima, que convierte bloques
numerados en ficheros con nombre.

## Decisiones tomadas

- **2026-10-14 — El hito se parte en 2a/2b.** 2a: SD con la librería `SD`,
  manteniendo M5Unified (nivel 1). 2b: quitar M5Unified (nivel 2), con el gate
  del checklist de nivel 1 del hito 1. Motivo: un solo cambio de nivel o de
  periférico por paso, para saber a qué atribuir un fallo. Reflejado en
  [`../roadmap.md`](../roadmap.md).
- **2026-10-14 — Hay MicroSD.** Formato por confirmar en el primer montaje.
- **2026-10-14 — Modelo de datos: la observación es el nodo primitivo** (event
  sourcing), y todo registro lleva **contador de arranques + tiempo**. Detalle,
  subpreguntas pendientes y lecturas en
  [`../06-model/data-model.md`](../06-model/data-model.md).

## Qué falló y cómo lo resolví

## Preguntas abiertas

- **Buffer de RAM lleno sin tarjeta.** Propuesta (2026-10-14): resumir las
  observaciones pendientes en un evento de resumen, en lugar de descartarlas en
  silencio. Ajuste: debe ser un *tipo* de registro distinto, inmutable, que diga
  qué sustituye (número, rango `(boot, t)`) y con qué regla se resumió.
  Campos propuestos: número de eventos resumidos (por tipo, no solo el total),
  rango `(boot, t)` cubierto, qué conserva y qué descarta, y la regla como
  identificador + versión (`resumen_v1`), no como texto libre, porque la regla
  cambiará entre versiones del firmware. Abre otra pregunta: ¿deben llevar
  todos los registros la versión de esquema o de firmware?

## Pendiente de comprobar en hardware

- [ ] **Test de frecuencia de la SD** (posterior al primer montaje): código que
      escriba datos con patrón conocido a varias frecuencias (p. ej. 4, 10, 20 y
      25 MHz), los lea de vuelta, compare y registre a partir de qué frecuencia
      aparecen diferencias. Objetivo: fijar la frecuencia de trabajo un escalón
      por debajo del primer fallo, con medición y no por suposición. Lo ejecuta
      y lo observa el usuario.

- [ ] Resultado del montaje, tamaño de la tarjeta y heap libre al arrancar.
      Anotar el heap en [`../01-hardware/cardputer-map.md`](../01-hardware/cardputer-map.md).
