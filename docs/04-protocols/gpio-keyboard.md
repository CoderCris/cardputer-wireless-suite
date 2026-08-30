# Teclado matricial del Cardputer

Nota de protocolo del teclado (56 teclas). En nivel 1 lo maneja
`M5Cardputer.Keyboard`; el objetivo de este documento es entender el mecanismo
para poder reimplementarlo en nivel 2.

Explicación visual larga (circuito, bits paso a paso, mapa completo):
[De la tecla al char](https://claude.ai/code/artifact/a4577c2c-8525-4a58-bb54-b0689329acd3).

Todo lo de aquí sale de **leer el driver**, no de observar hardware. Fuente:
`.pio/libdeps/cardputer/M5Cardputer/src/utility/Keyboard/`, versión **1.1.1**.

## Por qué una matriz

56 teclas con un pin cada una serían 56 GPIO — imposible en el S3. La solución
estándar es una **matriz**: las teclas se organizan en filas y columnas, y en cada
cruce hay un pulsador. Se detecta qué tecla está pulsada **escaneando**: se activa
una línea de selección cada vez y se lee qué líneas de lectura responden.

Una tecla no emite nada: es un interruptor. Las líneas de lectura llevan
**pull-up interno** (`pinMode(pin, INPUT_PULLUP)`), así que en reposo leen `1`; al
cerrarse la tecla conectan con la línea de selección activa, que está a 0 V, y
caen a `0`. **Pulsada = 0**, y por eso `get_input()` invierte la lectura.

## Pines reales (Cardputer V1)

De `KeyboardReader/IOMatrix.h`:

| Función | GPIO | Modo |
|---------|------|------|
| Selección | 8, 9, 11 | `OUTPUT` — llevan un número binario de 3 bits (0–7) |
| Lectura | 13, 15, 3, 4, 5, 6, 7 | `INPUT_PULLUP` — reposo 1, pulsada 0 |

8 selecciones × 7 lecturas = 56 teclas con 10 GPIO.

Tres pines no pueden ser ocho líneas: alimentan un **decodificador 3→8** que baja
exactamente una de sus ocho salidas, la que corresponde al número recibido. Que
sus salidas sean activas a nivel bajo se deduce de cómo el driver interpreta las
lecturas.

> **Sin verificar**: el chip concreto. La referencia habitual para esta función es
> la familia 74HC138, pero hay que contrastarlo contra el esquemático oficial de
> M5Stack.

## Escaneo (implementación real)

`IOMatrixKeyboardReader::update()` borra la lista y la reconstruye entera en cada
llamada:

1. Para `i` de 0 a 7: `set_output(output_list, i)` — baja la línea `Yi`.
2. `get_input(input_list)` — lee los 7 pines y los empaqueta en un `uint8_t`,
   un bit por pin.
3. Para cada bit `j` puesto, traduce `(i, j)` a coordenada `(x, y)`.

Las tres operaciones bitwise que lo hacen todo: `&` con máscara para aislar un
bit, `<<` para colocar un bit en su posición, `|` para acumular sin borrar.

Nota de C: `set_output()` hace `digitalWrite(11, output & 0b100)`, es decir pasa
el valor **4**, no 1. En C cualquier valor distinto de cero es verdadero, así que
se escribe nivel alto igual. Es idiomático y confunde la primera vez.

Nota de nivel 2: `begin()` llama a `gpio_reset_pin()`, que es de ESP-IDF
(`driver/gpio.h`), mezclada con `pinMode()`. Arduino para ESP32 **es una capa
sobre ESP-IDF**, no un framework aparte.

## De (paso, bit) a (fila, columna)

El mapa de caracteres es 4×14, pero el barrido es 8×7. Cada paso cubre **media
fila**:

- Pasos 0–3 → columnas **impares** (`x = 2j + 1`)
- Pasos 4–7 → columnas **pares** (`x = 2j`)
- Fila: `y = 3 - (i mod 4)` — escrito en el driver como `-y` y luego `+3`, porque
  el orden de barrido es el inverso al orden visible en la carcasa.

Cada GPIO de lectura sirve dos columnas contiguas:

| GPIO | 13 | 15 | 3 | 4 | 5 | 6 | 7 |
|------|----|----|---|---|---|---|---|
| `j` | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
| columnas `x` | 0,1 | 2,3 | 4,5 | 6,7 | 8,9 | 10,11 | 12,13 |

Ejemplo completo: la tecla `a` está en `_key_value_map[2][2]`, o sea `(x 2, y 2)`.
Columna par → paso 4–7; `x = 2j` → `j = 1` → **GPIO 15**; `y = 2` → `i - 4 = 1` →
**paso 5**. Los tres pines de selección llevan `101` (G8=1, G9=0, G11=1) = 5.

## De coordenada a carácter

`const KeyValue_t _key_value_map[4][14]` en `Keyboard.h`: array 2D de structs con
`{value_first, value_second}` (sin y con shift). El `const` permite al compilador
emitirlo a **flash** en vez de a RAM — patrón obligado en embebidos.

`updateKeysState()` recorre la lista de teclas **dos veces**: la primera solo
anota modificadores (shift, ctrl, alt, fn, opt), la segunda traduce las
imprimibles ya con el estado completo. Sin las dos pasadas, `shift + a` daría
`a` o `A` según el orden en que el barrido encontrara las teclas.

## Lo que el driver NO hace

**No hay antirrebote.** `IOMatrixKeyboardReader::update()` no tiene temporizadores
ni historial: borra y republica lo que ve. (Este documento afirmaba lo contrario
antes; era falso.)

**`isChange()` solo compara el número de teclas**, no cuáles:

```cpp
uint8_t current_size = keyList().size();
if (_last_key_size != current_size) { _last_key_size = current_size; return true; }
return false;
```

Consecuencia reproducible: si sueltas una tecla en la misma vuelta en que pulsas
otra, el contador pasa de 1 a 1, no hay cambio, y **el carácter nuevo se pierde**.
Al escribir rápido ocurre.

## API actual (1.1.1) — `lastKeyCode()` ya no existe

La versión 1.1.1 reorganizó `Keyboard_Class`. El estado se consume vía
`keysState()`, que devuelve una referencia a un struct `KeysState` con, entre
otros campos, `std::vector<char> word`: los caracteres imprimibles pulsados en ese
instante, ya resueltos con shift/caps.

Es un **vector** y no un `char` porque el barrido encuentra todas las teclas
cerradas a la vez y no sabe cuál pulsaste "última". La API antigua elegía una;
esta devuelve lo que hay.

Además, `M5.begin()` / `M5.update()` **no tocan el teclado**: solo
`M5Cardputer::begin()` llama a `Keyboard.begin()`, y solo `M5Cardputer::update()`
llama a `updateKeyList()` + `updateKeysState()`.

## Ghosting — pendiente de comprobar en hardware

Toda matriz **sin diodos** sufre teclas fantasma. Si pulsas tres teclas que forman
tres esquinas de un rectángulo en la rejilla de barrido, la corriente encuentra un
camino de vuelta a través de ellas y la **cuarta esquina se lee como pulsada** sin
que nadie la toque. La solución estándar es un diodo en serie con cada tecla, que
impide ese retorno.

El rectángulo hay que buscarlo en coordenadas de **barrido** `(i, j)`, no en las
visibles: dos líneas de selección × dos líneas de lectura. Como las columnas pares
e impares las cubren mitades distintas del barrido, un rectángulo dibujado sobre
el teclado solo sirve si **las dos columnas tienen la misma paridad**.

Si el Cardputer V1 lleva diodos o no, **no está verificado**. Se comprueba sin
instrumental, solo flasheando y observando:

1. Rectángulo válido: `t` (x5,y1), `u` (x7,y1), `f` (x5,y2), `h` (x7,y2). En
   barrido son `(i2,j2)`, `(i2,j3)`, `(i1,j2)`, `(i1,j3)` — las cuatro esquinas de
   las líneas de selección 1 y 2 con las de lectura 2 y 3.
2. Pulsa **tres** a la vez: `t`, `u` y `f`.
3. Si aparece una `h` que no has tocado, **no hay diodos**.
4. Repite con otro rectángulo (p. ej. `q`,`e`,`SHIFT` → fantasma `s`) para
   descartar una coincidencia.

Si hay fantasmas, hay que tenerlo en cuenta al diseñar cualquier atajo de tres
teclas. Anota el resultado en
[`../journal/hito-01-terminal.md`](../journal/hito-01-terminal.md).

## Pendiente

- [x] Pines y mecanismo de selección.
- [x] Mapa de coordenadas → caracteres, incluyendo modificadores.
- [ ] **Comprobar el ghosting** con el procedimiento de arriba (flasheando).
- [ ] Confirmar el chip decodificador contra el esquemático de M5Stack.
- [ ] Nivel 2: escaneo de la matriz con `gpio_config()` + debounce propio.

Referencia TRM: capítulo *IO MUX and GPIO Matrix* (configuración de los pines de
lectura/selección).
