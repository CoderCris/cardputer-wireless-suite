# Buses del Cardputer — qué cuelga de cada uno

Referencia conceptual de los tres buses serie que usa el Cardputer. Para pines
exactos, ver [`cardputer-map.md`](cardputer-map.md). Para el detalle a nivel de
protocolo y registros, ver [`../04-protocols/`](../04-protocols/).

Un **bus** es un conjunto de líneas compartidas por varios dispositivos. La gracia
del bus es que muchos periféricos usan las mismas líneas de datos/reloj y se
distinguen por una señal de selección (SPI) o una dirección (I2C).

## SPI (Serial Peripheral Interface)

Bus síncrono full-duplex, maestro-esclavo. Líneas: **SCLK** (reloj), **MOSI**
(maestro→esclavo), **MISO** (esclavo→maestro) y un **CS** (chip select) por
esclavo. El maestro genera el reloj; cada flanco desplaza un bit. Se selecciona un
esclavo bajando su CS.

En el Cardputer cuelgan de SPI el **display ST7789V2** y la **MicroSD**, pero
**en buses SPI físicos distintos** (el S3 tiene varios controladores SPI). El
display comparte `GPIO12` como CS con el MOSI de la SD — ver
[`pin-conflicts.md`](pin-conflicts.md).

El ESP32-S3 implementa SPI con periféricos dedicados (GP-SPI2/SPI3). En nivel 2
usarás el driver `spi_master` de ESP-IDF; en nivel 4 podrías bit-banguear SPI
manipulando GPIO directamente para ver el timing con tus propias manos.

Referencia TRM: capítulo *SPI Controller (GP-SPI2, GP-SPI3)*.

## I2C (Inter-Integrated Circuit)

Bus síncrono half-duplex de dos hilos: **SDA** (datos) y **SCL** (reloj). Cada
esclavo tiene una **dirección** de 7 bits; el maestro emite la dirección y todos
los esclavos la escuchan, pero solo responde el que coincide. Mucho más lento que
SPI, pero solo dos pines para muchos dispositivos.

En el Cardputer, el **AXP2101** (PMIC de energía) está en el bus I2C interno, y el
conector **Grove** expone SDA `GPIO2` / SCL `GPIO1` para periféricos externos.

Referencia TRM: capítulo *I2C Controller*.

## I2S (Inter-IC Sound)

Bus síncrono pensado para **audio digital**. Líneas típicas: **BCLK** (bit clock),
**LRCK/WS** (word select, indica canal izquierdo/derecho) y **DATA** (SD).
Transporta muestras de audio en serie, sincronizadas al reloj de bit.

En el Cardputer cuelgan de I2S el **micrófono SPM1423** (PDM/I2S de entrada) y el
**altavoz NS4168** (I2S de salida). Comparten `GPIO43` — ver
[`pin-conflicts.md`](pin-conflicts.md). Para blue team, el interés de I2S es el
análisis de señal y los side-channels (hito 7).

Referencia TRM: capítulo *I2S Controller*.

## Por qué esto importa para el proyecto

La escalera de abstracción ([`../02-abstraction-ladder.md`](../02-abstraction-ladder.md))
consiste, en buena parte, en ir bajando desde "la librería me habla con el
periférico" hasta "yo configuro el controlador del bus" y finalmente "yo genero el
timing del protocolo". Entender qué bus usa cada periférico es el primer paso para
poder bajar de nivel con criterio.
