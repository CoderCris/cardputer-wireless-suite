# Visión — de multiherramienta a cyberdeck

Capa de abstracción **por encima** del roadmap. Este documento describe hacia
dónde apunta el proyecto a largo plazo y, sobre todo, **qué restricciones impone
eso sobre el código que se escribe hoy**.

> No es un plan de trabajo. El plan de trabajo es [`roadmap.md`](roadmap.md), y
> manda sobre este documento. Si algo de aquí contradice a un hito, gana el hito.

---

## Qué es el objetivo

El Cardputer como núcleo de un **cyberdeck modular**: una carcasa impresa que aloja
el dispositivo junto a piezas de expansión con **procesador propio**, y un modelo
de datos común que todas las herramientas comparten y que se navega siguiendo las
fases de un pentest.

Tres piezas independientes, con dependencias distintas y niveles distintos de la
[escalera](02-abstraction-ladder.md):

| Pieza | Qué es | Nivel | Depende de |
|-------|--------|-------|------------|
| Plataforma de datos | Store compartido + navegador sobre el grafo | 1→2 | Hito 2 (persistencia) |
| Enlace inter-MCU | Protocolo propio entre Cardputer y coprocesadores | 4 | Hito 6 (ESP-IDF) |
| Carcasa | Integración física, alimentación, ergonomía | — | Las dos anteriores |

## Qué NO es

No es un producto ni un firmware alternativo a Bruce o Hydra: si el objetivo fuera
tener las herramientas, ya existen y están flasheadas. No es una lista de compras.
Y no es una licencia para construir por delante del nivel actual: la visión existe
para **condicionar interfaces**, no para justificar saltarse hitos.

---

## Por qué un segundo microcontrolador

La razón **no** es tener más pines. Si solo faltan GPIO, un expansor I2C
(MCP23017: 16 pines, sin firmware) lo resuelve y no aporta nada conceptualmente.

La razón real es que un MCU propio te da **una segunda línea temporal**. En el
ESP32-S3 del Cardputer, con el stack WiFi activo en modo promiscuo, hay
interrupciones de radio que no controlas y que arruinan cualquier bucle con
requisitos de microsegundos. Un coprocesador sin stack de red puede muestrear un
bus a MHz o generar un tren de pulsos con jitter bajo mientras el Cardputer hace
otra cosa. Lo que se compra es **determinismo**, no conectividad.

Esa es también la frontera que el proyecto busca cruzar: en software puro nunca
hace falta preguntarse de quién es un microsegundo concreto. Aquí es la pregunta
central.

## El enlace físico: siguen siendo dos pines

Un coprocesador no cambia la física del Cardputer. Al exterior solo salen `G1` y
`G2` del puerto Grove (ver [`01-hardware/cardputer-map.md`](01-hardware/cardputer-map.md)),
así que el bus entre piezas es **I2C o UART**, y esa elección arrastra el diseño
completo del protocolo.

Dos alternativas descartadas, con motivo:

- **ESP-NOW** (sin cables, sin gastar pines): necesita canal fijo, y la herramienta
  insignia del roadmap —el sniffer del hito 3— salta de canal. El bus inalámbrico
  se pelea con el caso de uso principal.
- **USB host**: el S3 tiene OTG, pero el USB-C del Cardputer está cableado para
  CDC/JTAG y alimentación. Convertirlo en host deja el dispositivo sin puerto de
  flasheo.

Dos condiciones que parecen menores y no lo son. **Masa común obligatoria** entre
piezas: sin referencia compartida no hay bus, y es el fallo que más se olvida. Y
**el USB-C tiene que quedar accesible** con el Cardputer montado en la carcasa: un
diseño que obligue a desacoplar para flashear no sobrevive a la segunda semana.

Sobre alimentación: el riel de 5 V del Grove sale de la batería del Cardputer (sin PMIC
gestionable por software) y no
está pensado para sostener una pantalla más un MCU más radios. El hub se alimenta
a sí mismo. En qué dirección fluye la corriente entre las dos piezas es una
decisión de diseño con consecuencias físicas, no una casualidad del cableado.

## Quién manda

**El Cardputer es el cerebro; el hub es una granja de coprocesadores.** El
Cardputer mantiene el store y la navegación, y cada pieza del hub expone una
interfaz de **mensajes semánticos**, no de pines: "muestra estos 6 nodos", no
"pon este píxel".

La alternativa habitual en cyberdecks —hub como cerebro, Cardputer degradado a
teclado y pantalla— sería un error aquí: tira a la basura el WiFi, el BLE y el IR
del S3, que son los hitos 3, 4 y 5.

## El filtro para aceptar o rechazar ideas

Toda pieza de la visión tiene que pasar una prueba:

> **¿Qué concepto enseña que no enseñe ninguna pieza anterior?**

Ejemplo trabajado, la segunda pantalla. Cableada directamente al SPI del Cardputer
es el mismo ST7789 del hito 1 con otro chip select: más soldadura, cero conceptos
nuevos — y además esos pines no salen del chasis. La misma pantalla colgada de un
coprocesador con protocolo propio enseña framing, sincronización, backpressure y
propiedad del tiempo. **La pantalla no está justificada; el coprocesador sí.**

## El coste que se acepta

En cuanto hay dos binarios hablando entre sí, se hereda el problema de sistemas
distribuidos en miniatura: framing de mensajes sobre un flujo de bytes,
detección de error, resincronización cuando una pieza arranca antes que la otra,
backpressure cuando el productor va más rápido que el consumidor, y versionado de
dos firmwares que evolucionan por separado. En software eso lo tapan TCP y JSON.
Aquí no hay nada debajo. Es excelente material de aprendizaje y es también donde
mueren la mayoría de estos proyectos.

---

## Decisiones abiertas

| Decisión | Bloquea | Estado |
|----------|---------|--------|
| I2C o UART para el enlace | Protocolo inter-MCU | Pendiente (ver el escenario del coprocesador con datos listos y el Cardputer ocupado) |
| Nodo = entidad u observación | [`06-model/data-model.md`](06-model/data-model.md) | Pendiente |
| Campo obligatorio de todo registro | Hito 2 | Pendiente |
| MCU del coprocesador | Primera compra | Pendiente, gateada al hito 6 |

## Cómo concilia con lo anterior

Los hitos 1 a 7 **no cambian**. La visión no los sustituye ni los acelera: los
consume. No se puede diseñar un protocolo entre microcontroladores sin haber
sentido antes un bus por dentro, y eso es el hito 2.

Lo que sí cambia, y entra ya:

1. **El contrato de salida** (desde el hito 2): ninguna herramienta imprime a
   pantalla como salida primaria; emite registros tipados al store, y la pantalla
   es un consumidor más. Detalle en [`06-model/data-model.md`](06-model/data-model.md).
2. **El hito 8** deja de ser "menú de herramientas" y pasa a ser "navegador del
   grafo". Ver el eje de plataforma en [`roadmap.md`](roadmap.md).
3. **El orden de compras** se reordena: el coprocesador pasa a ser la primera,
   por ser lo más barato que desbloquea un eje entero — pero sigue gateado.
