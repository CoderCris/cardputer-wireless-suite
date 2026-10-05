# Inventario — kit de desarrollo Arduino

Componentes disponibles **además del Cardputer**. Este documento es un inventario
de referencia, **no amplía el roadmap**: ninguna pieza de aquí abre un hito nuevo
ni se salta un *gate* de [`../roadmap.md`](../roadmap.md). Sirve para saber qué
hay en el cajón cuando un hito lo necesite, y para no comprar lo que ya se tiene.

> **Leído vs. observado.** Las identificaciones salen de las serigrafías que has
> reportado y de los datasheets típicos de esas referencias. Donde la referencia
> es ambigua (patillaje de displays, ánodo/cátodo común, tipo de zumbador) lo
> marco como **pendiente de comprobar**: se resuelve con un multímetro o con una
> prueba a 3,3 V + resistencia, no leyendo.

## Antes de conectar nada al Cardputer: dos restricciones

**Niveles lógicos.** El ESP32-S3 trabaja a **3,3 V y sus GPIO no toleran 5 V**
(el máximo absoluto es VDD + 0,3 V; ver *ESP32-S3 Datasheet*, §5.1 *Absolute
Maximum Ratings*). Arduino UNO y Mega son placas de **5 V**: un pin de salida del
Arduino a nivel alto conectado directamente a un GPIO del Cardputer lo inyecta 5 V
y puede dañarlo. En el sentido Cardputer → Arduino suele funcionar sin adaptar
(el ATmega reconoce como "1" cualquier tensión por encima de ~0,6·VCC = 3 V), pero
en el sentido Arduino → Cardputer hace falta como mínimo un **divisor resistivo**
(p. ej. 1 kΩ en serie + 2 kΩ a GND → 5 V × 2/3 ≈ 3,3 V). Las resistencias del kit
bastan para eso.

**Pines disponibles.** Según [`cardputer-map.md`](cardputer-map.md), el único
acceso externo a GPIO del Cardputer es el conector **Grove** (`GPIO1`, `GPIO2`,
más 5 V y GND). Dos pines. Casi todo lo de este kit pide más, así que para
usarlo desde el Cardputer hay que **multiplexar** (registro de desplazamiento,
bus I2C) o **delegar** en un Arduino. Esa restricción, más que el componente en
sí, es lo que convierte estas piezas en material didáctico.

## Resumen

| Componente | Qué es | Interés para el proyecto |
|------------|--------|--------------------------|
| LEDs rojo/amarillo/verde ×8 | Diodos emisores de luz de 5 mm | GPIO de salida, ley de Ohm |
| Protoboard grande + pequeña | Placa de prototipado sin soldadura | Montaje de todo lo demás |
| Pulsadores de 4 pines ×4 | Pulsador táctil 6×6 mm | GPIO de entrada, rebotes, pull-up |
| Potenciómetro B10K | Resistencia variable lineal de 10 kΩ | ADC, divisor de tensión, contraste del 1602A |
| 3 piezas con zigzag | **Fotorresistencias (LDR)** | ADC; detección de apertura de carcasa (*tamper*) |
| "LED infrarrojo" | Emisor IR 940 nm (pendiente de confirmar) | Hito 4: emitir IR |
| Mando con pila CR2025 | Mando IR, probablemente protocolo NEC | Hito 4: fuente de señales conocida |
| SW-520D ×2 | **Interruptor de inclinación** (no son condensadores) | Detección de movimiento (*tamper*) |
| VS1838B | Receptor IR demodulador de 38 kHz | Hito 4: recibir IR |
| 74HC595N | Registro de desplazamiento 8 bits | Bitwise, protocolo serie bit a bit (nivel 4) |
| Zumbador | Activo o pasivo (pendiente de comprobar) | PWM (LEDC) |
| MR-3461SRB-2 | Display 7 segmentos, 4 dígitos | Multiplexado |
| 1088BS | Matriz de LEDs 8×8 | Multiplexado por filas/columnas |
| 5611AH | Display 7 segmentos, 1 dígito, cátodo común | Mapa de segmentos con bits |
| SG90 | Micro servo 9 g | PWM con ancho de pulso preciso |
| Arduino UNO | Placa ATmega328P, 5 V | Dispositivo objetivo / generador de tráfico de bus |
| Arduino Mega 2560 | Placa ATmega2560, 5 V | Ídem, con 4 UART hardware |
| Placa de extensión | Sin identificar (ver abajo) | — |
| LCD 1602A | Display de caracteres 16×2, HD44780 | Bus paralelo, protocolo con temporización |
| Placa ULN2003AN | Driver Darlington de 7 canales | Conmutar cargas que un GPIO no aguanta |
| 28BYJ-48 | Motor paso a paso unipolar 5 V | Secuencias de fases |
| Clip de pila 9 V | Conector de alimentación | Alimentar el Arduino sin USB |
| Resistencias | Varios valores | Limitar corriente, divisores, pull-up/down |

