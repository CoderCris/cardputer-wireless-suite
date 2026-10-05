# Conflictos de pines compartidos

El Cardputer reutiliza pines entre periféricos. Un pin compartido no es un error
de diseño: significa que **no puedes usar los dos periféricos que lo comparten al
mismo tiempo** sin reconfigurar el pin. Señala esto siempre que sea relevante para
el hito en curso.

Los números de pin son los de [`cardputer-map.md`](cardputer-map.md).

Aquí solo cuentan los **pads compartidos en el cobre** de la placa. Compartir un
bus o un controlador SPI es otra cosa, con otras consecuencias: ver
[`spi-pads-controladores.md`](spi-pads-controladores.md).

## El conflicto conocido

> **Retirado: "`GPIO12` — Display CS ↔ SD MOSI".** Nunca existió. Salía de una
> tabla de pines equivocada. El display usa CS=37 y la SD usa CS=12 y MOSI=14; no
> comparten ningún pad. Fuentes en [`cardputer-map.md`](cardputer-map.md). Lo que
> sí comparten es el **presupuesto de controladores**: GP-SPI3 lo ocupa el display
> y GP-SPI2 lo ocupará la SD, y el S3 no tiene más para uso general.

### `GPIO43` — Mic CLK ↔ Speaker LRCK

Micrófono y altavoz comparten un pad: el reloj PDM del micrófono y el *word
select* del altavoz. No puedes capturar audio y
reproducir simultáneamente sobre esa configuración sin multiplexar. Para la
mayoría de casos (grabar *o* reproducir, no las dos cosas a la vez) no es
problema, pero condiciona cualquier idea de "escuchar y responder en tiempo real".

Relevante en: **hito 7 (side-channel de audio I2S)**.

> **Retirado: "`GPIO46` — IR RX ↔ Mic CLK".** Tampoco existía. `GPIO46` es el
> DATA del micrófono y el Cardputer V1 no tiene receptor IR (ver
> [`cardputer-map.md`](cardputer-map.md)). El hito 4 no compite con el micrófono
> por ningún pad del Cardputer; sí compite por el Grove con cualquier otro
> periférico externo.

## Cómo razonar sobre esto al programar

Un pin es un recurso físico único. Configurarlo para un periférico (dirección,
función en la matriz GPIO, pull-up/down) sobrescribe la configuración anterior. En
nivel Arduino la librería te oculta esto; en nivel 2+ (ESP-IDF / registros) eres
tú quien llama a `gpio_config()` o escribe la matriz GPIO, así que un conflicto se
manifiesta como "el periférico B dejó de funcionar cuando inicialicé A".

Antes de inicializar un periférico que use un pin compartido, pregúntate: ¿está el
otro periférico activo ahora mismo? Si sí, necesitas des-inicializarlo o
multiplexar en el tiempo.

## Tabla resumen

| GPIO | Periférico A | Periférico B | Regla |
|------|--------------|--------------|-------|
| 43 | Mic CLK | Speaker LRCK | No capturar y reproducir a la vez |
