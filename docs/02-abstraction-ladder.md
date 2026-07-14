# Escalera de abstracción

El método de aprendizaje del proyecto: bajar por niveles, **nunca saltando**, y no
subir de nivel hasta dominar el actual. Cada herramienta del roadmap se ataca
primero en el nivel más alto que la resuelva, y luego se re-implementa más abajo
para entender qué ocultaba la capa anterior.

## Los cuatro niveles

### Nivel 1 — Arduino-like
APIs M5Stack/Arduino: `M5.Display.print()`, `digitalWrite()`, `Serial.print()`.
La librería inicializa y habla con los periféricos por ti. Aquí aprendes el
**flujo**: editar → compilar → flashear → observar. No hay que entender el
hardware todavía, solo el ciclo de trabajo.

### Nivel 2 — ESP-IDF / HAL
Funciones del framework nativo: `gpio_config()`, `spi_bus_initialize()`,
`i2s_channel_init()`, tasks de FreeRTOS, interrupts. Ya no hay `M5.begin()` mágico:
tú inicializas cada periférico y entiendes qué hacía Arduino por debajo. Aquí
aparece el modelo real del SoC (drivers, handles, configuración de buses).

### Nivel 3 — Registros mapeados en memoria
Acceso directo a los registros del periférico: `GPIO_OUT_REG`, registros de SPI,
RTC, etc. Escribir un valor a una dirección de memoria y observar el efecto físico.
Aquí entiendes que "una función de ESP-IDF" es, al final, una escritura a un
registro documentado en el TRM.

### Nivel 4 — Protocolo/bus sin librería
Implementar el protocolo tú mismo: SPI/I2C/IR por bit-banging o manipulando el
periférico directamente, controlando el timing y la trama. Máximo control, máxima
comprensión. Aquí un error de microsegundos se ve en el osciloscopio (o en que el
periférico no responde).

## Regla de progresión

**No subir de nivel hasta demostrar dominio del actual.** El criterio de dominio
no es "funciona", es **poder explicar qué hace cada línea a nivel de hardware**.
Si puedes reescribir el bloque un nivel más abajo sin ayuda, dominas el nivel
actual.

## Cómo se mapea sobre el roadmap

Un mismo objetivo se recorre en varios niveles. Ejemplo del display:

1. **Nivel 1**: `M5.Display.println("hola")` — la librería lo hace todo.
2. **Nivel 2**: inicializar el bus SPI y el driver ST7789 con ESP-IDF, enviar
   comandos de inicialización tú mismo.
3. **Nivel 3**: escribir a los registros del controlador SPI para enviar un byte.
4. **Nivel 4**: bit-banguear el protocolo del ST7789 moviendo CS/DC/SCLK/MOSI a
   mano y respetando el timing del datasheet.

El [`roadmap.md`](roadmap.md) indica, por hito, en qué nivel(es) se trabaja. El
hito 6 (reescritura de WiFi de Arduino a ESP-IDF) es literalmente "coger una
herramienta ya hecha en nivel 1-2 y bajarla de nivel".

## Anti-patrón a evitar

Copiar código de nivel 4 (registros, bit-banging) sin haber pasado por los niveles
1-3 del mismo periférico. Funcionaría, pero no habrías aprendido nada: es
exactamente lo que hace flashear firmware de terceros, y el objetivo del repo es
lo contrario.
