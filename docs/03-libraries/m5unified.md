# M5Unified / M5Cardputer — qué usas y qué abstrae

Librerías base del proyecto en la fase inicial (nivel 1). Declaradas en
`platformio.ini`:

```ini
lib_deps =
    m5stack/M5Unified@^0.2.2
    m5stack/M5Cardputer@^1.0.1
```

**M5Unified** es la capa de abstracción de M5Stack sobre Arduino-ESP32: unifica
display, energía, botones y buses para toda la familia de placas M5.
**M5Cardputer** añade lo específico del Cardputer, sobre todo el **teclado
matricial**.

> Meta a medio plazo: **quitar estas librerías** e inicializar el hardware a mano
> (nivel 2, hito 2 en adelante). Documentar aquí qué hace cada llamada es la
> preparación para poder reemplazarla con conocimiento de causa.

## API que usamos y qué oculta

### `M5.begin(cfg)`
Inicializa todos los periféricos detectados. Por debajo, entre otras cosas:
- Configura el **bus SPI** y arranca el driver del **display ST7789V2**.
- Inicializa el **AXP2101** por I2C (rieles de energía, carga de batería).
- Prepara el escaneo del **teclado**.

Es el `begin` "mágico" que en nivel 2 desmontarás en llamadas explícitas
(`spi_bus_initialize`, init del ST7789, driver I2C del AXP2101...). Si no
existiera, tendrías que hacer todo eso a mano antes de poder pintar un píxel.

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
Se llama en cada iteración del `loop()`. Refresca el estado interno: lee botones,
actualiza el gestor de energía, atiende eventos. Sin esto, el estado de entrada
queda congelado.

### `M5Cardputer.Keyboard`
Driver del teclado matricial (56 teclas). Métodos que usamos:

| Llamada | Qué hace |
|---------|----------|
| `isChange()` | ¿Cambió el estado del teclado desde la última lectura? |
| `isPressed()` | ¿Hay alguna tecla pulsada ahora? |
| `lastKeyCode()` | Código de la última tecla |

Por debajo escanea la **matriz** del teclado (selección de columna + lectura de
filas). El detalle del mecanismo va en
[`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md); en nivel 2
lo reimplementarás sin esta clase.

## Riesgo de depender de la librería

M5Unified te da velocidad a cambio de opacidad: no ves los buses ni el timing, y
das por sentado un `M5.begin()` que hace docenas de cosas. Para blue team y para
el aprendizaje, ese es justo el conocimiento que quieres recuperar. Por eso el
plan es usarla como andamio inicial y retirarla hito a hito.
