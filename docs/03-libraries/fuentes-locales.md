# Dónde viven las fuentes (leer la librería como documentación)

La documentación de M5Stack es escasa y la de PlatformIO no cubre las librerías.
La fuente autoritativa de **qué firma tiene una función y qué campos tiene un
struct** no es esta carpeta ni un README de GitHub: es el `.h` que el compilador
está leyendo en tu disco, en la versión exacta que PlatformIO ha resuelto.

Este documento dice **dónde está ese fichero** y **cómo encontrar una declaración
dentro de él**. El resto de `docs/` cita rutas relativas a las raíces que se
definen aquí; esta es la única fuente de verdad para las rutas absolutas.

> **Estado en esta máquina** (comprobado el 2026-09-07): PlatformIO **no está
> instalado** (`pio` no está en el `PATH`, no existe `~/.platformio`, la extensión
> `platformio.platformio-ide` no está en VSCode) y el proyecto **no tiene `.pio/`**.
> Ninguna de las rutas locales de abajo existe todavía. La sección
> [Cómo hacer aparecer las fuentes](#cómo-hacer-aparecer-las-fuentes) explica cómo
> materializarlas — no hace falta el Cardputer para eso.

## Las dos raíces

PlatformIO separa lo que es **del proyecto** de lo que es **de la máquina**.

| Raíz | Qué contiene | Versionado |
|------|--------------|-----------|
| `<repo>/.pio/` | Dependencias resueltas de *este* proyecto y artefactos de build | En `.gitignore` |
| `~/.platformio/` | Plataformas, toolchains y frameworks compartidos por todos tus proyectos | Fuera del repo |

Que `lib_deps` se resuelva **dentro del proyecto** es deliberado: dos proyectos
pueden usar versiones distintas de M5Unified sin pisarse. Por eso la respuesta a
"¿qué versión de `Keyboard.h` estoy leyendo?" depende del proyecto, no del sistema.

Confirma ambas raíces en tu máquina con `pio system info` (imprime *core dir* y
*project dir*), no des por buenas las de este documento.

## Librerías del proyecto (`lib_deps`)

Cada entrada de `lib_deps` se descarga a una carpeta con el **nombre de la
librería**, dentro del entorno declarado en `platformio.ini` (aquí `[env:cardputer]`):

```
<repo>/.pio/libdeps/cardputer/
├── M5Cardputer/     <- m5stack/M5Cardputer
├── M5Unified/       <- m5stack/M5Unified
└── M5GFX/           <- dependencia transitiva: la arrastra M5Unified
```

`M5GFX` no está en `platformio.ini` y aun así aparece: es una **dependencia
transitiva** (M5Unified la declara en su propio `library.json`). Es donde vive de
verdad todo lo que hace `M5.Display`.

Dentro de cada una, el código está bajo `src/`. Puntos de entrada que ya se citan
en el resto de la documentación:

| Qué buscas | Fichero | Documentado en |
|------------|---------|----------------|
| Fachada del Cardputer, `begin()` / `update()` | `M5Cardputer/src/M5Cardputer.h` y `.cpp` | [`m5unified.md`](m5unified.md) |
| Driver del teclado, `KeysState`, mapa de teclas | `M5Cardputer/src/utility/Keyboard/Keyboard.h` y `.cpp` | [`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md) |
| Barrido de la matriz, pines reales | `M5Cardputer/src/utility/Keyboard/KeyboardReader/IOMatrix.h` y `.cpp` | idem |
| Clase base polimórfica de los lectores | `M5Cardputer/src/utility/Keyboard/KeyboardReader/KeyboardReader.h` | [`../deuda-conceptual.md`](../deuda-conceptual.md) |
| Fachada M5, `M5.begin()`, `M5.config()` | `M5Unified/src/M5Unified.hpp` | [`m5unified.md`](m5unified.md) |
| Botones, energía, altavoz, micrófono | `M5Unified/src/utility/` | [`m5unified.md`](m5unified.md) |
| Display: `print`, `fillScreen`, envío SPI | `M5GFX/src/` | [`../04-protocols/spi-st7789.md`](../04-protocols/spi-st7789.md) |

Los nombres de fichero dentro de `utility/` cambian entre versiones. Trátalos como
punto de partida y confirma con `find`, no los copies a ciegas.

## Framework, toolchain y plataforma

Bajo `~/.platformio/`, tres paquetes importan:

| Paquete | Ruta | Qué es |
|---------|------|--------|
| Core de Arduino para ESP32 | `~/.platformio/packages/framework-arduinoespressif32/` | `pinMode`, `digitalWrite`, `Serial`, el `main` que llama a tu `setup()`/`loop()` |
| Toolchain | `~/.platformio/packages/toolchain-xtensa-esp32s3/` | Compilador (`xtensa-esp32s3-elf-gcc`), `binutils`, headers de la libc |
| Definición de plataforma | `~/.platformio/platforms/espressif32/` | `boards/m5stack-stamps3.json`: flash, RAM, flags base de la placa |

Dentro del core de Arduino, dos sitios:

- `cores/esp32/` — la implementación de la capa Arduino. Aquí ves que
  `digitalWrite()` no es magia: es una función C que acaba llamando a ESP-IDF.
- Los **headers de ESP-IDF** (`driver/gpio.h`, `esp_wifi.h`, `esp_wifi_types.h`...)
  van empaquetados con el core, pero su ubicación **cambió entre la serie 2.x y la
  3.x** del core (en 2.x colgaban de `tools/sdk/esp32s3/include/`; en 3.x se
  movieron a un paquete de librerías precompiladas aparte). No memorices la ruta:
  localízala con el `find` de abajo la primera vez y anótala aquí.

Esos headers son los que vas a necesitar de verdad a partir del hito 3
(`esp_wifi_set_promiscuous_rx_cb` y la firma de su callback viven en
`esp_wifi.h` / `esp_wifi_types.h`).

## Cómo encontrar una declaración

Tres recetas. La segunda es la que usarás el 90% de las veces.

**Localizar el fichero que declara algo**, sin saber dónde está:

```sh
grep -rn --include=*.h --include=*.hpp "keysState" .pio/libdeps/ ~/.platformio/packages/
```

**Ver la firma exacta de un método** y su contexto:

```sh
grep -rn -A3 "KeysState& keysState" .pio/libdeps/cardputer/M5Cardputer/src/
```

`-n` da el número de línea (para poder citarlo como `fichero:línea`), `-A3` añade
las tres líneas siguientes, que suelen ser el cuerpo o los parámetros por defecto.

**Ver la estructura de una librería** antes de bucear:

```sh
find .pio/libdeps/cardputer/M5Cardputer/src -name '*.h' | sort
find ~/.platformio/packages/framework-arduinoespressif32 -name 'esp_wifi_types.h'
```

Regla al leer: **la declaración manda sobre el ejemplo**. Un `.cpp` de ejemplo en
`examples/` puede estar escrito para una versión anterior de la API — es
exactamente lo que pasó con `lastKeyCode()`, que desapareció en M5Cardputer 1.1
y sigue apareciendo en ejemplos y tutoriales por internet. El `.h` instalado no
miente.

## Qué versión estás leyendo

`platformio.ini` declara un **rango**, no una versión. `^1.0.1` autoriza cualquier
1.x, y lo que se ha resuelto puede ser 1.1.1 con la API reorganizada. Para saber
qué hay realmente en disco:

```sh
pio pkg list          # versiones resueltas de lib_deps en este proyecto
pio pkg outdated      # qué tiene versión más nueva disponible
```

Y en la propia librería, `library.json` (o `library.properties`) lleva el campo
`version`. Ese es el número que debes citar cuando documentes algo leído del
código, como hace [`gpio-keyboard.md`](../04-protocols/gpio-keyboard.md) al
fijar "versión 1.1.1".

## Cómo hacer aparecer las fuentes

Nada de esto necesita el Cardputer conectado. Son dos pasos.

**1. Instalar PlatformIO.** Dos vías, equivalentes en cuanto a las rutas de arriba:

```sh
pipx install platformio        # CLI independiente (recomendado en Kali)
```

o instalar la extensión `platformio.platformio-ide` en VSCode, que trae su propio
Python y deja el CLI en `~/.platformio/penv/bin/pio`. Si usas esa vía, añade esa
ruta al `PATH` o invócalo con la ruta completa.

**2. Descargar las dependencias sin compilar:**

```sh
pio pkg install       # resuelve lib_deps -> .pio/libdeps/, sin construir nada
```

`pio pkg install` solo descarga; `pio run` además compila y linka (y baja el
toolchain y el framework, que son cientos de MB, la primera vez). Si lo único que
quieres es **leer** las librerías de `lib_deps`, `pio pkg install` basta.

**Alternativa sin PlatformIO**: clonar los repos y leerlos ahí, fijando el tag de
la versión que te interesa. Sirve para leer, no para compilar:

```sh
git clone --branch 1.1.1 --depth 1 https://github.com/m5stack/M5Cardputer
```

Los URLs de los tres repos están en [`../references.md`](../references.md).

## Mantenimiento

Cuando confirmes una ruta que aquí está marcada como variable (los headers de
ESP-IDF dentro del core, los nombres bajo `utility/`), **anótala aquí con la
versión** en la que la comprobaste. Es el tipo de dato que se pudre en silencio: la
ruta sigue existiendo en tu máquina y deja de existir en la del siguiente
`pio pkg update`.
