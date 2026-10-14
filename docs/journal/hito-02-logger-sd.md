# Diario — Hito 2: Logger a MicroSD

> Diario de hito. Rellénalo **tú** a medida que avanzas: es tu registro de
> decisiones y aprendizajes, no una spec.

**Objetivo**: persistir en la MicroSD lo que hoy solo vive en pantalla. Primer
destino de datos que no es el display, y por eso el punto donde entra el
contrato de salida ([`../06-model/data-model.md`](../06-model/data-model.md)).

**Estado**: en 2a. Esqueleto `TODO(sd 1..4)` + `TODO(heap)` en `src/main.cpp`.

## Estado al guardar partida (2026-10-13)

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

## Pendiente de comprobar en hardware

- [ ] Resultado del montaje, tamaño de la tarjeta y heap libre al arrancar.
      Anotar el heap en [`../01-hardware/cardputer-map.md`](../01-hardware/cardputer-map.md).
