// sniffer.cpp — Sniffer WiFi, pieza 1: contar tramas por tipo en un canal fijo.
//
// Qué hay debajo (detalle en docs/04-protocols/wifi-radio-esp32s3.md):
// el radio de 2,4 GHz demodula lo que hay en el canal sintonizado, el MAC de
// hardware comprueba el FCS y, en modo normal, descarta lo que no va dirigido a
// nuestra dirección. El modo promiscuo desactiva ese filtro de direcciones: el
// MAC entrega por DMA todas las tramas con FCS correcto y el driver (blob
// cerrado, sin registros documentados en el TRM) llama a nuestro callback.

#include <WiFi.h>        // WiFi.mode(): inicializa el driver y arranca el radio
#include <esp_wifi.h>    // API promiscua de ESP-IDF
#include <atomic>
#include "sniffer.h"

// ---------------------------------------------------------------------------
// MEMORIA COMPARTIDA ENTRE NÚCLEOS
//
// El callback corre en la tarea del driver WiFi, fijada al núcleo 0. loop()
// corre en el núcleo 1. Estas tres variables están en SRAM interna, que ambos
// núcleos ven por el mismo bus, sin caché de por medio.
//
// Por qué std::atomic y no un uint32_t a secas:
//  - Para el COMPILADOR: sin él, puede guardar el valor en un registro de la
//    CPU dentro de un bucle de quien lee y no volver a leer la memoria, porque
//    "nadie en este hilo lo cambia". atomic obliga a ir a memoria cada vez.
//  - Para el HARDWARE: fetch_add es una lectura-modificación-escritura
//    indivisible. En el Xtensa LX7 se implementa con S32C1I (compare-and-swap
//    sobre el registro SCOMPARE1): si otro núcleo tocó la palabra entre la
//    lectura y la escritura, la escritura falla y se reintenta.
//
// Hoy solo hay un escritor (el callback), así que bastaría con que la
// escritura no se pierda y la lectura no salga a medias: una palabra de 32
// bits alineada se lee con un solo l32i. fetch_add cuesta unos ciclos más y
// sigue siendo correcto si mañana aparece un segundo escritor.
//
// Regla de diseño: loop() NUNCA escribe aquí (no pone a cero). La tasa por
// segundo se calcula restando dos lecturas. Si loop() pusiera a cero, habría
// dos escritores y un incremento que cayera entre su lectura y su puesta a
// cero se perdería.
// ---------------------------------------------------------------------------
static std::atomic<uint32_t> cuenta_mgmt{0};
static std::atomic<uint32_t> cuenta_data{0};
static std::atomic<uint32_t> cuenta_misc{0};


// El callback. Se ejecuta una vez por trama, en el núcleo 0, dentro de la
// tarea del driver: mientras dura, el driver no entrega la siguiente. Si los
// buffers de RX del driver se llenan, las tramas nuevas se pierden sin aviso.
// Por eso aquí solo se cuenta: nada de pintar, imprimir ni reservar memoria.
//
// IRAM_ATTR coloca la función en RAM interna en vez de en flash. La flash se
// lee a través de la caché y, mientras se escribe en ella (NVS, OTA), la caché
// se desactiva: código en flash no podría ejecutarse en ese momento.
static void IRAM_ATTR rx_cb(void* buf, wifi_promiscuous_pkt_type_t tipo)
{
    (void)buf;   // en esta pieza no se mira el contenido de la trama

    // memory_order_relaxed: solo importa que la suma sea indivisible; no hay
    // otras variables cuyo orden de escritura deba ver el otro núcleo.
    switch (tipo) {
        case WIFI_PKT_MGMT: cuenta_mgmt.fetch_add(1, std::memory_order_relaxed); break;
        case WIFI_PKT_DATA: cuenta_data.fetch_add(1, std::memory_order_relaxed); break;
        case WIFI_PKT_MISC: cuenta_misc.fetch_add(1, std::memory_order_relaxed); break;
        default: break;  // CTRL no llega: el filtro de abajo no la pide
    }
}


esp_err_t sniffer_begin(uint8_t canal)
{
    // Modo estación sin llamar a WiFi.begin(): el driver arranca, el radio se
    // enciende y calibra, pero no busca ni se asocia a ningún punto de acceso.
    // Internamente es esp_wifi_init() + esp_wifi_set_mode() + esp_wifi_start().
    if (!WiFi.mode(WIFI_STA)) return ESP_FAIL;

    // Qué tipos entrega el driver al callback. Las de control (ACK, RTS, CTS)
    // quedan fuera: son muy numerosas, de 10-16 bytes, y no aportan a esta
    // pieza (tienen su propio filtro, WIFI_PROMIS_CTRL_FILTER_MASK_ALL).
    wifi_promiscuous_filter_t filtro = {};
    filtro.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT |
                         WIFI_PROMIS_FILTER_MASK_DATA |
                         WIFI_PROMIS_FILTER_MASK_MISC;

    esp_err_t err;
    if ((err = esp_wifi_set_promiscuous_filter(&filtro)) != ESP_OK) return err;
    if ((err = esp_wifi_set_promiscuous_rx_cb(rx_cb))    != ESP_OK) return err;
    if ((err = esp_wifi_set_promiscuous(true))           != ESP_OK) return err;

    // El canal se fija DESPUÉS de activar el modo promiscuo. Sintoniza el
    // sintetizador del radio a la frecuencia central del canal (2412 MHz +
    // 5 MHz por canal a partir del 1). WIFI_SECOND_CHAN_NONE: ancho de 20 MHz,
    // sin canal secundario de 40 MHz.
    return esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
}


SnifferCuentas sniffer_leer()
{
    // Tres lecturas de 32 bits, cada una indivisible. Las tres juntas NO son
    // una foto instantánea: entre la primera y la tercera puede entrar una
    // trama. Para contadores es irrelevante (el error es de una trama y se
    // corrige en la siguiente lectura); para copiar una MAC de 6 bytes no lo
    // sería, y por eso necesitará otra protección.
    return {
        cuenta_mgmt.load(std::memory_order_relaxed),
        cuenta_data.load(std::memory_order_relaxed),
        cuenta_misc.load(std::memory_order_relaxed),
    };
}
