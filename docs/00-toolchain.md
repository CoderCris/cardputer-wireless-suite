# Toolchain y flujo de trabajo

Cómo compilar, flashear y depurar en este proyecto. Entorno: **PlatformIO +
VSCode**, board **StampS3**, framework **Arduino** en la fase inicial (destino:
ESP-IDF).

## `platformio.ini` comentado

```ini
[env:cardputer]
platform = espressif32
board = m5stack-stamps3
framework = arduino
```

Config declarativa y versionable del entorno. `board` selecciona el StampS3
(ESP32-S3); PlatformIO deriva de ahí el toolchain, la memoria y los flags base.

### build_flags
```ini
-DARDUINO_USB_MODE=1         ; Usa el USB nativo del S3, no un UART externo
-DARDUINO_USB_CDC_ON_BOOT=1  ; Habilita CDC (Communications Device Class) al arrancar
-DBOARD_HAS_PSRAM            ; El StampS3 tiene 8 MB de PSRAM (buffers grandes)
```

El Cardputer **no tiene chip UART externo**: el puerto serie es el USB nativo del
ESP32-S3 en modo CDC. Sin estos flags, `Serial.print()` no saldría por el USB-C.
Esto explica por qué a veces el puerto "desaparece" al resetear: es el propio SoC
el que crea/destruye el dispositivo USB.

### Particiones y monitor
```ini
board_build.partitions = default_16MB.csv  ; espacio para OTA + SPIFFS/LittleFS
monitor_speed = 115200                      ; baudios del monitor serie
```

## Ciclo de trabajo

| Acción | Comando | Qué hace |
|--------|---------|----------|
| Compilar + linkar | `pio run` | Genera el firmware. **No toca el hardware.** |
| Flashear | `pio run -t upload` | Sube el firmware al Cardputer por USB |
| Monitor serie | `pio device monitor` | Abre el puerto serie (Serial.print) |
| Flashear + monitor | `pio run -t upload -t monitor` | Las dos cosas seguidas |
| Limpiar | `pio run -t clean` | Borra artefactos de build |

`pio run` valida **compilación y linkado**, no comportamiento. Que compile no
significa que funcione en hardware: los pines mal, un bus mal configurado o un
deadlock no dan error de compilación.

## Modo de flasheo del ESP32-S3

Si el upload falla, el S3 puede necesitar entrar en **modo bootloader**
manualmente. En placas ESP32-S3 esto suele ser: mantener **BOOT**, pulsar
**RESET**, soltar RESET, soltar BOOT. Confirma el método concreto del Cardputer en
la doc de M5Stack (enlace en [`references.md`](references.md)). Tras flashear,
puede hacer falta un reset para que reaparezca el puerto CDC.

## Depuración disponible

- **`Serial.print()`** por USB-CDC: el método principal. Rápido y suficiente para
  la mayoría de hitos.
- **JTAG por USB nativo**: el S3 expone JTAG por el mismo USB. Permite breakpoints
  reales con `pio debug`. Útil cuando `Serial` no basta (p. ej. debugging de un
  crash temprano o de timing).

## Límite de verificación del asistente

El asistente puede ejecutar `pio run` (compila/linka) pero **no puede flashear ni
observar el dispositivo**. Flasheas tú, observas tú, y reportas el resultado. Ver
`CLAUDE.md` en la raíz.
