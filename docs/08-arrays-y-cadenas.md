# Arrays y cadenas en C++ (para buffers de tamaño fijo)

Lo que hace falta para manipular un buffer estático sin romper nada. Aparece en el
hito 1 al implementar el buffer de línea de la terminal.

Complementa a [`07-memory-model.md`](07-memory-model.md): allí se explica *dónde*
vive el array; aquí, *qué se puede hacer* con él.

Los ejemplos de este documento están **ejecutados**, no citados de memoria. El
guion que produce las salidas se reproduce al final.

---

## Un array es un bloque contiguo y nada más

```c++
char linea[21];
```

Reserva 21 bytes seguidos. No hay cabecera, ni longitud guardada, ni comprobación
de límites: en tiempo de ejecución un array es una dirección y nada más. Todo lo
que el compilador sabe del tamaño lo sabe **en tiempo de compilación**, y solo
mientras el nombre esté a la vista.

De ahí salen las dos reglas que gobiernan todo lo demás:

1. **Un array nunca viaja solo.** O lo acompaña una longitud explícita
   (`linea` + `linea_n`), o un convenio de terminación (el `'\0'` de las cadenas).
   No hay tercera opción.
2. **Nadie te avisa si te sales.** Escribir en `linea[21]` compila, ejecuta y
   corrompe lo que haya detrás. Los límites los compruebas tú, antes de indexar.

Tres tamaños distintos que se confunden constantemente:

| Concepto | En el buffer de la terminal | Cómo se obtiene |
|----------|-----------------------------|-----------------|
| Capacidad | Cuántos caracteres útiles admites (20, 64...) | Constante que eliges tú |
| Tamaño reservado | Bytes del array (capacidad + 1 por el `'\0'`) | `sizeof(linea)`, solo donde el array es visible |
| Longitud | Cuántos caracteres válidos hay ahora | Tu contador `linea_n`, o `strlen()` |

```
  sizeof(linea) = 21   (capacidad reservada, incluido el hueco del \0)
  strlen(linea) = 4    (caracteres hasta el primer \0), con linea = "hola"
```

## Indexar es aritmética de punteros

`a[i]` es azúcar sintáctico exacto de `*(a + i)`: coge la dirección base, avanza
`i` elementos (no `i` bytes: `i * sizeof(elemento)`), y desreferencia. Es
literalmente conmutativo, cosa que demuestra que no hay magia:

```
  linea[2] = l   *(linea+2) = l   2[linea] = l
```

El rango válido de `char linea[21]` es `linea[0]` a `linea[20]`. El índice 21 no
existe. Casi todo fallo con arrays es un **fuera-por-uno**: confundir *cuántos
hay* con *cuál es el último índice*, que siempre se diferencian en uno.

Un puntero al elemento *siguiente al último* (`linea + 21`) sí es legal de
calcular y comparar — es el patrón `begin`/`end` —, pero desreferenciarlo no.

## Al pasarlo a una función, el tamaño se pierde

```
  sizeof(linea) fuera de la funcion  = 21
  sizeof(a)     dentro, con char a[] = 8    <- en el PC (puntero de 64 bits)
```

Esto se llama *decaimiento* (*array decay*): en la firma, `char a[]` y `char a[21]`
significan **exactamente lo mismo que `char* a`**. El compilador convierte el
array en un puntero a su primer elemento y el tamaño se queda fuera. En el
ESP32-S3 ese `8` sería `4`, porque el puntero es de 32 bits; el punto es que en
ningún caso es 21.

Consecuencia práctica: toda función que reciba un buffer necesita **también** su
longitud o su capacidad. Por eso `procesar_vuelta(const char* word, uint8_t
word_n)` lleva dos parámetros, y no por capricho.

## Cadenas C: el convenio del terminador

Una "cadena de C" no es un tipo: es un array de `char` con el acuerdo de que el
primer `'\0'` marca el final. Todo lo que empieza por `str` en `<cstring>` depende
de ese acuerdo, y `printf("%s")` también: recorren memoria hasta encontrarlo. Si
no está, siguen leyendo lo que haya detrás.

```
  memcpy de 3 bytes ("abc") en un buffer relleno de '#', luego printf("%s"):
  -> "abc#####"     el memcpy no pone terminador; solo copia lo que le pides
```

`'\0'` (el byte 0) no es `'0'` (el carácter cero, byte 48). Y el terminador
**ocupa sitio**: por eso el array de una línea de 20 caracteres se declara de 21.

Si además llevas la longitud aparte, como en el buffer de la terminal, el
terminador es redundante para ti — lo mantienes solo para poder entregar el buffer
a algo que espere `const char*`, como `M5.Display.print()`, sin copiarlo antes.

## Operaciones

Los arrays no se asignan ni se comparan con los operadores del lenguaje. `a = b`
no compila para arrays, y `a == b` compila pero compara **direcciones**, no
contenido:

```
  strcmp(x, y) == 0 ? si    (contenido igual)
  x == y ?            no    (direcciones distintas)
```

Lo que sí existe, todo en `<cstring>`, y todo sin comprobar límites:

