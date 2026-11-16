# El radio WiFi del ESP32-S3 y el modo promiscuo

Cómo llega una trama 802.11 desde la antena hasta el código, qué impone eso al
diseño del sniffer (hito 3) y qué queda fuera del alcance del hardware. Complementa
[`wifi-registro-de-captura.md`](wifi-registro-de-captura.md), que detalla el
contenido de `rx_ctrl` y de la trama.

Fuentes: headers del ESP-IDF instalado con el core Arduino
(`~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/esp_wifi/include/`)
y su `sdkconfig.h` (`tools/sdk/esp32s3/qio_qspi/include/sdkconfig.h`). Lo marcado
**[guía]** viene de la ESP-IDF Programming Guide y no está verificado en el repo.
Nada de aquí viene de observar hardware.

---

## Recorrido de una trama en el hardware

El radio está integrado en el SoC; no hay chip externo, y la antena va en el módulo
StampS3 ([`../01-hardware/cardputer-map.md`](../01-hardware/cardputer-map.md)).

1. **Front-end de RF** (analógico). Un conmutador alterna la antena entre emisión y
   recepción. En recepción, un amplificador de bajo ruido (LNA) sube la señal y un
   mezclador la baja de 2,4 GHz a una frecuencia digitalizable.
2. **Baseband / PHY** (digital). Demodula: DSSS/CCK para 802.11b, OFDM para g/n.
   Aquí se miden los metadatos que acaban en `rx_ctrl`: RSSI, velocidad, modo de
   señal, ruido de fondo.
3. **MAC por hardware.** Comprueba el FCS (CRC de 4 bytes al final de la trama) y
   filtra por dirección: en modo normal descarta lo que no va a la MAC propia, a
   broadcast o a un grupo suscrito. **El modo promiscuo desactiva ese filtro de
   direcciones** y todo lo demodulado sube al software.
4. **DMA a RAM.** La trama se copia sin CPU a un buffer de recepción en RAM interna:
   `CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM 8` y
   `CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM 32` (`sdkconfig.h:400-401`). Esa RAM
   sale del mismo heap que mide el informe de arranque.

**El WiFi no está documentado a nivel de registros.** El TRM del ESP32-S3 no tiene
capítulo de la MAC ni del PHY WiFi; Espressif los entrega como librerías
precompiladas (`libnet80211.a`, `libpp.a`, `libesp_phy.a`, `libcoexist.a` en
`tools/sdk/esp32s3/lib/`). El nivel más bajo accesible es la API `esp_wifi`. El
sniffer puede bajar a ESP-IDF, pero no a registros.

## El callback y su contexto de ejecución

```c
typedef void (* wifi_promiscuous_cb_t)(void *buf, wifi_promiscuous_pkt_type_t type);  // esp_wifi.h:719
esp_err_t esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb);                   // esp_wifi.h:732
esp_err_t esp_wifi_set_promiscuous(bool en);                                           // esp_wifi.h:743
```

`buf` es un `wifi_promiscuous_pkt_t` (48 B de `rx_ctrl` + trama); `type` es
`WIFI_PKT_MGMT`, `CTRL`, `DATA` o `MISC`. Una `MISC` (p. ej. MIMO) llega con
payload de longitud cero (`esp_wifi_types.h:452`).

El callback **no corre en `loop()`**: corre en la tarea del driver WiFi, fijada al
**núcleo 0** (`CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0`, `sdkconfig.h:415`).
`loop()` corre en el **núcleo 1** (`CONFIG_ARDUINO_RUNNING_CORE 1`, `sdkconfig.h:87`).
Son dos núcleos en paralelo real. Consecuencias:

- **Breve.** Mientras el callback trabaja, el driver no atiende la siguiente
  trama; si los buffers se llenan, las nuevas se pierden en silencio. **[guía]**
  pide evitar operaciones largas en el callback.
- **Copiar, no guardar punteros.** Al volver del callback el driver libera el
  buffer; lo que haga falta conservar se copia dentro.
- **No pintar.** Pintar es una transacción SPI larga, y M5GFX no está preparado
  para que dos núcleos usen el display a la vez.

Patrón resultante: **el callback cuenta o copia; `loop()` pinta.** Lo que queda en
medio es memoria compartida entre dos núcleos.

## Canales y límites del hardware

En 2,4 GHz los canales 1-13 están separados 5 MHz y cada uno ocupa unos 20 MHz:
se solapan, y solo 1, 6 y 11 no interfieren entre sí. El radio sintoniza un canal
cada vez; ver todos exige saltar de canal (channel hopping) y perder lo que pasa
en los demás mientras tanto.

Fuera de alcance:

- **5 GHz**: el S3 solo tiene radio de 2,4 GHz.
- **Modos que el PHY no demodula** (802.11ac/ax en 2,4 GHz, algunas
  configuraciones MIMO): llegan como `MISC` sin contenido o no llegan.
- **Tramas con FCS corrupto**: descartadas salvo `WIFI_PROMIS_FILTER_MASK_FCSFAIL`
  (`esp_wifi_types.h:463`), que el header desaconseja en general.
- **Tramas de control** (ACK, RTS, CTS): necesitan su propia máscara de filtro
  (`WIFI_PROMIS_CTRL_FILTER_MASK_ALL`, `esp_wifi_types.h:465`). Son muy
  numerosas y casi todas de 10 a 16 bytes.

## Interés defensivo

Las tramas de gestión viajan **sin cifrar** aunque la red use WPA2/WPA3, salvo
con PMF (802.11w): beacons (anuncios de los puntos de acceso), probe requests
(dispositivos preguntando por redes conocidas) y deauth/disassoc (órdenes de
desconexión). Un ataque de desautenticación es una ráfaga de deauth falsificadas;
la detección mínima es contar deauth por segundo y canal y vigilar picos. Por eso
el contador por tipo y subtipo es la base del sniffer.

## Pendiente de verificar

- [ ] Tipo de antena del StampS3 (PCB o cerámica) contra el esquemático de M5Stack.
- [ ] Contexto exacto del callback según la Programming Guide de la versión de
      ESP-IDF del core instalado (marcado **[guía]** arriba).
- [ ] Tasa de tramas a partir de la cual se pierden en el Cardputer (se mide con
      el propio sniffer).
