# Modelo de memoria: dónde vive una variable

Cómo reparte C++ las variables entre pila, secciones estáticas y heap en el
ESP32-S3, y qué significa eso al escribir código para el Cardputer. Aparece en el
hito 1 al necesitar un buffer que sobreviva a `loop()`.

Explicación visual larga (mapa de direcciones, arranque, ciclo de vida):
[¿Dónde vive una variable?](https://claude.ai/code/artifact/e8a15164-f15f-4874-991a-0975b6dece12).

Los números de este documento están **medidos** sobre
`.pio/build/cardputer/firmware.elf` del repo con las herramientas del toolchain
instalado, no tomados de un manual. Cómo reproducirlos: la última sección.

## Duración de almacenamiento

C++ no clasifica las variables por dónde se declaran sino por cuánto tiempo
existen (*storage duration*). Son tres, y la palabra clave decide cuál.

| Duración | Qué la produce | Dónde vive | Cuándo muere |
|----------|----------------|------------|--------------|
| Automática | Declaración dentro de una función | La pila | Al salir del bloque |
| Estática | Global, o local con `static` | `.data` o `.bss` | Nunca (hasta el reset) |
| Dinámica | `new`, `malloc()`, contenedores que crecen | El heap | Cuando se libera |

Una variable automática dentro de `loop()` **no es la misma variable** en la
vuelta siguiente: es otro objeto en la misma zona de pila, con el contenido que
dejara ahí la última función que pasó por allí. Por eso un buffer de línea que
deba acumular pulsaciones necesita duración estática.

`static` sobre una local cambia **solo la duración**, no la visibilidad: el
nombre sigue siendo inaccesible desde fuera de la función. Sobre una global es al
revés — cambia la visibilidad (la hace local a la unidad de traducción) y no la
duración, que ya era estática.

## Secciones del binario

| Sección | Contiene | ¿Ocupa flash? |
|---------|----------|---------------|
| `.text` | Instrucciones | Sí |
| `.rodata` | Constantes: literales de cadena, tablas `const` | Sí |
| `.data` | Estáticas con valor inicial **distinto de cero** | Sí |
| `.bss` | Estáticas a cero o sin inicializador | **No** |

La diferencia entre `.data` y `.bss` es ahorro de flash. Un valor inicial no
trivial hay que guardarlo en la imagen para poder copiarlo al arrancar; una
variable que empieza a cero no necesita que se guarde nada, basta con anotar
cuántos bytes hay que rellenar. Consecuencia directa: `static char linea[21];` no
hace crecer el firmware ni un byte, mientras que `static char linea[21] = "listo";`
ocupa 21 bytes de flash además de los 21 de RAM.

En el arranque, antes de `app_main()`, el startup del ESP-IDF copia `.data` de
flash a DRAM y rellena `.bss` de ceros. `.text` y `.rodata` no se copian: se
ejecutan y leen desde la flash, mapeada por la MMU (`.flash.text` está en
`0x42000020`, fuera del rango de SRAM interna).

## Mapa real de este firmware

Direcciones y tamaños de `firmware.elf`, DRAM ordenada de menor a mayor:

| Sección | Dirección | Tamaño |
|---------|-----------|--------|
| `.dram0.dummy` (relleno de alineación) | `0x3FC88000` | 45 276 B |
| `.dram0.data` | `0x3FC930E0` | 14 024 B |
| `.dram0.bss` | `0x3FC967A8` | 8 696 B |
| heap (resto de la DRAM) | `0x3FC989A0` | — |
| `.flash.text` | `0x42000020` | 277 327 B |
| `.flash.rodata` | `0x3C050120` | 111 128 B |

Dos lecturas que conviene fijar:

**El `RAM: 6.9%` que imprime PlatformIO son `.data` + `.bss` = 22 720 bytes,
exactamente.** No incluye pila ni heap, porque el enlazador no puede conocerlos.
El consumo real solo se ve en el dispositivo (`ESP.getFreeHeap()`).

**La pila de `loop()` sale del heap.** `loop()` no es el programa: es el cuerpo de
una tarea de FreeRTOS. En
`~/.platformio/packages/framework-arduinoespressif32/cores/esp32/main.cpp`,
`app_main()` la crea con `xTaskCreateUniversal(loopTask, "loopTask", ...)`, y el
tamaño de pila es `ARDUINO_LOOP_STACK_SIZE`, **8192 bytes** (línea 14 del mismo
archivo). Ese bloque se reserva del heap al crear la tarea. Es un presupuesto
cerrado y compartido por toda la profundidad de llamadas: un `char buffer[4096]`
local se come medio presupuesto. Pasarse no da un error de compilación, da un
reinicio en ejecución con `***ERROR*** A stack overflow in task loopTask has been
detected`.

## Por qué el heap se evita

Dos costes, ninguno visible en un PC. `malloc()` recorre la lista de bloques
libres, y ese recorrido no dura lo mismo en cada llamada: latencia no acotada
dentro de un `loop()` que quiere ser predecible. Y la **fragmentación**: reservar
y liberar bloques de tamaños distintos deja huecos, así que se puede tener 100 KB
libres y fallar al pedir 4 KB seguidos. No hay compactación. El síntoma es un
dispositivo que se cuelga tras días de captura, que es el fallo más caro de
diagnosticar.

De ahí el estilo: buffer de tamaño fijo dimensionado al caso peor, `static`, y un
índice de ocupación. Es la misma razón por la que el `KeysState` del teclado se
captura con `const auto&` — copiarlo llamaría al constructor de copia de sus
`std::vector`, una reserva de heap por tecla pulsada. Ver
[`04-protocols/gpio-keyboard.md`](04-protocols/gpio-keyboard.md).

## Cómo comprobarlo

Los ejecutables están en `~/.platformio/packages/toolchain-xtensa-esp32s3/bin/`.
No hace falta flashear: todo esto se lee del binario.

Símbolo a símbolo, sobre un `.o` suelto:

```
$ xtensa-esp32s3-elf-g++ -c -O0 -o caso.o caso.cpp
$ xtensa-esp32s3-elf-nm -S caso.o

00000000 00000037 T _Z9loop_demov
00000000 00000015 b _ZZ9loop_demovE5linea
00000015 00000001 b _ZZ9loop_demovE5largo
```

La tercera columna es la sección: `T`/`t` es `.text`, `D`/`d` es `.data`, `B`/`b`
es `.bss`, `R`/`r` es `.rodata`. **Minúscula significa símbolo local** a la unidad
de traducción, que es lo que hace `static` con la visibilidad. La segunda columna
es el tamaño: `0x15` = 21 bytes. Una variable automática **no aparece**: no tiene
símbolo porque no tiene dirección fija, solo un desplazamiento respecto al puntero
de pila.

Añadir un inicializador (`static char linea[21] = "x";`) mueve el símbolo de `b` a
`d`. Es la forma más rápida de ver la frontera `.bss`/`.data`.

Sección a sección, sobre el firmware entero:

```
$ xtensa-esp32s3-elf-size -A .pio/build/cardputer/firmware.elf
```

Con `-O2` el compilador elimina las variables que no se usan, así que para estas
inspecciones conviene `-O0`.

## Referencias

- *ESP32-S3 Technical Reference Manual*, § 4 «System and Memory»: 4.3.2 Internal
  Memory (rangos de DRAM/IRAM) y 4.3.3 External Memory (mapeo de flash por MMU).
  Enlace en [`references.md`](references.md).
- `cores/esp32/main.cpp` del framework: creación de `loopTask` y tamaño de pila.
  Ruta completa en [`03-libraries/fuentes-locales.md`](03-libraries/fuentes-locales.md).