| Función | Qué hace | Trampa |
|---------|----------|--------|
| `memset(dst, byte, n)` | Rellena `n` bytes con un valor | El segundo argumento es un byte, no un patrón |
| `memcpy(dst, src, n)` | Copia `n` bytes | **No** copia el `'\0'` salvo que lo cuentes en `n`. Si las zonas se solapan, comportamiento indefinido |
| `memmove(dst, src, n)` | Igual, pero correcto si se solapan | Es la que sirve para desplazar dentro del mismo buffer |
| `memcmp(a, b, n)` | Compara `n` bytes | Devuelve 0 si son iguales (no `true`) |
| `strlen(s)` | Longitud hasta el `'\0'` | Recorre la cadena entera: O(n). No la llames dentro de un bucle sobre lo mismo |
| `strcmp(a, b)` | Compara cadenas | 0 = iguales. Necesita terminador en las dos |
| `snprintf(dst, size, fmt, ...)` | Formatea con límite | `size` **incluye** el terminador. Siempre termina la cadena. Es la forma segura por defecto |

`strcpy` y `strcat` no aparecen en la tabla a propósito: no tienen límite y son la
fuente clásica de desbordamiento. Donde tengas la tentación, usa `snprintf`.

Copiar un array elemento a elemento con un `for` es perfectamente válido y a veces
más claro; `memcpy` solo es más rápido porque copia en palabras de 4 bytes en vez
de byte a byte.

## Insertar y borrar en el medio

Añadir o quitar al **final** es O(1): escribes en `linea[linea_n]` y ajustas el
contador. Hacerlo en **el medio** obliga a desplazar todo lo que viene detrás,
porque los elementos son contiguos por definición:

```
  insertar ',' en el índice 4 de "hola mundo"

  antes:  h  o  l  a  _  m  u  n  d  o  \0
          0  1  2  3  4  5  6  7  8  9  10

  memmove(buf+5, buf+4, len-4+1)   <- el +1 arrastra también el \0
          h  o  l  a  _  _  m  u  n  d  o  \0
                         ^ hueco abierto en el 4... (se desplazó todo)

  buf[4] = ','
  ->  "hola, mundo"
```

Es O(n) por operación. Con 64 caracteres da igual; el motivo de mencionarlo es que
esto es lo que aparece el día que quieras un cursor movible con las flechas: la
inserción deja de ser una escritura y pasa a ser un desplazamiento, y `linea_n`
deja de servir como punto de inserción porque ya no coincide con el cursor.

## Enteros: las dos trampas que muerden en buffers

**Sin signo no baja de cero.** Un contador sin signo que se decrementa en cero da
la vuelta al máximo del tipo, en silencio:

```
  uint8_t  n = 0; n--;  ->  n = 255
  uint16_t m = 0; m--;  ->  m = 65535
```

Por eso `borrar()` comprueba `linea_n > 0` **antes** de restar, y por eso un
`uint8_t` no sirve como índice de un buffer de más de 255: la comprobación
`linea_n < CAPACIDAD` nunca llegaría a ser falsa.

**Las expresiones promocionan a `int`.** Los tipos menores que `int` se convierten
a `int` antes de operar, así que la cuenta se hace bien y el destrozo ocurre al
guardar:

```
  uint8_t a = 200, b = 100;
  a + b evaluado en int      -> 300
  a + b guardado en uint8_t  -> 44
```

Aparte, comparar un índice `int` con algo `size_t` (lo que devuelve `.size()` de
un `std::vector`) mezcla con y sin signo, y el compilador avisa con
`-Wsign-compare`. Ocurre hoy en `src/main.cpp:68` y `src/main.cpp:83`, con
`i < estado.word.size()`. Aquí es inofensivo porque `i` nunca es negativo, pero el
arreglo correcto es declarar el índice del mismo tipo que el límite.

## Por qué aquí no se usa `std::string` ni `std::vector`

Ambos resuelven todo lo anterior — crecen solos, guardan su longitud, comparan por
contenido — a cambio de reservar en el **heap**. En un MCU eso significa
fragmentación y latencia no determinista, precisamente en el bucle que tiene que
responder a cada pulsación. El motivo largo, con los números de este firmware, en
[`07-memory-model.md`](07-memory-model.md#por-qué-el-heap-se-evita).

Que el driver del teclado sí use `std::vector<char>` para `word` no es una
contradicción: ese vector se rellena y se vacía en cada `update()`, con un tamaño
que rara vez pasa de tres o cuatro elementos, y la decisión la tomó la librería,
no tú. Tus buffers propios van en `.bss`, con tamaño fijo.

## Reproducir las salidas

Todos los bloques de este documento salen de un único guion ejecutable:

```
g++ -Wall -Wextra -o /tmp/arrays sandbox/arrays.cpp && /tmp/arrays
```

Es un banco de pruebas del PC, no código del firmware: PlatformIO solo compila
`src/`.

## Referencias

- `cppreference`, *Array declaration* y *String library*: <https://en.cppreference.com/w/cpp/language/array>
- Modelo de memoria del proyecto: [`07-memory-model.md`](07-memory-model.md)
- Dónde están las cabeceras del toolchain instalado:
  [`03-libraries/fuentes-locales.md`](03-libraries/fuentes-locales.md)
