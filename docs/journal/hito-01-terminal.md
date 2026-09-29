# Diario — Hito 1: Terminal interactiva

> Plantilla de diario de hito. Rellénala **tú** a medida que avanzas: es tu
> registro de decisiones y aprendizajes, no una spec. Un diario por hito.

**Objetivo**: entrada por teclado → eco a display. Aprender el ciclo
editar → compilar → flashear → observar. Nivel 1 (Arduino-like).

**Estado**: código completo y compilando; **pendiente de flashear y observar**.

## Qué construyo

Una terminal mínima: cada tecla pulsada se imprime en el display. Es el patrón
base entrada→proceso→salida que reutilizarán todos los hitos siguientes.

## Conceptos que toca

- Ciclo de trabajo de PlatformIO (ver [`../00-toolchain.md`](../00-toolchain.md)).
- Display ST7789 vía M5GFX (ver [`../04-protocols/spi-st7789.md`](../04-protocols/spi-st7789.md)).
- Teclado matricial vía M5Cardputer (ver [`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md)).
- El bucle `setup()` / `loop()` de Arduino y `M5.update()`.

## Decisiones tomadas

<!-- Ej: "Uso setTextSize(2) porque a tamaño 1 no se lee a 30cm." Anota el porqué. -->

**El buffer es la fuente de verdad; la pantalla es un reflejo.** El ST7789 no
sabe qué es un carácter: solo recibe píxeles. No existe "borra el último
carácter", así que un Backspace obliga a repintar la franja con el color de
fondo y volver a dibujar la línea entera desde el buffer. Eso descarta el modelo
de `print()` por carácter según llega.

**Se repinta la línea entera, no el hueco del carácter borrado.** Calcular el
rectángulo exacto de un carácter es posible (CASET/RASET + RAMWR), pero con 20
caracteres el ahorro no compensa la complejidad.

**Dirty flag en lugar de repintar en cada rama.** Mandar píxeles por SPI es lo
más caro del loop. `insertar()` y `borrar()` devuelven `bool` (si el estado
cambió de verdad), el loop acumula con `|=` y repinta una sola vez al final.
Escribir con el buffer lleno o borrar en vacío no dispara nada.

**Backspace y Enter necesitan su propio detector de flancos.** No están en
`estado.word`: son flags (`estado.del`, `estado.enter`) que valen `true`
mientras la tecla siga pulsada. Sin flanco propio, mantener Enter medio segundo
entrega una línea por vuelta de `loop()`. El estado anterior se actualiza
**después** de comparar; al revés, el flanco no se detecta nunca.

**`confirmar()` repinta antes de bajar de fila.** El barrido puede ver un
carácter y el Enter en la misma vuelta. Si `confirmar()` confiara en el
repintado de la vuelta anterior, ese último carácter se perdería en pantalla
aunque estuviera en el buffer.

**Al llegar abajo se limpia la pantalla y se empieza arriba.** Desplazar una
fila exigiría leer de vuelta la GRAM del ST7789 (el Cardputer no cablea MISO, no
se puede) o mantener el historial de líneas en RAM. Lo segundo es lo correcto y
llega en el hito 2 con el store; hasta entonces, limpiar.

**`prompt_y` sale de `getCursorY()`, no de un número de píxeles.** Así sobrevive
a un cambio de `setTextSize()`.

**`MAX_TECLAS = 4`**, con su consecuencia asumida: con más de cuatro teclas
imprimibles mantenidas a la vez, las que no caben en el historial de flancos
repiten carácter en cada vuelta.

**Se saltó el contrato de mentoría** (`CLAUDE.md`) para la integración final en
`main.cpp`: la lógica del buffer ya estaba resuelta por mí en el sandbox y el
resto era cableado. El código de `src/main.cpp` lo escribió el agente; los
esqueletos previos y `sandbox/` no.

## Qué falló y cómo lo resolví

<!-- Errores de compilación, comportamiento inesperado en hardware, timing, etc.
     Esta sección es oro para tu yo futuro. -->

**`linea_n` es `uint8_t`: restar en vacío no da -1, da 255.** El array tiene 21
bytes; escribir el terminador en `linea[255]` pisa lo que haya al lado. De ahí
la guarda `if (linea_n == 0) return false;` antes del decremento.

**Fuera-por-uno en el límite del buffer.** Con `CAPACIDAD = 20`, el último
índice válido para un carácter es el 19 y `linea[20]` es el terminador. El banco
de pruebas de `sandbox/buffer.cpp` lo cubre con un canario (eventos 7 a 11) y
pasa los 12 casos.

**Verificación disponible**: banco de pruebas en el PC (12/12) y `pio run`
(compila y linka, RAM 6.9%, flash 7.1%). Ninguna de las dos dice nada sobre el
comportamiento en el dispositivo.

## Preguntas abiertas

<!-- Cosas que aún no entiendo del todo y quiero retomar al bajar de nivel. -->
- ¿Qué hace exactamente `M5.begin()` por debajo, paso a paso? (se desmonta en hito 2)
- ~~¿Cómo distingue el driver una pulsación real de un rebote?~~ **Resuelto: no lo
  distingue.** No hay antirrebote en el driver; lo que filtra repeticiones es
  `isChange()`, que solo compara el número de teclas pulsadas. Ver
  [`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md).

## Pendiente de comprobar en hardware

Cosas que solo se resuelven flasheando y observando. Anota aquí el resultado.

- [ ] **Ghosting.** ¿Lleva diodos la matriz? Pulsa `t`, `u` y `f` a la vez y mira
      si aparece una `h` fantasma. Procedimiento completo y por qué ese trío en
      [`../04-protocols/gpio-keyboard.md`](../04-protocols/gpio-keyboard.md).
- [ ] **Pérdida de teclas al escribir rápido.** ¿Se nota en la práctica el fallo
      de `isChange()` al solapar dos pulsaciones? (El código actual no usa
      `isChange()`: hace su propio detector de flancos sobre `estado.word`, que
      no tiene ese fallo. Queda por ver si aparece otro.)
- [ ] **Rebote de contactos.** El driver no tiene antirrebote alguno. ¿Aparecen
      caracteres dobles al teclear? Si sí, es el primer candidato a resolver en
      el hito 2 con un antirrebote propio.
- [ ] **Secuencia de prueba**: `hola`, cuatro backspaces seguidos (dos de más a
      propósito), `test`, Enter, y luego Enter tres veces en vacío. Los
      backspaces de más no deben romper nada y los Enter vacíos deben bajar de
      fila limpiamente.
- [ ] **Barrido de la pantalla al llegar abajo.** Confirmar que al agotar las
      filas limpia y reanuda arriba sin dejar restos.

## Deuda conceptual

Conceptos que has decidido aparcar aquí, no abandonar. Índice completo en
[`../deuda-conceptual.md`](../deuda-conceptual.md).

- **Polimorfismo en C++** (`virtual` / `override` / `std::unique_ptr`), aparcado
  en el hito 1 al leer el driver del teclado.

## Checklist de dominio (nivel 1)

Marca cuando puedas explicarlo sin ayuda:

- [ ] Sé qué hace cada línea de `setup()` y `loop()`.
- [ ] Entiendo por qué `M5Cardputer.update()` es necesario en cada iteración, y
      por qué `M5.update()` no basta.
- [ ] Sé por qué se consulta `isChange()` antes de leer `keysState().word`, y qué
      caso pierde esa comprobación.
- [ ] Podría describir qué ocurre en el bus SPI al hacer `println()`.

Cuando marques las cuatro, estás listo para el hito 2 (bajar a nivel 2 quitando
M5Unified para la SD). Ver [`../roadmap.md`](../roadmap.md).
