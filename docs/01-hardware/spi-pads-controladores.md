# Pad, bus y controlador

Cuando se dice que dos periféricos "comparten SPI", pueden estar compartiendo
tres cosas distintas, y cada una tiene consecuencias diferentes. Esta nota separa
las tres capas que hay entre el silicio del ESP32-S3 y el chip externo, y aplica
el modelo al Cardputer. Los números de pin están en
[`cardputer-map.md`](cardputer-map.md) y aquí solo se citan.

Explicación visual (recorrido de un bit, qué se puede cambiar en cada capa):
[Del controlador al cobre](https://claude.ai/artifact/NRY4e42mJNDBEQem6xSqFz).

Todo lo de aquí sale de **leer código y datasheets**, no de observar hardware.

---

## Las tres capas

```
  DENTRO DEL ESP32-S3 (se cambia escribiendo registros)       PCB (cobre, fijo)
 ┌─────────────────────────────────────────────────────────┐
 │ SPI0/SPI1 ── MSPI ── flash interna        (no son tuyos) │
 │                                                          │
 │  CONTROLADOR          ENRUTADO                PAD        │
 │  ┌─────────┐     ┌──────────────────┐                    │
 │  │ GP-SPI2 │════►│                  │──► 40 ─────────────┼── SCLK ─┐
 │  │ (FSPI)  │════►│   GPIO Matrix    │──► 14 ─────────────┼── MOSI ─┤ MicroSD
 │  │         │◄════│   (+ IO MUX)     │◄── 39 ─────────────┼── MISO ─┤
 │  │         │════►│                  │──► 12 ─────────────┼── CS  ──┘
 │  └─────────┘     │                  │                    │
 │  ┌─────────┐     │                  │                    │
 │  │ GP-SPI3 │════►│                  │──► 36 ─────────────┼── SCLK ─┐
 │  │         │◄═══►│                  │◄─► 35 ─────────────┼── SDA  ─┤ ST7789
 │  │         │════►│                  │──► 37 ─────────────┼── CS  ──┤
 │  └─────────┘     │                  │──► 34 ─────────────┼── DC  ──┘
 │  GPIO normal ═══►│                  │   (DC no es SPI)   │
 │                  └──────────────────┘                    │
 └──────────────────────────────────────────────────────────┘
```

**Controlador.** Es el bloque de silicio que hace el trabajo SPI: genera el reloj,
desplaza bits desde o hacia un registro de desplazamiento, mueve las CS que tenga
asignadas y avisa por interrupción o DMA al terminar. El S3 tiene cuatro. SPI0 y
SPI1 forman el MSPI (memory SPI), que ejecuta código desde la flash y no está
disponible para ti. Quedan **dos de propósito general: GP-SPI2 y GP-SPI3**.
Referencia TRM: capítulo *SPI Controller (SPI)*.

**Enrutado.** Cada señal interna de un controlador (por ejemplo `FSPICLK_out`) no
sale a un pin fijo, sino a una centralita configurable. El **IO MUX** conecta
algunas señales de forma directa a unos pads concretos. La **GPIO Matrix** puede
llevar casi cualquier señal a casi cualquier pad, a cambio de un pequeño retardo.
Es una decisión de **software**: son registros. Referencia TRM: capítulo *IO MUX
and GPIO Matrix (GPIO, IO MUX)*.

**Pad y cobre.** El pad es la patilla física del chip. A qué está soldado lo
decidió M5Stack al diseñar la placa, y ningún registro lo cambia. El pad 14 está
unido a la línea MOSI de la ranura SD, y lo seguirá estando.

## Tres formas de "compartir"

| Qué se comparte | Dónde se decide | Consecuencia | En el Cardputer |
|---|---|---|---|
| Pad (mismo cobre, dos chips) | PCB | Exclusión mutua: solo uno a la vez | `GPIO43` mic CLK ↔ speaker LRCK ([conflictos](pin-conflicts.md)) |
| Bus (SCLK/MOSI/MISO comunes, un CS por chip) | PCB | Conviven; el CS decide quién escucha | Ninguno |
| Controlador | Tu código | Reconfigurar el enrutado en cada cambio, sin solapar transacciones | Evitable hoy; no a partir de un tercer dispositivo |

Un **bus compartido** es el uso normal de SPI. Varios chips cuelgan en paralelo de
las mismas SCLK, MOSI y MISO, y cada uno tiene su CS. El chip con el CS alto
ignora el reloj y deja su MISO en alta impedancia (desconectado eléctricamente),
así que no estorba. Un **pad compartido** es otro asunto: no hay CS que arbitre,
porque los dos chips están físicamente en el mismo cable.

## Reparto en el Cardputer

| Periférico | Controlador | Quién lo eligió | Fuente |
|---|---|---|---|
| Display ST7789 | GP-SPI3 (`SPI3_HOST`) | M5GFX, en su autodetección | `.pio/libdeps/cardputer/M5GFX/src/M5GFX.cpp` |
| MicroSD | GP-SPI2 (FSPI) | El objeto `SPI` de Arduino, si lo usas | `cores/esp32/esp32-hal-spi.h` (`FSPI 0`) y `esp32-hal-spi.c` (`_spi_bus_array[0]` = `DR_REG_SPI2_BASE`) |

Las dos rutas del framework son relativas a
`~/.platformio/packages/framework-arduinoespressif32/`.

Con display y SD funcionando, **no queda ningún controlador SPI libre**. Esa es la
restricción real del hito 2, no un pin compartido.

Ninguno de los dos buses usa los pads nativos del IO MUX para FSPI (10 a 13 en el
S3); los dos pasan por la GPIO Matrix. El retardo de la matriz afecta sobre todo a
las **entradas**: el dato de MISO llega un poco tarde respecto al flanco en el que
el controlador lo muestrea, y eso pone un techo a la frecuencia de lectura. Puede
explicar por qué M5GFX escribe al display a 40 MHz pero lee a 16 MHz. El ejemplo
de la SD arranca a 25 MHz.

## Si aparece un tercer dispositivo SPI

Antes de pensar en controladores hay que contar pads. El Grove expone **dos**
señales (`G1`, `G2`), y un SPI completo necesita cuatro (tres en la variante de 3
hilos, más el CS). El primer límite es el cobre. Si los pads existieran, las
opciones serían estas:

1. **Multiplexar un controlador en el tiempo.** Terminar la última transacción,
   liberar el driver, reenrutar las señales del controlador a los pads del otro
   dispositivo y volver atrás al acabar. Requisito de hardware que se suele
   olvidar: el CS del dispositivo que se queda sin controlador **debe quedar
   fijado en alto** como GPIO normal. Si queda flotante, el chip puede darse por
   seleccionado y poner datos en su línea. En la SD, además, hay que cerrar los
   ficheros antes de soltarla: la tarjeta conserva lo escrito, pero los buffers
   de FAT que están en RAM y aún no se han volcado se pierden.
2. **Compartir el bus de verdad.** Llevar el nuevo dispositivo a las mismas
   SCLK/MOSI/MISO que otro y darle su propio CS. El driver `spi_master` de
   ESP-IDF admite varios dispositivos por controlador
   (`spi_bus_add_device` en la copia instalada:
   `~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/driver/include/driver/spi_master.h`). Solo es posible si las líneas son accesibles, cosa que
   en el Cardputer no ocurre sin soldar.
3. **Bit-banging.** Generar SPI moviendo GPIO a mano (nivel 4 de la
   [escalera](../02-abstraction-ladder.md)). Sirve con cualquier pad y no gasta
   controlador, pero la CPU paga cada flanco y el timing depende del software.

## Pendiente de observar en hardware

- [ ] La lectura del display por SPI de 3 hilos (`readPixel()` de M5GFX)
      devuelve lo que se pintó.
- [ ] La SD monta a 25 MHz en GP-SPI2 con el display activo en GP-SPI3.
