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

## Incidencia abierta: pantalla negra (2026-09-29)

Primer flasheo del hito 1 al dispositivo. Nada de lo anterior se ha podido
comprobar todavía porque la pantalla no muestra nada.

**Síntoma.** Pantalla en negro al encender, **ya con Bruce** (que antes se veía),
antes de flashear nada del hito. Tras `pio run -t upload` (upload correcto) sigue
igual. Desconectar, apagar/encender y reset no cambian nada.

**Descartado.**
- El chip está vivo: enumera como `303a:1001` (USB Serial/JTAG del S3), estable,
  y `esptool.py flash_id` responde. Silicio y flash bien.
- No es la lógica del terminal: `setup()` escribe "Terminal ready" sin esperar
  teclas, y Bruce también salía en negro.
- Que el monitor serie no muestre nada es esperado y no es síntoma: el firmware
  no usa `Serial`, y el log de la ROM se pierde porque el USB se re-enumera en
  cada reset (no hay conversor USB-UART externo que siga enumerado).

**Hipótesis vigente.** Hardware o alimentación, no software. M5GFX detecta el
Cardputer leyendo el ID del ST7789 por SPI de 3 hilos (`M5GFX.cpp:2154-2184`);
si el panel no responde, no inicializa el display ni enciende el backlight
(GPIO38). Candidatos: batería LiPo muy descargada tras meses sin uso, conexión
StampS3 ↔ base, o el propio panel.

**Siguientes pasos, en orden.**
1. [ ] Dejarlo cargando ~30 min con el interruptor en ON. Reintentar.
2. [ ] En oscuridad: ¿hay brillo tenue de backlight o está muerto del todo?
3. [ ] Si sigue en negro: flashear Bruce (firmware conocido). Negro con Bruce →
       hardware confirmado.

**Actualización 2026-10-02: no es la pantalla, es un bucle de arranque.**
Leyendo el puerto serie sin tocar DTR/RTS (para no provocar resets nosotros), la
ROM imprime su banner ~40 veces por segundo (158 arranques en 4 s), siempre igual:
`rst:0x3 (RTC_SW_SYS_RST)`, carga el bootloader de segunda etapa (`entry
0x403c98d0`) y vuelve a empezar. La app nunca llega a ejecutarse, así que la
pantalla negra es consecuencia, no causa. El USB no se re-enumera durante el bucle.

Lo comprobado en flash (lectura con `esptool.py read_flash` / `verify_flash`):
- `0x0`: el bootloader grabado es idéntico byte a byte a
  `.pio/build/cardputer/bootloader.bin` (cabecera: DIO, 80 MHz, 8 MB).
- `0x8000`: tabla de particiones = `default_16MB.csv`, válida.
- `0xe000` (otadata): secuencia 1 → arranca `ota_0`. `ota_1` está vacía (`0xFF`).
- `0x10000`: `verify_flash` del `firmware.bin` → digest correcto. El hash
  SHA-256 que lleva la imagen al final también es válido (`esptool image_info`).

**Causa encontrada y corregida (2026-10-02): la tabla de particiones.** La primera
hipótesis (lecturas corruptas a 80 MHz) quedó **refutada**: con la flash a 40 MHz
el ritmo del bucle no cambió (158 arranques / 4 s), así que el fallo no dependía de
leer la app. El `Saved PC` se había mapeado con el ELF equivocado: el bootloader
grabado es la variante **QIO** (`bootloader_qio_80m.elf`, el board define
`flash_mode: qio`) con la cabecera parcheada a DIO. Con el ELF correcto,
`0x403cdb0a` es el bucle final de `bootloader_reset()`.

