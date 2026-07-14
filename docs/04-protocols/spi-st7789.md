# SPI + display ST7789V2

Nota de protocolo del display del Cardputer. Se irá ampliando a medida que bajes
de nivel (hito 1 en nivel 1; más adelante niveles 2-4). Pines en
[`../01-hardware/cardputer-map.md`](../01-hardware/cardputer-map.md).

## Qué es el ST7789V2

Controlador de un panel TFT de 240×135 (en el Cardputer). Recibe comandos y datos
por SPI y mantiene su propia RAM de framebuffer. Tú le envías comandos de
configuración (área de dibujo, formato de color, encendido) y luego un flujo de
píxeles.

## Señales relevantes

- **SCLK, MOSI**: reloj y datos SPI (write-only; no leemos del display).
- **CS** (`GPIO12`): selecciona el display. Compartido con SD MOSI — ver
  [`../01-hardware/pin-conflicts.md`](../01-hardware/pin-conflicts.md).
- **DC** (`GPIO34`): Data/Command. **Nivel bajo = byte de comando; nivel alto =
  byte de datos.** Esta línea es lo que distingue "te estoy configurando" de "te
  estoy mandando píxeles". Es el concepto central del protocolo del ST7789.
- **RST** (`GPIO33`): reset por hardware del controlador.
- **BL** (`GPIO38`): backlight. Sin retroiluminación, el panel no se ve aunque
  el framebuffer sea correcto.

## Formato de color

El ST7789 en esta configuración trabaja típicamente en **RGB565**: 16 bits por
píxel (5 rojo, 6 verde, 5 azul). Cuando M5Unified acepta constantes como `GREEN`,
por debajo son valores RGB565.

## Secuencia conceptual de dibujo

1. Bajar CS (seleccionar display).
2. Con DC en bajo, enviar comando de "set column address" y "set row address"
   (definir la ventana de dibujo).
3. Con DC en bajo, enviar el comando de "memory write".
4. Con DC en alto, enviar el flujo de píxeles RGB565.
5. Subir CS.

En **nivel 1** todo esto lo hace `M5.Display`. En **nivel 4** lo harás tú,
moviendo CS/DC/SCLK/MOSI y respetando el timing del datasheet del ST7789.

## Estado actual (nivel 1)

El `main.cpp` inicial usa `M5.Display.setTextSize/setTextColor/fillScreen/
setCursor/println`. Ver qué abstrae cada una en
[`../03-libraries/m5unified.md`](../03-libraries/m5unified.md).

## Pendiente de documentar (según avances)

- [ ] Lista concreta de comandos de init del ST7789V2 (contrastar con datasheet).
- [ ] Timing SPI: frecuencia de reloj usada por M5GFX y máximos del panel.
- [ ] Nivel 2: init del bus con `spi_bus_initialize` + `spi_device_interface_config_t`.
- [ ] Nivel 4: bit-banging del envío de un comando y un byte de datos.

Referencia TRM: capítulo *SPI Controller (GP-SPI2, GP-SPI3)*. Datasheet del
controlador: Sitronix ST7789V2 (enlace en [`../references.md`](../references.md)).