---

## Componentes pasivos y de entrada

### LEDs (8 rojos, 8 amarillos, 8 verdes)

LED estándar de 5 mm. Es un **diodo**: conduce solo en un sentido y, una vez
conduce, la tensión entre sus patas se queda casi fija (tensión directa, *Vf*),
así que **la corriente no la limita el LED, la limita la resistencia en serie**.
Sin resistencia, la corriente sube hasta que algo se rompe: el LED o el pin.

| Color | Vf típica | Corriente máxima continua |
|-------|-----------|---------------------------|
| Rojo | 1,8–2,2 V | ~20 mA |
| Amarillo | 1,9–2,2 V | ~20 mA |
| Verde | 2,0–2,2 V (verde-amarillento); ~3 V si es verde puro | ~20 mA |

Polaridad: la **pata larga es el ánodo (+)**; el lado del encapsulado con el
borde plano es el cátodo (−). Cálculo de la resistencia: `R = (Vcc − Vf) / I`.
Ejemplo desde un GPIO a 3,3 V con un LED rojo a 10 mA: (3,3 − 2,0) / 0,010 =
130 Ω → usa el valor comercial superior, 150 Ω o 220 Ω. No hace falta llegar a
20 mA para que se vea; 5–10 mA es lo sensato y protege al pin.

### Protoboard (grande y pequeña)

Placa de prototipado sin soldadura. Por dentro tiene tiras metálicas: en la zona
central, **cada columna de 5 agujeros está unida** y la ranura central separa las
dos mitades (por eso los chips DIP se colocan a caballo sobre ella). Los raíles
laterales (+/−) recorren la placa a lo largo; en algunas placas grandes están
**cortados por la mitad**, compruébalo con continuidad antes de fiarte. La
grande suele ser de 830 puntos y la pequeña de 400 o 170.

### Pulsadores de 4 pines

Pulsador táctil de 6×6 mm. Tiene 4 patas pero solo **dos contactos**: las patas
están unidas por pares internamente, y el botón une un par con el otro al
pulsar. Regla práctica: **dos patas en diagonal** son siempre un par conmutado.
Confírmalo con el multímetro en continuidad.

