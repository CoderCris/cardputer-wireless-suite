#include <M5Unified.h>
#include <M5Cardputer.h>   // ya incluye M5Unified.h; el include de arriba es
                           // redundante pero inofensivo (include guards)
#include <SPI.h>           // objeto global SPI = controlador GP-SPI2
#include <SD.h>            // comandos SD en modo SPI + FatFs encima
#include "sniffer.h"       // hito 3: herramienta activa
#include "terminal.h"      // hito 1: no se llama en este hito

// Pines de la SD. Fuente: docs/01-hardware/spi-pads-controladores.md.

constexpr uint8_t SD_SCK_PIN = 40;
constexpr uint8_t SD_MISO_PIN = 39;
constexpr uint8_t SD_MOSI_PIN = 14;
constexpr uint8_t SD_CS_PIN = 12;

// Canal 2,4 GHz que escucha el sniffer (1-13). El 6 es uno de los tres que no
// se solapan (1, 6, 11) y de los más usados por los routers domésticos.
constexpr uint8_t CANAL = 6;




// ---------------------------------------------------------------------------
// INFORME DE ARRANQUE EN PANTALLA
//
// Cómo viaja un log_w() del core hasta el exterior: log_printfv() formatea el
// texto y llama a ets_printf() (esp32-hal-uart.c:842), una función de la ROM
// del chip. ets_printf no sabe de USB ni de UART: entrega cada carácter a dos
// "putc" instalables, putc1 y putc2 (esp_rom/.../esp32s3/rom/ets_sys.h:218).
// Con ARDUINO_USB_CDC_ON_BOOT, putc2 es el USB nativo (HWCDC.cpp:599) y putc1
// queda para una UART. Aquí no hay nadie escuchando una UART, así que usamos
// putc1 para nosotros: cada carácter de log entra también en log_hook().
//
// log_hook() NO pinta: puede llamarse desde cualquier sitio, incluso desde una
// interrupción (isr_log_*), y pintar es una transacción SPI larga contra el
// display. Solo copia en un buffer en RAM; setup() lo vuelca a pantalla luego.
// ---------------------------------------------------------------------------

constexpr int LOG_LINEAS = 8;    // líneas de log que se conservan (las últimas)
constexpr int LOG_ANCHO  = 96;   // caracteres por línea; lo que sobra se corta

static char log_buf[LOG_LINEAS][LOG_ANCHO];
static int  log_n = 0;           // líneas completas guardadas (como mucho LOG_LINEAS)
static char log_cur[LOG_ANCHO];  // línea que se está recibiendo
static int  log_cur_n = 0;

static void log_hook(char c)
{
    if (c == '\r') return;                       // los logs acaban en "\r\n"
    if (c != '\n') {
        if (log_cur_n < LOG_ANCHO - 1) log_cur[log_cur_n++] = c;
        return;
    }
    // Fin de línea: guardarla. Si el buffer está lleno se descarta la más
    // antigua desplazando las demás; en arranque es raro y es barato.
    log_cur[log_cur_n] = '\0';
    if (log_n == LOG_LINEAS) {
        memmove(log_buf[0], log_buf[1], sizeof(log_buf[0]) * (LOG_LINEAS - 1));
        log_n--;
    }
    memcpy(log_buf[log_n++], log_cur, log_cur_n + 1);
    log_cur_n = 0;
}

static void instalar_espejo_log()
{
    ets_install_putc1(log_hook);
}

