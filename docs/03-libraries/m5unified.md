# M5Unified / M5Cardputer — qué usas y qué abstrae

Librerías base del proyecto en la fase inicial (nivel 1). Declaradas en
`platformio.ini`:

```ini
lib_deps =
    m5stack/M5Unified@^0.2.2
    m5stack/M5Cardputer@^1.0.1
```

> **Ojo con el caret.** `^1.0.1` autoriza cualquier 1.x, y PlatformIO ha resuelto
> **M5Cardputer 1.1.1** (y M5Unified **0.2.18**). La 1.1 reorganizó
> `Keyboard_Class` y **eliminó `lastKeyCode()`**. Comprueba siempre la versión
> real con `pio pkg list`, no la del `lib_deps`.

**M5Unified** es la capa de abstracción de M5Stack sobre Arduino-ESP32: unifica
display, energía, botones y buses para toda la familia de placas M5.
**M5Cardputer** añade lo específico del Cardputer, sobre todo el **teclado
matricial**.

> Meta a medio plazo: **quitar estas librerías** e inicializar el hardware a mano
> (nivel 2, hito 2 en adelante). Documentar aquí qué hace cada llamada es la
> preparación para poder reemplazarla con conocimiento de causa.

## API que usamos y qué oculta

### `M5.begin(cfg)`
Inicializa los periféricos que M5Unified conoce de la placa detectada. Por debajo,
entre otras cosas:
- Configura el **bus SPI** y arranca el driver del **display ST7789V2**.
- Inicializa el **AXP2101** por I2C (rieles de energía, carga de batería).
- Prepara botones, altavoz y micrófono.

Es el `begin` "mágico" que en nivel 2 desmontarás en llamadas explícitas
(`spi_bus_initialize`, init del ST7789, driver I2C del AXP2101...). Si no
existiera, tendrías que hacer todo eso a mano antes de poder pintar un píxel.

> **No inicializa el teclado.** El teclado matricial es específico del Cardputer y
> lo arranca `M5Cardputer.begin()`, que llama a `M5.begin()` y *además* a
> `Keyboard.begin()`. Si usas `M5.begin()` a secas, los 10 GPIO del teclado nunca
> se configuran.

### `M5.config()`
Devuelve una estructura de configuración con valores por defecto para la placa
detectada. Permite ajustar qué se inicializa antes de `M5.begin()`.

### `M5.Display` (objeto tipo LovyanGFX/M5GFX)
Fachada del display. Métodos que usamos:

| Llamada | Qué hace | Qué oculta |
|---------|----------|------------|
| `setTextSize(n)` | Escala del font (multiplicador entero) | Nada de hardware; es cálculo de render |
| `setTextColor(fg, bg)` | Color de texto y fondo | Formato de color RGB565 del display |
| `fillScreen(color)` | Rellena toda la pantalla | Envío de un frame completo por SPI al ST7789 |
| `setCursor(x, y)` | Posición del cursor | Coordenadas en el framebuffer |
| `print()` / `println()` | Escribe texto | Rasterización del font + envío por SPI |

El display trabaja con un **buffer en memoria (canvas)** que se compone en RAM y
se envía completo al ST7789 por SPI. Eso evita *flickering* (parpadeo): en vez de
escribir píxel a píxel al display, compones el frame entero y lo mandas de golpe
(double buffering simplificado).

### `M5.update()`
Se llama en cada iteración del `loop()`. Refresca el estado interno de M5Unified:
lee botones, actualiza el gestor de energía, atiende eventos. Sin esto, el estado
de entrada queda congelado.

> **Tampoco toca el teclado.** El barrido de la matriz lo dispara
> `M5Cardputer.update()`, que llama a `M5.update()` y luego a
> `Keyboard.updateKeyList()` (los 8 pasos del barrido) y
> `Keyboard.updateKeysState()` (traducción a caracteres). Con solo `M5.update()`,
> la lista de teclas queda vacía para siempre y `isPressed()` devuelve 0.

### `M5Cardputer.begin(cfg)` / `M5Cardputer.update()`
Son las que hay que llamar en un Cardputer. Cada una envuelve a su equivalente de
M5Unified y le añade el teclado. Además, `M5Cardputer` expone `Display`, `Power`,
`Speaker` y `Mic` como **referencias** a los objetos de `M5`: son el mismo objeto,
no copias, así que `M5Cardputer.Display` y `M5.Display` son intercambiables.

### `M5Cardputer.Keyboard`
Driver del teclado matricial (56 teclas). Métodos que usamos:

| Llamada | Qué hace | Cuidado |
|---------|----------|---------|
| `isChange()` | ¿Cambió el **número** de teclas pulsadas desde la última lectura? | No compara *cuáles*. Ver abajo |
| `isPressed()` | Devuelve **cuántas** teclas hay pulsadas (`uint8_t`, no `bool`) | 0 = ninguna |
| `keysState()` | Referencia al struct `KeysState` con todo el estado del instante | Es la API real |
| `keysState().word` | `std::vector<char>` con los caracteres imprimibles pulsados, ya resueltos con shift/caps | Vector, no `char` |
| `isKeyPressed(c)` | ¿Está pulsada esa tecla concreta? | Útil para atajos |
| `keyList()` | Coordenadas físicas crudas (`Point2D_t`) de las teclas cerradas | Antes de traducir a carácter |

`lastKeyCode()` **ya no existe** en 1.1.1. Su sustituto es `keysState().word`, y
el cambio de `char` a vector no es capricho: el barrido encuentra todas las teclas
cerradas a la vez y no sabe cuál pulsaste "última".

Dos límites del driver que conviene tener presentes desde el principio: **no
aplica antirrebote** (*debounce*) en ninguna parte, e `isChange()` solo compara el
tamaño de la lista, así que soltar una tecla en la misma vuelta en que pulsas otra
**pierde el carácter nuevo**.

Por debajo escanea la **matriz** del teclado: 3 GPIO de selección hacia un
decodificador 3→8, 7 GPIO de lectura con pull-up. El detalle del mecanismo, los
pines y el mapa completo de teclas están en
[`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md); en nivel 2
lo reimplementarás sin esta clase.

## Riesgo de depender de la librería

M5Unified te da velocidad a cambio de opacidad: no ves los buses ni el timing, y
das por sentado un `M5.begin()` que hace docenas de cosas. Para blue team y para
el aprendizaje, ese es justo el conocimiento que quieres recuperar. Por eso el
plan es usarla como andamio inicial y retirarla hito a hito.
