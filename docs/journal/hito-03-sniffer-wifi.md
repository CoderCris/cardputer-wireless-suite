# Diario — Hito 3: Sniffer WiFi

**Objetivo**: capturar tramas 802.11 en modo promiscuo y mostrarlas en pantalla,
sin depender de la SD (la persistencia llegará cuando el hito 2a monte una
tarjeta). Primera herramienta de captura de red real.

**Rama**: `hito-03-sniffer-wifi`, sacada de `hito-02-logger-sd` (incluye el
informe de arranque, que tolera la ausencia de SD).

**Estado**: explicación de hardware dada (2026-11-06). Sin código todavía.

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

## Pendiente de comprobar en hardware

- [ ] Contadores por tipo en un canal fijo (primera pieza).