// Mensaje propio del arranque: va a pantalla (en color) y al monitor serie.
static void boot_msg(uint16_t color, const char* fmt, ...)
{
    char txt[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(txt, sizeof(txt), fmt, args);
    va_end(args);

    Serial.println(txt);
    M5.Display.setTextColor(color, BLACK);
    M5.Display.println(txt);
}

// Traduce los mensajes conocidos de sd_diskio.cpp a la capa que ha fallado.
// Devuelve nullptr si el mensaje no tiene pista asociada.
static const char* pista_sd(const char* msg)
{
    // CMD0 sin respuesta: la tarjeta no contesta en absoluto. MISO se lee a 0
    // porque la placa no tiene pull-up en esa línea y queda flotando
    // (observado el 2026-11-06 con la ranura vacía).
    if (strstr(msg, "GO_IDLE_STATE")) return "-> capa electrica: sin tarjeta o sin contacto";
    // Contesta a CMD0 pero rechaza la secuencia de arranque (CMD8/ACMD41/CMD58).
    if (strstr(msg, "OP_COND") || strstr(msg, "READ_OCR") || strstr(msg, "SEND_IF_COND"))
        return "-> protocolo SD: tarjeta rara, vieja o danada";
    // La tarjeta funciona pero no hay un volumen FAT que FatFs entienda.
    if (strstr(msg, "No FAT") || strstr(msg, "no valid FAT"))
        return "-> sistema de archivos: formatear en FAT32";
    return nullptr;
}

// Pinta lo capturado por log_hook() en formato corto y vacía el buffer.
static void volcar_log_arranque()
{
    // Una pista por volcado: si fallan varios pasos, la primera capa que
    // falla es la que explica las demás.
    const char* pista = nullptr;

    for (int i = 0; i < log_n; i++) {
        const char* linea_log = log_buf[i];

        // Formato del core: "[  5863][W][sd_diskio.cpp:104] sdWait(): Wait Failed"
        // (ARDUHAL_LOG_FORMAT en esp32-hal-log.h:91). En pantalla basta con el
        // nivel y lo que va tras el último "] ": "W sdWait(): Wait Failed".
        char nivel = '?';
        const char* p = strstr(linea_log, "][");
        if (p) nivel = p[2];
        const char* msg = linea_log;
        const char* q;
        while ((q = strstr(msg, "] ")) != nullptr) msg = q + 2;

        uint16_t color = (nivel == 'E') ? RED : (nivel == 'W') ? YELLOW : WHITE;
        M5.Display.setTextColor(color, BLACK);
        M5.Display.printf("%c %s\n", nivel, msg);

        if (!pista) pista = pista_sd(msg);
    }

    if (pista) {
        M5.Display.setTextColor(CYAN, BLACK);
        M5.Display.println(pista);
    }
    log_n = 0;
}


void setup()
{
    // M5Cardputer.begin() hace DOS cosas, y por eso no vale M5.begin():
    //
    //  1. Llama a M5.begin(cfg), que inicializa lo que M5Unified conoce de la
    //     placa: configura el bus SPI y arranca el driver del display
    //     ST7789V2, configura la lectura de batería por ADC (GPIO10; el
    //     Cardputer V1 no tiene PMIC), y prepara botones, altavoz y micrófono.
    //
    //  2. Llama a Keyboard.begin(), que configura los 10 GPIO del teclado
    //     matricial: 8, 9 y 11 como salidas (seleccionan una de las 8 líneas
    //     del barrido a través de un decodificador 3->8) y 13, 15, 3, 4, 5, 6
    //     y 7 como entradas con pull-up interno (leen las 7 columnas).
    //
    // M5.begin(cfg) a secas NO hace el paso 2: los pines del teclado nunca se
    // configuran y la lista de teclas queda vacía para siempre.
    // En nivel 2 desmontaremos esto en llamadas explícitas para entender cada
    // paso. Detalle en docs/04-protocols/gpio-keyboard.md
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    // El arranque se pinta a escala 1 (40 columnas x 16 filas): los mensajes
    // de diagnóstico son largos y a escala 2 solo caben 20 caracteres por fila.
    // El terminal vuelve a escala 2 al final de setup().
    M5.Display.setTextSize(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);

    // A partir de aquí, todo log_e/log_w del core y de las librerías se copia
    // también en el buffer de arranque (ver log_hook más abajo).
    instalar_espejo_log();

    boot_msg(GREEN, "Cardputer boot");
    boot_msg(WHITE, "SPI2: SCK%u MISO%u MOSI%u CS%u",
             SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    // GP-SPI2 (el objeto SPI de Arduino). El display ya ocupa GP-SPI3; este es
    // el último controlador SPI de propósito general libre.
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    // Montaje de la SD. Los 4 MHz NO son la frecuencia de la inicialización:
    // esa la fija la librería a 400 kHz como máximo (sd_diskio.cpp:502),
    // porque antes del diálogo de arranque el host no sabe qué tarjeta tiene
    // delante. 4 MHz es la frecuencia de TRABAJO que se aplica después. Se
    // elige muy por debajo del techo del modo por defecto (25 MHz, recortado
    // en sd_diskio.cpp:605) para que la integridad de señal (pistas, zócalo,
    // calidad de la tarjeta) no sea sospechosa en el primer montaje: si falla
    // a 4 MHz, la causa está en cableado, formato o ausencia de tarjeta, no en
    // la velocidad. La frecuencia definitiva saldrá del test de frecuencia
    // pendiente (diario del hito 2).
    boot_msg(WHITE, "SD.begin @4MHz...");
    bool sd_connected = SD.begin(SD_CS_PIN, SPI, 4000000);

    // Lo que la librería haya dicho durante el montaje, y una pista de causa.
    volcar_log_arranque();

    if (sd_connected) {
        // cardSize() devuelve bytes en uint64_t: una tarjeta de 16 GB son
        // ~1.6e10 bytes y un int de 32 bits solo llega a ~2.1e9. Meterlo en
        // un int conservaría los 32 bits bajos y saldría un número sin
        // sentido, incluso negativo. Se divide en 64 bits y se imprime en MiB
        // (1 MiB = 2^20 bytes), que sí cabe holgado.
        uint64_t sd_mib = SD.cardSize() / (1024ULL * 1024ULL);

        // cardType() es lo que la tarjeta declaró en el arranque: SDSC (<=2 GB,
        // direcciona por bytes) o SDHC/SDXC (direcciona por bloques de 512 B;
        // la librería las llama a todas SDHC). Lo decide la respuesta a CMD58.
        const char* tipo = "?";
        switch (SD.cardType()) {
            case CARD_MMC:  tipo = "MMC";  break;
            case CARD_SD:   tipo = "SDSC"; break;
            case CARD_SDHC: tipo = "SDHC"; break;
            default:        break;
        }
        boot_msg(GREEN, "SD ok: %s, %llu MiB", tipo, (unsigned long long)sd_mib);
    } else {
        boot_msg(RED, "SD FAIL (ver pista arriba)");
    }

    // Radio y modo promiscuo. Va DESPUÉS de la SD: si el WiFi falla, el
    // informe ya muestra el estado de todo lo anterior.
    esp_err_t err = sniffer_begin(CANAL);
    volcar_log_arranque();   // lo que el driver WiFi haya dicho al arrancar
    if (err == ESP_OK) {
        boot_msg(GREEN, "WiFi promisc ok, canal %u", CANAL);
    } else {
        // esp_err_to_name() traduce el código numérico (esp_err.h) a su nombre.
        boot_msg(RED, "WiFi FAIL: %s", esp_err_to_name(err));
    }

    // Heap libre: los bytes de RAM interna que quedan para reservas dinámicas
    // (malloc/new). El StampS3 no tiene PSRAM, así que este número es todo el
    // presupuesto del buffer de observaciones. Se mide DESPUÉS del WiFi: el
    // driver reserva sus buffers de RX/TX en el heap, y la diferencia con el
    // hito 2 es lo que cuesta el radio. El bloque libre más grande dice si el
    // heap está troceado: es el mayor malloc que saldría bien ahora.
    boot_msg(WHITE, "Heap: %u B (bloque max %u B)",
             (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());

    // Tiempo para leer el informe antes de pasar a la pantalla del sniffer.
    // Sigue también en el monitor serie. Los contadores ya corren: el
    // callback no depende de loop().
    delay(4000);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(2);
}


void loop()
{
    // Una vez por segundo: leer, restar con la lectura anterior y pintar.
    // millis() se compara por diferencia (ahora - ultimo), no por "ahora >
    // siguiente", para que el desbordamiento de los 32 bits (a los ~49 días)
    // no rompa la cuenta: la resta sin signo da el resultado correcto igual.
    static uint32_t ultimo_ms = 0;
    static SnifferCuentas antes = {0, 0, 0};

    uint32_t ahora_ms = millis();
    if (ahora_ms - ultimo_ms < 1000) return;
    ultimo_ms = ahora_ms;

    SnifferCuentas ya = sniffer_leer();

    // Tasa = diferencia entre dos lecturas. También sin signo: si un contador
    // desborda (2^32 tramas, imposible en la práctica a estas tasas), la resta
    // sigue dando el número de tramas del intervalo.
    uint32_t d_mgmt = ya.mgmt - antes.mgmt;
    uint32_t d_data = ya.data - antes.data;
    uint32_t d_misc = ya.misc - antes.misc;
    antes = ya;

    // Pintar es una transacción SPI con el ST7789 (GP-SPI3). Solo se hace
    // aquí, en el núcleo 1: el callback nunca toca el display.
    // printf con ancho fijo (%6lu) y color de fondo: cada carácter nuevo tapa
    // al viejo, sin fillScreen ni parpadeo.
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(CYAN, BLACK);
    M5.Display.printf("Canal %-2u  t=%lus\n", CANAL, (unsigned long)(ahora_ms / 1000));
    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.printf("      /s   total\n");
    M5.Display.setTextColor(GREEN, BLACK);
    M5.Display.printf("MGMT %4lu %7lu\n", (unsigned long)d_mgmt, (unsigned long)ya.mgmt);
    M5.Display.setTextColor(YELLOW, BLACK);
    M5.Display.printf("DATA %4lu %7lu\n", (unsigned long)d_data, (unsigned long)ya.data);
    M5.Display.setTextColor(MAGENTA, BLACK);
    M5.Display.printf("MISC %4lu %7lu\n", (unsigned long)d_misc, (unsigned long)ya.misc);

    Serial.printf("ch%u mgmt %lu/s data %lu/s misc %lu/s\n", CANAL,
                  (unsigned long)d_mgmt, (unsigned long)d_data, (unsigned long)d_misc);
}