Dos conceptos que vas a encontrar con ellos. Primero, el pin de entrada necesita
un estado definido cuando el botón está suelto: una resistencia de **pull-up**
(o la interna del ESP32-S3) lo lleva a 1, y pulsar lo baja a 0 — el mismo
esquema que ya usa el teclado del Cardputer
([`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md)).
Segundo, el **rebote** (*bounce*): el contacto mecánico vibra durante unos
milisegundos y el pin ve varias transiciones por una sola pulsación.

### Potenciómetro B10K (3 pines)

Resistencia variable de **10 kΩ** entre las dos patas extremas; la central
(cursor, *wiper*) se desliza sobre ella. La **"B" indica curva lineal** (la "A"
sería logarítmica, para audio). Con los extremos a 3,3 V y GND, el cursor da una
tensión entre 0 y 3,3 V proporcional al giro: es un **divisor de tensión**
ajustable. Uso típico: entrada analógica para el ADC (TRM §*ADC Controller*
/ *SAR ADC*) y ajuste del contraste del LCD 1602A.

### Las 3 piezas con zigzag metálico: fotorresistencias (LDR)

La superficie lacada con un zigzag es la firma visual de una **LDR** (*Light
Dependent Resistor*): el zigzag es una pista de sulfuro de cadmio (CdS) cuya
resistencia **baja cuanto más luz recibe**. Probablemente de la serie GL55xx
(sin serigrafía no se puede afinar el modelo): del orden de 10–20 kΩ con luz
ambiente y de cientos de kΩ a 1 MΩ a oscuras. No tiene polaridad.

Se lee igual que el potenciómetro: LDR + resistencia fija (≈10 kΩ) en serie
forman un divisor, y el ADC lee el punto medio. Es lenta (decenas de ms en
reaccionar), así que sirve para luz ambiental, no para datos.

Interés blue team: **detección de manipulación física** (*tamper detection*).
Una LDR dentro de una caja cerrada pasa de "oscuro" a "luz" cuando alguien la
abre; es el principio de muchos sensores antimanipulación de equipos.

### SW-520D (×2): interruptor de inclinación, no condensador

**Corrección:** no son condensadores. El SW-520D es un **interruptor de
inclinación de bola** (*tilt switch*): un cilindro metálico con una bolita
conductora dentro. En vertical (patas hacia abajo) la bola une los dos contactos
y el interruptor **conduce**; al inclinarlo lo bastante, la bola rueda y el
circuito se **abre**. Eléctricamente es un pulsador que acciona la gravedad, y se
lee igual: pull-up + GPIO, con rebotes incluidos (más que un pulsador, porque la
bola rebota). Solo para señales de baja corriente.

Interés blue team: igual que la LDR, **detección de movimiento o manipulación**
de un equipo que debería estar quieto.

---

## Infrarrojos (material directo del hito 4)

### "LED infrarrojo" — pendiente de confirmar

Un LED IR emisor (típicamente **940 nm**, Vf ≈ 1,2–1,5 V, hasta ~50–100 mA en
pulsos) tiene aspecto de LED transparente o ligeramente azulado. Pero en los kits
también viene a veces un **fotodiodo/fototransistor receptor** de aspecto casi
idéntico, normalmente con encapsulado **negro u oscuro**.

**Prueba para distinguirlos:** alimenta el componente como un LED (3,3 V + 220 Ω,
pata larga a +) y míralo **a través de la cámara del móvil** (la frontal suele no
tener filtro IR). Si ves un punto violáceo encendido, es un emisor. Si no, es
probablemente un receptor o lo has puesto al revés.

### Mando IR con pila CR2025

Mando a distancia infrarrojo; la CR2025 es una pila de botón de litio de 3 V. Los
mandos de estos kits casi siempre emiten **protocolo NEC**: una portadora de
**38 kHz** que se enciende y apaga formando una cabecera (≈9 ms encendida + 4,5 ms
apagada) seguida de **32 bits** (dirección, dirección invertida, comando, comando
invertido), codificados por la duración de los huecos. Esto es **pendiente de
comprobar**: lo confirmarás capturando la señal en el hito 4.

Su valor para el proyecto es que es una **fuente de señales conocida y
repetible**: antes de analizar un mando desconocido, decodificas uno cuyo
protocolo conoces.

### VS1838B — receptor IR de 38 kHz

Receptor/demodulador integrado: dentro tiene un fotodiodo, un amplificador, un
filtro paso banda centrado en **38 kHz** y un demodulador. Su salida **no es la
luz cruda, sino la envolvente**: entrega un nivel lógico que está en **alto en
reposo y baja a 0 mientras recibe portadora de 38 kHz** (salida activa en bajo).
Por eso con él mides directamente las duraciones de los pulsos NEC, sin tener que
filtrar la portadora tú mismo.

| Pin (mirando la cúpula, patas abajo, de izquierda a derecha) | Función |
|------|---------|
| 1 | OUT (salida digital) |
| 2 | GND |
| 3 | VCC (2,7–5,5 V; a 3,3 V funciona) |

Este patillaje es el del VS1838B suelto; si viniese montado en una plaquita, las
serigrafías de la placa mandan. Para medir sus pulsos con precisión de hardware
está el periférico RMT (TRM §*Remote Control Peripheral (RMT)*).

> **Discrepancia a verificar.** [`cardputer-map.md`](cardputer-map.md) lista un
> *IR RX* en `GPIO46`. Las especificaciones públicas del Cardputer V1 mencionan
> solo un **emisor** IR (`GPIO44`) y sitúan `GPIO46` en el micrófono. Contrasta
> con el esquemático oficial antes del hito 4. Si el Cardputer **no** tiene
> receptor, el VS1838B por el puerto Grove es el que cubre ese hueco.

---

## Lógica digital y drivers

### 74HC595N (HYC 825Z) — registro de desplazamiento

"HYC 825Z" es el código de fabricante/lote; la pieza es el **74HC595**: un
registro de desplazamiento de 8 bits con entrada serie, salida paralelo y latch.
Convierte **3 pines del microcontrolador en 8 salidas**, y se pueden encadenar
varios (16, 24... salidas con los mismos 3 pines).

Funcionamiento: en cada flanco de subida de **SRCLK** el chip lee el bit presente
en **SER** y desplaza todo el registro una posición. Tras 8 flancos, el registro
interno contiene el byte, pero las salidas aún no cambian. Un flanco en **RCLK**
(*latch*) copia el registro interno a las 8 salidas de golpe. Esa separación
evita que las salidas "parpadeen" con los valores intermedios del desplazamiento.

| Pin | Nombre | Función |
|-----|--------|---------|
| 15, 1–7 | QA, QB…QH | Las 8 salidas paralelo |
| 8 | GND | Masa |
| 9 | QH' | Salida serie (para encadenar al SER del siguiente chip) |
| 10 | SRCLR̅ | Borrado del registro, activo en bajo → a VCC si no se usa |
| 11 | SRCLK | Reloj de desplazamiento |
| 12 | RCLK | Reloj del latch (actualiza salidas) |
| 13 | OE̅ | Habilitación de salidas, activo en bajo → a GND |
| 14 | SER | Entrada de datos serie |
| 16 | VCC | 2–6 V (funciona a 3,3 V) |

Limitación eléctrica: cada salida admite como máximo ±35 mA y el chip entero
**70 mA** por VCC/GND (máximos absolutos del datasheet). Ocho LEDs a 20 mA
(160 mA) lo superan: con este chip, LEDs a ~5 mA.

Por qué importa aquí: manejarlo **sin librería** es exactamente el nivel 4 de la
escalera (generar a mano el timing de un protocolo serie síncrono, que es SPI en
miniatura), y obliga a extraer bits de un byte con desplazamientos y máscaras.

### Placa con ULN2003AN (17TYDZ718K) — driver de potencia

"17TYDZ718K" es código de lote; el chip es el **ULN2003A**: siete pares
**Darlington** (dos transistores en cascada que amplifican mucho la corriente)
en un solo encapsulado. Cada canal deja que una entrada de nivel lógico (unos mA)
conmute a masa una carga de hasta **500 mA** y 50 V. Incluye **diodos de rueda
libre** (*flyback*) para absorber el pico de tensión que genera una bobina al
cortarle la corriente; por eso sirve para motores y relés sin añadir diodos.

La placa típica trae entradas IN1–IN4, cuatro LEDs que muestran qué canal está
activo, el conector blanco de 5 pines para el 28BYJ-48 y dos pines de
alimentación del motor (5–12 V). Con entradas de 3,3 V el ULN2003A conmuta
correctamente las corrientes de este motor (~100 mA por fase).

Idea clave: **un GPIO nunca alimenta un motor directamente**. Da la orden; el
driver pone la corriente, que viene de otra fuente.

---

## Displays

Tres de los displays del kit son, por dentro, **muchos LEDs con un terminal
compartido**. El concepto común es el **multiplexado**: en lugar de un pin por
LED, se enciende un dígito (o una fila) cada vez, muy rápido, y la persistencia
de la visión hace que parezca que todos están encendidos a la vez.

"Cátodo común" significa que todos los LEDs de un dígito comparten la pata
negativa (se activan poniendo el segmento en alto); "ánodo común", que comparten
la positiva (se activan poniendo el segmento en bajo). Equivocarse no rompe nada
con resistencia en serie, pero no se enciende.

### 5611AH — 7 segmentos, 1 dígito

Display de 0,56" (el "56"), un dígito, **cátodo común** (la "A"; la variante
5611BH es de ánodo común). 10 pines, con los dos centrales como común.

| Pin | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-----|---|---|---|---|---|---|---|---|---|----|
| Segmento | E | D | COM | C | DP | B | A | COM | F | G |

Pines numerados con los decimales hacia abajo: 1–5 la fila inferior de izquierda
a derecha, 6–10 la superior de derecha a izquierda. Cada segmento necesita su
propia resistencia. Representar un dígito es asignar un bit a cada segmento y
tener una tabla de 10 bytes: buen ejercicio de máscaras, y encaja con el 74HC595
(8 salidas = 7 segmentos + punto).

### MR-3461SRB-2 — 7 segmentos, 4 dígitos

Display de 4 dígitos de 0,36" (el "36"), rojo (*SR*, *super red*), 12 pines:
8 de segmentos (A–G + DP) compartidos por los cuatro dígitos y 4 comunes, uno por
dígito. Los segmentos de igual letra están unidos entre dígitos, por lo que
**solo puede mostrarse un dígito a la vez**: obligatorio multiplexar. "13 19"
parece código de fecha.

**Pendiente de comprobar: ánodo o cátodo común.** La "B" del sufijo suele indicar
ánodo común en esta nomenclatura, pero no es fiable entre fabricantes. Prueba:
multímetro en modo diodo (o 3,3 V + 1 kΩ) entre un pin de dígito y uno de
segmento, en los dos sentidos; el sentido que enciende revela cuál es el común.

### 1088BS — matriz de LEDs 8×8

Matriz de 64 LEDs rojos en 8 filas × 8 columnas, 16 pines. Cada LED está entre
una fila y una columna; se enciende uno poniendo su fila y su columna en la
polaridad correcta. En la nomenclatura habitual, **"BS" = ánodos en las filas**
(la "AS" sería la inversa), **pendiente de comprobar**. Ojo: el patillaje **no es
secuencial** (el pin 1 no es la fila 1); hay que mapearlo con la prueba de diodo
y apuntarlo. Se maneja barriendo filas: 16 pines con multiplexado, o dos 74HC595
(uno para filas, otro para columnas).

### LCD 1602A — display de caracteres 16×2

Display LCD de 2 líneas de 16 caracteres con un controlador **HD44780** (o un
clon compatible) que ya sabe dibujar caracteres: tú le envías códigos ASCII y
comandos, no píxeles. Es el contraste perfecto con el ST7789 del Cardputer, donde
tú pintas cada píxel.

| Pin | Nombre | Función |
|-----|--------|---------|
| 1 | VSS | GND |
| 2 | VDD | Alimentación, normalmente **5 V** |
| 3 | V0 | Contraste: al cursor del potenciómetro B10K |
| 4 | RS | Register Select: 0 = comando, 1 = dato (análogo al DC del ST7789) |
| 5 | RW | 0 = escribir, 1 = leer → **a GND** |
| 6 | E | Enable: el flanco de bajada hace que el LCD lea el bus |
| 7–14 | D0–D7 | Bus de datos paralelo de 8 bits |
| 15 | A | Ánodo de la retroiluminación (+, normalmente con resistencia) |
| 16 | K | Cátodo de la retroiluminación (GND) |

Admite **modo de 4 bits** (solo D4–D7, cada byte en dos mitades), lo que reduce
los pines a 6: RS, E y D4–D7. Con un ESP32: alimentarlo a 5 V y atar RW a GND
hace que el LCD solo **reciba** señales de 3,3 V (que interpreta como "1") y
**nunca devuelva** 5 V al GPIO. El kit no trae el adaptador I2C (PCF8574) que lo
reduce a 2 hilos; con el Grove del Cardputer, el camino sería el 74HC595.

---

## Actuadores

### Zumbador (activo o pasivo — pendiente de comprobar)

Dos tipos con aspecto parecido. El **activo** lleva un oscilador interno: le das
tensión continua y pita a una frecuencia fija. El **pasivo** es solo el
transductor: hay que darle una onda cuadrada (PWM) y la frecuencia elige el
tono. Pistas: el activo suele tener la parte inferior **sellada** con resina y
una pegatina encima; el pasivo deja ver una **placa verde**. Prueba definitiva:
3,3 V directos (pata larga a +); si pita, es activo.

En el ESP32-S3 el PWM lo genera el periférico **LEDC** (TRM §*LED PWM
Controller (LEDC)*). Un zumbador de 5 V consume del orden de 30 mA; sin
transistor en el kit, mejor no colgarlo directamente de un GPIO a máxima
potencia.

### Micro servo SG90 (9 g)

Servomotor con su propio control interno: no le das potencia variable, le das
**una posición** codificada como ancho de pulso. Espera un pulso cada **20 ms
(50 Hz)**; la duración del pulso fija el ángulo, nominalmente 1 ms → 0° y
2 ms → 180° (en la práctica muchos SG90 usan ~0,5–2,4 ms; se calibra).

| Cable | Función |
|-------|---------|
| Marrón | GND |
| Rojo | 4,8–6 V |
| Naranja | Señal PWM (3,3 V suele bastar) |

Al arrancar o bloquearse puede pedir **varios cientos de mA**. Alimentado desde
la misma fuente que el microcontrolador puede provocar caídas de tensión que lo
reinicien (*brown-out*): mejor fuente aparte con **GND común**.

### Motor paso a paso 28BYJ-48

Motor **unipolar** de 5 V, 4 fases, 5 cables (el **rojo** es el común a +5 V).
No gira con tensión continua: gira activando las bobinas **en secuencia**, y
cada cambio de fase avanza un paso. Lleva una reductora de ≈1:64, así que da
**≈2048 pasos por vuelta** en paso completo y ≈4096 en medio paso, con poca
velocidad y bastante par. Cada bobina consume ~100 mA a 5 V: por eso va siempre a
través del ULN2003. Controlarlo es escribir una secuencia de patrones de 4 bits,
otro ejercicio natural de bitwise.

---

## Placas Arduino

Ambas son placas de **5 V** (ver la restricción de niveles al principio). Su
papel en este proyecto **no es sustituir al Cardputer**, sino hacer de
**dispositivo objetivo**: un equipo cuyo tráfico de bus (UART, I2C, SPI) generas
tú, sabiendo qué envía, para luego observarlo y analizarlo. También serán el
prototipo natural del enlace inter-MCU de [`../05-vision.md`](../05-vision.md),
que sigue **gateado al hito 6**.

### Arduino UNO

| Dato | Valor |
|------|-------|
| MCU | ATmega328P, 8 bits, 16 MHz |
| Flash / SRAM / EEPROM | 32 KB (0,5 KB bootloader) / 2 KB / 1 KB |
| Pines digitales | 14 (6 con PWM) |
| Entradas analógicas | 6 (ADC de 10 bits) |
| UART hardware | 1 (compartida con el USB) |
| Corriente por pin | 20 mA recomendados, 40 mA máximo absoluto |
| Alimentación | USB 5 V, o jack/Vin 7–12 V |

Mira el chip junto al USB: un **ATmega16U2** indica placa original o clon fiel;
un **CH340** indica un clon que en Linux usa el driver `ch341` (incluido en el
kernel).

### Arduino Mega 2560

| Dato | Valor |
|------|-------|
| MCU | ATmega2560, 8 bits, 16 MHz |
| Flash / SRAM / EEPROM | 256 KB (8 KB bootloader) / 8 KB / 4 KB |
| Pines digitales | 54 (15 con PWM) |
| Entradas analógicas | 16 |
| UART hardware | 4 |

Sus 4 UART hardware lo hacen cómodo para tener una consola por USB y, a la vez,
otra UART libre hablando con otro dispositivo.

### Placa de extensión — sin identificar

Sin serigrafía no puedo identificarla. Las dos candidatas habituales en estos
kits son:

| Candidata | Cómo reconocerla |
|-----------|------------------|
| **Prototype shield** (ProtoShield) | Mismo tamaño que el UNO, con pines macho por debajo para encajar encima, zona de agujeros para soldar y a veces una mini protoboard pegada |
| **Módulo de alimentación de protoboard** (tipo MB102) | Placa pequeña con jack de barril, conector USB, interruptor y jumpers para elegir 3,3 V o 5 V; se pincha en los raíles de la protoboard |

Pendiente: describe forma, conectores y cualquier texto impreso para cerrarla.

### Clip de pila de 9 V

Conector para pila de 9 V, normalmente terminado en un jack de barril para el
UNO/Mega (o en cables sueltos para Vin). El regulador del Arduino baja 9 V a 5 V
disipando la diferencia como calor, y una pila de 9 V tiene poca capacidad
(~500 mAh): vale para que el Arduino funcione suelto, **no para motores ni
servos**.

---

## Resistencias

Valores habituales en estos kits: 10 Ω, 100 Ω, 220 Ω, 330 Ω, 1 kΩ, 2 kΩ,
5,1 kΩ, 10 kΩ, 100 kΩ, 1 MΩ. Se identifican por **código de colores**:

| Color | Dígito | Multiplicador | Tolerancia |
|-------|--------|---------------|------------|
| Negro | 0 | ×1 | — |
| Marrón | 1 | ×10 | ±1 % |
| Rojo | 2 | ×100 | ±2 % |
| Naranja | 3 | ×1 k | — |
| Amarillo | 4 | ×10 k | — |
| Verde | 5 | ×100 k | ±0,5 % |
| Azul | 6 | ×1 M | ±0,25 % |
| Violeta | 7 | ×10 M | ±0,1 % |
| Gris | 8 | — | — |
| Blanco | 9 | — | — |
| Dorado | — | ×0,1 | ±5 % |
| Plateado | — | ×0,01 | ±10 % |

**4 bandas** (cuerpo beige, carbón, ±5 %): dígito, dígito, multiplicador,
tolerancia. **5 bandas** (cuerpo azul, película metálica, ±1 %): dígito, dígito,
dígito, multiplicador, tolerancia. La banda de tolerancia está algo separada de
las demás: se lee empezando por el extremo opuesto. Ejemplo: rojo-rojo-marrón-
dorado = 22 × 10 = 220 Ω ±5 %. En caso de duda, el multímetro manda.

---

## Herramientas que faltan en la lista

Para cerrar los "pendiente de comprobar" de este documento hace falta un
**multímetro** (continuidad y modo diodo): ¿lo tienes? También conviene confirmar
que el kit trae **cables Dupont** (macho-macho y macho-hembra). El kit no incluye
**transistores ni diodos sueltos**, lo que limita conmutar cargas desde un GPIO
fuera de lo que cubre el ULN2003.

## Mapa hacia el roadmap

Solo orientación; el roadmap manda.

| Hito / capa | Piezas del kit que encajan |
|-------------|----------------------------|
| Práctica de GPIO y bitwise (cualquier momento) | LEDs, pulsadores, 74HC595, 5611AH |
| Hito 4 — IR | VS1838B, emisor IR, mando NEC |
| Análisis de buses (hitos de nivel 4) | Arduino UNO/Mega como generador de tráfico conocido, 74HC595, LCD 1602A |
| Detección de manipulación (blue team) | LDR, SW-520D |
| Enlace inter-MCU (gate: hito 6) | Arduino UNO/Mega como coprocesador de prueba |