`call_start_cpu0` llega a `bootloader_reset()` por cuatro caminos: falla
`bootloader_init()`, falla `bootloader_utility_load_partition_table()`, no hay
partición seleccionable, o no hay app arrancable. El segundo es el nuestro.
`esp_partition_table_verify` compara cada partición con
`g_rom_flashchip.chip_size` (8 MB, sacado de la cabecera del bootloader) y
devuelve `ESP_ERR_INVALID_SIZE` (`0x104`) si `offset + size` lo excede. Con
`default_16MB.csv`, `app1` acaba en `0xC90000` > `0x800000` → tabla rechazada →
reset. No se ve ningún mensaje porque el bootloader de Arduino está compilado con
`CONFIG_BOOTLOADER_LOG_LEVEL_NONE`.

Lo que se dio por "latente" no lo era: era la causa. Cambios en `platformio.ini`:
1. `default_16MB.csv` → `default_8MB.csv`. Tras flashear: 1 arranque, sin bucle.
2. Fuera `-DBOARD_HAS_PSRAM`: el chip **no tiene PSRAM** (eFuse `PSRAM_CAP = None`,
   `espefuse.py summary`). Con el flag, la app logueaba `PSRAM ID read error`.
   Tras quitarlo: arranque limpio, 8 s sin reinicios ni errores.

Queda abierto: por qué Bruce también salía en negro. No lo sabemos; una tabla de
particiones incompatible con 8 MB produciría exactamente el mismo síntoma.

**Comprobado en hardware (2026-10-02, por el usuario):** la pantalla muestra la
terminal; Backspace borra y Enter salta de línea correctamente. Quedan las
pruebas de ghosting y rebote.

## Estado al guardar partida (2026-10-02)

**En curso: ajuste de línea (wrap).** M5GFX ya hace el wrap visual por su cuenta
(`_textwrap_x = true` por defecto, `LGFXBase.hpp:1057`; salto en
`LGFXBase.cpp:2413-2419`). Lo que lo impide es `CAPACIDAD = 20`. Esqueleto con
`TODO(wrap 1..5)` en `src/main.cpp`; compila y se comporta igual que antes
(`filas_linea()` devuelve 1). Pregunta abierta: en `TODO(wrap 4)`, ¿qué
rectángulo limpiar para que el redibujado sea correcto tras cualquier Backspace,
sin recordar el estado anterior?

**Propuesta para el hito 2:** partirlo en 2a (SD con la librería `SD` de Arduino,
manteniendo M5Unified: nivel 1, concepto nuevo = filesystem) y 2b (quitar
M5Unified, init manual: nivel 2). Pendiente de decidir y reflejar en
`roadmap.md`. El gate sigue siendo el checklist de dominio de abajo, no el wrap.

**Errores en `docs/` pendientes de corregir:**
- AXP2101 (ver arriba).
- `01-hardware/pin-conflicts.md`: `GPIO12` NO es CS del display. Display en
  SCK 36 / MOSI 35 / CS 37 (`M5GFX.cpp:2161-2169`); SD en SCK 40 / MOSI 14 /
  MISO 39 / CS 12 (`M5Unified.cpp:186`, `M5Cardputer/examples/Basic/sdcard`).
  Pregunta abierta: ¿comparten bus SPI? (TRM §30, *SPI Controller*).

**Entorno arreglado de paso.** Usuario añadido a `dialout` (hace falta re-login
para que el kernel lo cargue en las credenciales del proceso); `pio` enlazado en
`~/.local/bin`. Pendiente opcional: regla udev mínima para que ModemManager no
toque `303a:1001`.

**Error encontrado en `docs/`.** El Cardputer V1 **no tiene AXP2101**: M5Unified
usa `pmic_adc` para `board_M5Cardputer` (batería por ADC en GPIO10, divisor 2:1;
`Power_Class.cpp:295-301`). Afirman lo contrario `01-hardware/cardputer-map.md`,
`01-hardware/buses.md`, `03-libraries/m5unified.md`, `glossary.md`,
`05-vision.md` y el comentario de `src/main.cpp:63`. Pendiente de corregir.

**Latente.** `platformio.ini` usa `default_16MB.csv` con flash de 8 MB: `app1`,
`spiffs` y `coredump` quedan fuera del chip. Inofensivo mientras solo se use
`app0`; hay que arreglarlo antes de OTA o sistema de ficheros.

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
