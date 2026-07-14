# Diario — Hito 1: Terminal interactiva

> Plantilla de diario de hito. Rellénala **tú** a medida que avanzas: es tu
> registro de decisiones y aprendizajes, no una spec. Un diario por hito.

**Objetivo**: entrada por teclado → eco a display. Aprender el ciclo
editar → compilar → flashear → observar. Nivel 1 (Arduino-like).

**Estado**: en curso.

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
- _(pendiente)_

## Qué falló y cómo lo resolví

<!-- Errores de compilación, comportamiento inesperado en hardware, timing, etc.
     Esta sección es oro para tu yo futuro. -->
- _(pendiente)_

## Preguntas abiertas

<!-- Cosas que aún no entiendo del todo y quiero retomar al bajar de nivel. -->
- ¿Qué hace exactamente `M5.begin()` por debajo, paso a paso? (se desmonta en hito 2)
- ¿Cómo distingue el driver una pulsación real de un rebote?

## Checklist de dominio (nivel 1)

Marca cuando puedas explicarlo sin ayuda:

- [ ] Sé qué hace cada línea de `setup()` y `loop()`.
- [ ] Entiendo por qué `M5.update()` es necesario en cada iteración.
- [ ] Sé por qué se consulta `isChange()` antes de `lastKeyCode()`.
- [ ] Podría describir qué ocurre en el bus SPI al hacer `println()`.

Cuando marques las cuatro, estás listo para el hito 2 (bajar a nivel 2 quitando
M5Unified para la SD). Ver [`../roadmap.md`](../roadmap.md).
