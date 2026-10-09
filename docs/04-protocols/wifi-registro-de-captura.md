# Del radio al registro: qué lleva una captura WiFi

Qué contiene un registro de captura 802.11, qué entrega el ESP32-S3 para rellenarlo
y qué conviene calcular después en lugar de guardar. Aparece al preparar el
[modelo de datos](../06-model/data-model.md) del hito 2, antes de decidir los campos
de un registro, y sirve de base al sniffer del hito 3.

Esquema visual (tira de bytes de un pcap, las 12 palabras de `rx_ctrl`, cabecera
802.11 y salto de canal):
[Del radio al registro](https://claude.ai/artifact/3JbVW9ipwcXS9iKRTMu8Uc).

Lo que sale de headers del ESP-IDF instalado está citado con su struct. El tamaño de
`wifi_pkt_rx_ctrl_t` está **medido** compilando el struct con
`xtensa-esp32s3-elf-gcc` del toolchain. Nada de aquí viene de observar hardware.

---

## Wireshark ya es un log de eventos

Un fichero `.pcap` es un log de solo-anexar. Empieza con una cabecera global de 24
bytes y sigue con un registro por paquete, siempre al final. Cada registro tiene tres
capas.

```
 [ cabecera pcap, 16 B ]  ts_sec, ts_usec, incl_len, orig_len
 [ metadatos del radio ]  radiotap (linktype 127), longitud variable
 [ trama 802.11 cruda  ]  cabecera + cuerpo + FCS de 4 B
```

`incl_len` es lo que se guardó y `orig_len` lo que midió el aire; difieren si se
recorta. Wireshark no guarda «SSID = X» ni «dispositivo Y». Esos campos los calculan
los *dissectors* (decodificadores de protocolo) al leer los bytes, y las
estadísticas, las conversaciones y los filtros son proyecciones del mismo log. Si
mejora el dissector se reabre el fichero y se obtiene mejor interpretación sin
recapturar. Es el modelo B del data model: hechos inmutables, entidades derivadas.

## Qué entrega el callback del S3

El callback de modo promiscuo recibe un `wifi_promiscuous_pkt_t`
(`esp_wifi_types.h`): la cabecera `rx_ctrl`, de tipo `wifi_pkt_rx_ctrl_t`, y detrás
`payload`, con `rx_ctrl.sig_len` bytes **incluido el FCS**. El callback recibe además
el tipo de trama (`wifi_promiscuous_pkt_type_t`: `WIFI_PKT_MGMT`, `CTRL`, `DATA`,
`MISC`), y `wifi_promiscuous_filter_t` permite elegir cuáles llegan.

| Wireshark | ESP-IDF |
|-----------|---------|
| radiotap | `wifi_pkt_rx_ctrl_t rx_ctrl` |
| bytes de la trama | `payload[sig_len]` |
| tipo de trama del dissector | argumento `wifi_promiscuous_pkt_type_t` del callback |

`wifi_pkt_rx_ctrl_t` ocupa **48 bytes** (medido), es decir 12 palabras de 32 bits,
de las cuales casi la mitad son reservadas en el S3. Los campos con significado
documentado son:

| Palabra | Campos |
|---------|--------|
| 0 | `rssi` (8, con signo, dBm), `rate` (5, solo válido en 11bg), `sig_mode` (2) |
| 1 | `mcs` (7), `cwb` (1), `aggregation`, `stbc` (2), `fec_coding`, `sgi` |
| 2 | `ampdu_cnt` (8), `channel` (4), `secondary_channel` (4) |
| 3 | `timestamp` (32, µs) |
| 5 | `noise_floor` (8, dBm) |
| 7 | `ant` (1) |
| 11 | `sig_len` (12), `rx_state` (8) |
| 4, 6, 8, 9, 10 | reservadas |

El `timestamp` del chip son 32 bits en microsegundos: desborda a los 2³² µs, unos
71,6 minutos. No sirve como reloj de sesión; para eso está el `t` de 64 bits de
`esp_timer_get_time()` del data model.

## Lo que calcula el dissector

Dentro de `payload`, una trama de gestión empieza con una cabecera de 24 bytes.

| Campo | Bytes | Contenido |
|-------|-------|-----------|
| Frame Control | 2 | tipo, subtipo, flags |
| Duration | 2 | duración |
| Address 1 | 6 | destino |
| Address 2 | 6 | origen |
| Address 3 | 6 | BSSID |
| Sequence Control | 2 | 4 bits de fragmento, 12 de número de secuencia |

Después viene el cuerpo y el FCS. En un beacon o un probe request el cuerpo son
*information elements* (SSID, velocidades soportadas, capacidades). Las tramas de
datos pueden llevar una cuarta dirección y campos de QoS, así que la cabecera no
siempre mide 24 bytes. El número de secuencia y la huella de los information elements
son las pistas para enlazar MACs aleatorias; ese enlace es una inferencia con
confianza, no un dato, y su sitio es la fase *Identify* de
[`phases.md`](../06-model/phases.md).

## Un solo canal a la vez

El Cardputer tiene un único radio WiFi de 2,4 GHz. Escucha un canal por instante, y
mientras escucha el 6 no ve nada del 1. Un hueco en el log puede deberse a una cola
llena o a que el radio estaba en otro canal. El registro de `gap` debería permitir
distinguir las dos causas.

## Decisión abierta: qué guarda una observación WiFi

| | Prefijo de la trama cruda | Campos ya decodificados |
|---|---|---|
| Qué guarda | Metadatos útiles + primeros N bytes de la trama | Tipo, subtipo, MACs, canal, RSSI, secuencia |
| Bytes por registro | Más | Muchos menos |
| Trabajo en el dispositivo | Poco | CPU en el camino caliente del callback |
| Información perdida | Ninguna dentro del prefijo | Todo lo que no se extraiga |

Es una decisión del usuario, a tomar en la tabla del data model.

## Pendiente de verificar

- [ ] Cuántos bytes de prefijo hacen falta para llegar a los information elements de
      un beacon real (se mide con una captura propia).
- [ ] Qué campos de `rx_ctrl` merece la pena guardar y cuáles no.
