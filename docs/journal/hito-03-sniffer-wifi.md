# Diario — Hito 3: Sniffer WiFi

**Objetivo**: capturar tramas 802.11 en modo promiscuo y mostrarlas en pantalla,
sin depender de la SD (la persistencia llegará cuando el hito 2a monte una
tarjeta). Primera herramienta de captura de red real.

**Rama**: `hito-03-sniffer-wifi`, sacada de `hito-02-logger-sd` (incluye el
informe de arranque, que tolera la ausencia de SD).

**Estado**: pieza 1 (contadores por tipo) escrita y compilada el 2026-11-11, sin flashear.

## Estado al guardar partida (2026-11-06)

**Al retomar:** `git fetch origin` (Forgejo puede estar caído; ver abajo),
`git switch hito-03-sniffer-wifi`, `pio run`.

**Leído:** [`../04-protocols/wifi-radio-esp32s3.md`](../04-protocols/wifi-radio-esp32s3.md)
recoge la explicación de la sesión: recorrido de la trama (RF, PHY, MAC con
filtro de direcciones, DMA), WiFi sin registros documentados, el callback en la
tarea WiFi del núcleo 0 frente a `loop()` en el núcleo 1, canales y límites,
interés defensivo de las tramas de gestión.

**Pregunta abierta para empezar:** el callback hace `contador_mgmt++` en el
núcleo 0 y `loop()` lo lee en el núcleo 1. Un `uint32_t` alineado se lee y se
escribe con una sola instrucción en el Xtensa. ¿Hace falta proteger los
contadores? ¿Qué cambia si en vez de un contador se copia una MAC de 6 bytes?

**Primera pieza (tras la pregunta):** WiFi arrancado sin conectarse, canal fijo,
modo promiscuo, el callback incrementa contadores de gestión, datos y `MISC`, y
`loop()` los pinta una vez por segundo. Sin decodificar tramas todavía.

## 2026-11-11 — Memoria compartida entre núcleos y pieza 1

**Respuesta a la pregunta abierta, corregida.** La respuesta inicial fue «hay que
protegerlos para que no apunten fuera de los núcleos disponibles» y «con una MAC
sobran 2 bytes». Las dos ideas son erróneas. Un contador no es un puntero: es una
palabra en SRAM interna que ven los dos núcleos por el mismo bus, y el número de
núcleos no interviene. El riesgo es el acceso simultáneo. `contador++` son tres
instrucciones (`l32i`, `addi`, `s32i`): con dos escritores se pierden cuentas, con
uno solo no. La lectura de 32 bits alineados es un único `l32i` y nunca sale a
medias. Por eso el diseño es que el callback sea el único escritor, que los
contadores solo crezcan y que `loop()` calcule la tasa restando dos lecturas, sin
poner nada a cero. En la MAC el relleno es irrelevante. El problema es que 6 bytes
exigen al menos dos stores, y un lector en medio se lleva una dirección mezclada
que nunca pasó por el aire (*torn read*). Eso sí exige spinlock o cola.

**Pieza 1.** `src/sniffer.cpp`: `WiFi.mode(WIFI_STA)` sin asociarse, filtro
MGMT|DATA|MISC, callback en IRAM con `std::atomic<uint32_t>::fetch_add`, canal fijo
(`CANAL = 6` en `main.cpp`). `loop()` pinta una vez por segundo las tramas por
segundo y el total. El terminal del hito 1 pasa a `src/terminal.cpp` sin cambios de
lógica y no se llama en este hito.

**Leído en el binario:** el callback queda en `.iram0.text` y cada `fetch_add` se
compila en línea como bucle `l32i` / `wsr.scompare1` / `s32c1i` / `bne`
(compare-and-swap del Xtensa LX7), sin llamada a librería.

**Qué observar al flashear:** el informe de arranque debe acabar en «WiFi promisc
ok, canal 6» y en un heap menor que el del hito 2 (la diferencia es lo que reserva
el driver WiFi). A los 4 s, la pantalla de contadores. Cerca de un router en el
canal 6, MGMT debería rondar 10/s por cada AP (beacon cada 102,4 ms). DATA debería
subir al usar un móvil en esa red.

## Pendiente de comprobar en hardware

- [ ] Contadores por tipo en un canal fijo (pieza 1): informe de arranque, heap
  tras el WiFi, MGMT/s coherente con los AP del canal 6, DATA sube con tráfico.
