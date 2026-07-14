# Teclado matricial del Cardputer

Nota de protocolo del teclado (56 teclas). En nivel 1 lo maneja
`M5Cardputer.Keyboard`; el objetivo de este documento es entender el mecanismo
para poder reimplementarlo en nivel 2.

## Por qué una matriz

56 teclas con un pin cada una serían 56 GPIO — imposible en el S3. La solución
estándar es una **matriz**: las teclas se organizan en filas y columnas, y en cada
cruce hay un pulsador. Se detecta qué tecla está pulsada **escaneando**: se activa
una columna cada vez y se lee qué filas responden. Así, con F filas + C columnas
cubres F×C teclas usando solo F+C líneas (o menos, si la selección de columna se
hace con un demultiplexor).

## Escaneo (concepto)

1. Seleccionar una columna (ponerla en el nivel activo).
2. Leer el estado de todas las filas.
3. Si una fila está activa, la tecla en (columna, fila) está pulsada.
4. Repetir para cada columna, rápido y en bucle.

El estado del teclado se refresca en cada `M5.update()` / lectura del driver. Por
eso en el `loop()` se consulta `isChange()` / `isPressed()` en vez de "esperar" a
una tecla: el driver mantiene un snapshot que tú lees.

## Anti-rebote (debounce)

Un pulsador mecánico no hace un contacto limpio: rebota durante milisegundos y
puede leerse como varias pulsaciones. El driver aplica **debounce** (filtrado
temporal) para entregar una sola pulsación por tecla. Si en nivel 2 escaneas la
matriz tú mismo, tendrás que implementar el debounce o verás teclas "fantasma".

## Estado actual (nivel 1)

```cpp
if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    char key = M5Cardputer.Keyboard.lastKeyCode();
    if (key != 0) M5.Display.print(key);
}
```

`isChange()` evita procesar el mismo estado repetido; `isPressed()` filtra los
eventos de "tecla soltada"; `lastKeyCode()` da el carácter. Ver
[`../03-libraries/m5unified.md`](../03-libraries/m5unified.md).

## Pendiente de documentar (según avances)

- [ ] Pines/mecanismo exacto de selección de columna en el Cardputer V1
      (contrastar con el esquemático de M5Stack).
- [ ] Mapa de scancodes → caracteres, incluyendo modificadores (Fn, shift).
- [ ] Nivel 2: escaneo de la matriz con `gpio_config()` + debounce propio.

Referencia TRM: capítulo *IO MUX and GPIO Matrix* (configuración de los pines de
lectura/selección).
