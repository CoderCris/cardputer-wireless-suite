# Dónde viven las fuentes (leer la librería como documentación)

La documentación de M5Stack es escasa y la de PlatformIO no cubre las librerías.
La fuente autoritativa de **qué firma tiene una función y qué campos tiene un
struct** no es esta carpeta ni un README de GitHub: es el `.h` que el compilador
está leyendo en tu disco, en la versión exacta que PlatformIO ha resuelto.

Este documento dice **dónde está ese fichero** y **cómo encontrar una declaración
dentro de él**. El resto de `docs/` cita rutas relativas a las raíces que se
definen aquí; esta es la única fuente de verdad para las rutas absolutas.

> **Estado verificado el 2026-09-07** en esta máquina, con la extensión
> `platformio.platformio-ide` 3.3.4 instalada. Todas las rutas de este documento
> están comprobadas contra el disco, no supuestas. El CLI **no queda en el
> `PATH`**: vive en `~/.platformio/penv/bin/pio`. Para usarlo desde una terminal:
>
> ```sh
> export PATH="$HOME/.platformio/penv/bin:$PATH"
> ```

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
├── M5Cardputer/     1.1.1   <- m5stack/M5Cardputer
├── M5Unified/       0.2.21  <- transitiva
├── M5GFX/           0.2.28  <- transitiva
├── IRremote/        4.7.1   <- transitiva
└── integrity.dat            <- control interno de PlatformIO, no lo toques
```

Solo `M5Cardputer` sale de `platformio.ini`. Las otras tres son **dependencias
transitivas**: M5Cardputer las declara en su propio `library.json` y PlatformIO
las arrastra. Dos consecuencias que importan:

- `M5GFX` es donde vive de verdad todo lo que hace `M5.Display`. M5Unified solo
  expone la fachada.
- **`IRremote` ya está en tu disco**, sin que la hayas pedido, porque M5Cardputer
  la usa para el emisor IR de la placa. En el hito 4 no partirás de cero: partirás
  de decidir si la usas o la reimplementas sobre RMT.

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

| Paquete | Ruta | Versión resuelta | Qué es |
|---------|------|------------------|--------|
| Core de Arduino para ESP32 | `~/.platformio/packages/framework-arduinoespressif32/` | `3.20017` = core **2.0.17** | `pinMode`, `digitalWrite`, `Serial`, el `main` que llama a tu `setup()`/`loop()` |
| Toolchain | `~/.platformio/packages/toolchain-xtensa-esp32s3/` | `8.4.0+2021r2-patch5` | Compilador (`xtensa-esp32s3-elf-gcc`), `binutils`, headers de la libc |
| Definición de plataforma | `~/.platformio/platforms/espressif32/` | `7.1.1` | `boards/m5stack-stamps3.json`: flash, RAM, flags base de la placa |

El número del core engaña: `3.20017.241212` **no** es la serie 3.x. El esquema es
`3` (formato de empaquetado) + `2.00.17` (la versión real de Arduino-ESP32). Estás
en el core **2.0.17**, no en el 3.

### Dónde están los headers de ESP-IDF

Verificado en el core 2.0.17:

```
~/.platformio/packages/framework-arduinoespressif32/
├── cores/esp32/          <- la capa Arduino: digitalWrite() y compañía
├── libraries/            <- WiFi.h, SD.h, SPI.h... el "Arduino" de alto nivel
├── variants/m5stack_stamp_s3/   <- pins_arduino.h de esta placa
└── tools/sdk/esp32s3/include/   <- ESP-IDF, precompilado, un dir por componente
```

Dos trampas al buscar ahí:

1. Hay un `tools/sdk/` **por SoC** (`esp32`, `esp32s2`, `esp32s3`, `esp32c3`). Un
   `find` sin filtrar te devuelve cuatro copias del mismo header. La tuya es
   `esp32s3`.
2. La ruta lleva `include` **dos veces**: componente y luego su carpeta pública.
   Los dos que vas a necesitar:

```
tools/sdk/esp32s3/include/driver/include/driver/gpio.h
tools/sdk/esp32s3/include/esp_wifi/include/esp_wifi_types.h
```

> **ESP-IDF 4.4**, no 5.x. Comprobado en
> `tools/sdk/esp32s3/include/esp_common/include/esp_idf_version.h`
> (`ESP_IDF_VERSION_MAJOR 4`, `MINOR 4`). Esto **no es un detalle menor**: entre
> 4.4 y 5.x Espressif reescribió la API de RMT (`driver/rmt.h` pasó a
> `driver/rmt_tx.h` / `rmt_rx.h`) y renombró la de I2S. Los ejemplos que
> encuentres para 5.x **no compilarán** aquí. Cuando consultes el ESP-IDF
> Programming Guide, fija la versión a `v4.4` en el selector — la portada por
> defecto es `latest`. Afecta a los hitos 3, 4 y 7.

En el core 3.x esta ubicación cambia: ESP-IDF se movió a un paquete de librerías
precompiladas aparte. Si algún día actualizas el core, esta sección se queda
obsoleta — revalida la ruta con el `find` de abajo y actualiza la versión de arriba.

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

Nada de esto necesita el Cardputer conectado. En esta máquina ya está hecho; queda
anotado para reproducirlo en otra o tras un `git clone` limpio.

**1. Instalar PlatformIO.** Dos vías, equivalentes en cuanto a las rutas de arriba:

```sh
pipx install platformio        # CLI independiente
```

o la extensión `platformio.platformio-ide` de VSCode — que es la vía usada aquí.
Trae su propio Python, y **el CLI no queda en el `PATH`**: lo deja en
`~/.platformio/penv/bin/pio`. Al abrir el proyecto, la extensión resuelve las
dependencias por su cuenta, así que `.pio/libdeps/` puede aparecer sin que hayas
ejecutado nada.

**2. Descargar las dependencias sin compilar:**

```sh
pio pkg install       # resuelve lib_deps -> .pio/libdeps/, sin construir nada
```

`pio pkg install` solo descarga; `pio run` además compila y linka. Si lo único que
quieres es **leer** las librerías, `pio pkg install` basta.

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
