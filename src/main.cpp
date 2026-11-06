#include <M5Unified.h>
#include <M5Cardputer.h>   // ya incluye M5Unified.h; el include de arriba es
                           // redundante pero inofensivo (include guards)
#include <SPI.h>           // objeto global SPI = controlador GP-SPI2
#include <SD.h>            // comandos SD en modo SPI + FatFs encima

// TODO(sd 1): cuatro constantes con los pines de la SD (SCK, MISO, MOSI, CS).
// Fuente: docs/01-hardware/spi-pads-controladores.md. Elige el tipo pensando
// en qué esperan recibir SPI.begin() y SD.begin().

constexpr uint8_t SD_SCK_PIN = 40;
constexpr uint8_t SD_MISO_PIN = 39;
constexpr uint8_t SD_MOSI_PIN = 14;
constexpr uint8_t SD_CS_PIN = 12;


// Máximo de teclas imprimibles simultáneas que recordamos entre vueltas.
// Si mantienes más de MAX_TECLAS pulsadas a la vez, las que sobran no caben en
// el historial y se vuelven a detectar como "nuevas" en cada vuelta, repitiendo
// su carácter. Con 4 no pasa escribiendo normal; es un límite consciente.
constexpr uint8_t MAX_TECLAS = 4;

// Caracteres que caben en UNA FILA del display: 240 px de ancho / 12 px por
// carácter (font 6x8 con setTextSize(2)). Es una propiedad de la pantalla.
constexpr uint8_t COLS_FILA = 20;

// Caracteres que admite la LÍNEA LÓGICA (el buffer). Es una propiedad del
// editor y ya no coincide con la anterior: una línea larga ocupa varias filas.
// 80 son 4 filas de las 8 que tiene el display (135 px / 16 px por fila), así
// que una línea llena siempre cabe entera en una pantalla recién limpiada, que
// es lo que necesita redibujar_linea() para poder recolocarla arriba. Y queda
// lejos de 255, el máximo que puede contar linea_n (uint8_t).
constexpr uint8_t CAPACIDAD = 80;


// ---------------------------------------------------------------------------
// ESTADO PERSISTENTE
//
// Todo esto es 'static' a nivel de archivo: duración estática, vive en .bss
// (el arranque lo pone a cero antes de llamar a setup()). No puede ser local de
// loop(), porque loop() se ejecuta miles de veces por segundo y las locales
// mueren al salir.
// ---------------------------------------------------------------------------

// Buffer de la línea que se está escribiendo. El +1 es el sitio del '\0':
// linea_n NO lo cuenta. Si linea_n vale 3, hay 3 caracteres y linea[3] es '\0'.
// Se mantiene SIEMPRE terminado, para poder pasarlo tal cual a print().
static char    linea[CAPACIDAD + 1];
static uint8_t linea_n = 0;

// Detector de flancos de las teclas imprimibles: qué caracteres había pulsados
// en la vuelta anterior, para distinguir "se acaba de pulsar" de "sigue
// pulsada". El driver no tiene historial: republica en cada barrido lo que ve.
static char    last_word[MAX_TECLAS];
static uint8_t last_word_n = 0;

// Detector de flancos de las teclas de control. Backspace y Enter NO están en
// estado.word: son dos flags que valen true mientras la tecla siga pulsada
// (Keyboard.cpp:174-183). Sin flanco propio, mantener Enter medio segundo
// entregaría una línea por cada vuelta de loop().
static bool last_del   = false;
static bool last_enter = false;

// Fila (coordenada Y en píxeles) donde se dibuja la línea en curso. Baja al
// confirmar: lo ya entregado queda encima, congelado.
static int prompt_y = 0;


// Declaraciones adelantadas: el compilador necesita la firma antes de verlas
// usadas en loop(); las definiciones están más abajo.
static bool insertar(char c);
static bool borrar();
static void confirmar();
static void redibujar_linea();
static int  filas_linea();


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

    // Heap libre: los bytes de RAM interna que quedan para reservas dinámicas
    // (malloc/new) tras arrancar display, teclado y el driver de la SD. El
    // StampS3 no tiene PSRAM, así que este número es todo el presupuesto del
    // buffer de observaciones antes de volcarlas a la SD. El bloque libre más
    // grande dice si ese heap está troceado: es el tamaño máximo de un único
    // malloc que saldría bien ahora mismo.
    boot_msg(WHITE, "Heap: %u B (bloque max %u B)",
             (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());

    // Una línea en blanco separa el informe de arranque del terminal.
    M5.Display.println();
    M5.Display.setTextSize(2);             // Escala del terminal
    M5.Display.setTextColor(GREEN, BLACK);

    // La línea en curso empieza justo debajo de la cabecera. getCursorY() ya
    // tiene en cuenta el setTextSize() activo: si mañana cambias la escala,
    // esto sigue cuadrando. Un número de píxeles a pelo, no.
    prompt_y = M5.Display.getCursorY();

    // El buffer arranca vacío pero terminado: print(linea) sobre un array sin
    // '\0' leería memoria adyacente hasta encontrar un cero por casualidad.
    linea[0] = '\0';
}


void loop()
{
    // M5Cardputer.update() llama a M5.update() y además dispara el barrido del
    // teclado, que es lo que M5.update() por sí solo no hace:
    //   - updateKeyList()   -> recorre los 8 pasos de la matriz. En cada paso
    //                          baja una línea de selección y lee los 7 pines de
    //                          entrada de golpe, empaquetados en un uint8_t.
    //   - updateKeysState() -> traduce esas coordenadas físicas a caracteres,
    //                          en dos pasadas: primero resuelve los
    //                          modificadores (shift, ctrl...), después traduce
    //                          las teclas imprimibles con ese estado ya fijado.
    M5Cardputer.update();

    // keysState() devuelve el estado YA traducido de esta vuelta. Se toma por
    // referencia const (&) para no copiar el struct (tres std::vector dentro)
    // en cada iteración del loop.
    const auto& estado = M5Cardputer.Keyboard.keysState();

    // Dirty flag: marca si el buffer ha cambiado en esta vuelta. Mandar píxeles
    // por SPI es lo más caro que hace este loop, así que el repintado se hace
    // UNA vez al final y solo si algo cambió, no dentro de cada rama.
    bool sucio = false;


    // -----------------------------------------------------------------------
    // BLOQUE A — Teclas imprimibles (incluido el espacio)
    //
    // Para cada carácter que el barrido ve ahora, comprobar si ya estaba en la
    // vuelta anterior. Si no estaba, es un flanco de subida: tecla recién
    // pulsada, y hay que atenderla una sola vez.
    // -----------------------------------------------------------------------
    for (size_t i = 0; i < estado.word.size(); i++) {
        bool ya_estaba = false;

        for (uint8_t j = 0; j < last_word_n; j++) {
            if (last_word[j] == estado.word[i]) {
                ya_estaba = true;
                break;
            }
        }

        if (!ya_estaba) {
            // insertar() devuelve false si el buffer estaba lleno y descartó el
            // carácter. En ese caso nada ha cambiado y no hay que repintar: el
            // OR acumulativo (|=) deja 'sucio' como estaba.
            sucio |= insertar(estado.word[i]);
        }
    }

    // Guardar el estado de esta vuelta para poder comparar en la siguiente.
    // El recorte a MAX_TECLAS es el límite comentado arriba.
    last_word_n = 0;
    for (size_t i = 0; i < estado.word.size() && last_word_n < MAX_TECLAS; i++) {
        last_word[last_word_n] = estado.word[i];
        last_word_n++;
    }


    // -----------------------------------------------------------------------
    // BLOQUE B — Teclas de control (Backspace y Enter)
    //
    // El mismo flanco de subida, pero sobre dos booleanos: interesa el paso de
    // false a true, no el "está pulsada".
    // -----------------------------------------------------------------------
    if (estado.del && !last_del) {
        sucio |= borrar();        // false si la línea ya estaba vacía
    }

    if (estado.enter && !last_enter) {
        confirmar();
        sucio = true;             // confirmar siempre cambia el estado visible
    }

    // Actualizar el estado anterior DESPUÉS de las comparaciones. Al revés, el
    // valor de ahora y el "anterior" coincidirían siempre y no habría flanco.
    last_del   = estado.del;
    last_enter = estado.enter;


    // -----------------------------------------------------------------------
    // BLOQUE C — Reflejo en pantalla
    //
    // La pantalla no acumula: refleja el buffer, que es la única fuente de
    // verdad. Solo se toca el bus SPI si algo ha cambiado.
    // -----------------------------------------------------------------------
    if (sucio) {
        redibujar_linea();
    }
}


// ---------------------------------------------------------------------------
// BUFFER DE LÍNEA
//
// Las tres operaciones del editor. Devuelven si el estado cambió, para que el
// llamante sepa si hace falta repintar.
// ---------------------------------------------------------------------------

// Ha llegado un carácter imprimible. Devuelve false si no cabía.
static bool insertar(char c)
{
    // Lleno: linea_n ya vale CAPACIDAD, el último índice válido para un
    // carácter es CAPACIDAD-1 y linea[CAPACIDAD] es el terminador. Escribir
    // aquí sería salirse del array.
    if (linea_n >= CAPACIDAD) {
        return false;
    }

    linea[linea_n] = c;     // primera casilla libre
    linea_n++;
    linea[linea_n] = '\0';  // el terminador se corre una casilla
    return true;
}

// Ha llegado un Backspace. Devuelve false si no había nada que borrar.
static bool borrar()
{
    // linea_n es uint8_t: si restas con la línea vacía, 0 - 1 no da -1, da 255,
    // y la siguiente escritura del terminador se va a linea[255], fuera del
    // array (CAPACIDAD + 1 bytes); lo que hay ahí es de otras variables.
    if (linea_n == 0) {
        return false;
    }

    linea_n--;
    linea[linea_n] = '\0';  // el carácter viejo no hace falta borrarlo: deja de
                            // ser válido con que linea_n ya no lo alcance
    return true;
}

// Ha llegado un Enter: la línea se da por terminada.
static void confirmar()
{
    // Dibujar la línea ANTES de moverse de fila. No basta con confiar en que el
    // repintado de la vuelta anterior ya la dejó pintada: si en esta misma
    // vuelta han llegado un carácter y el Enter a la vez (el barrido ve las dos
    // teclas), ese último carácter nunca se habría llegado a dibujar.
    redibujar_linea();

    // Bajar tantas filas como ocupa la línea recién confirmada: con el wrap ya
    // no es siempre una. fontHeight() devuelve la altura del font con el
    // setTextSize() actual ya aplicado. Se calcula ANTES de vaciar el buffer,
    // porque filas_linea() depende de linea_n.
    prompt_y += filas_linea() * M5.Display.fontHeight();

    // Si la siguiente fila ya no cabe entera en el display, se limpia la
    // pantalla y se empieza otra vez arriba.
    //
    // La alternativa real sería desplazar el contenido hacia arriba una fila,
    // pero eso exige o bien leer de vuelta la GRAM del ST7789 (sin MISO: solo
    // por SPI de 3 hilos, a 16 MHz, y sin comprobar en hardware) o mantener en
    // RAM el historial de líneas y repintarlo entero. Lo segundo es lo correcto
    // porque la fuente de verdad es el buffer, no la pantalla. Llega en el hito
    // 2, cuando aparezca el store: entonces el historial ya existirá y esto se
    // reescribe. Hasta entonces, limpiar es honesto y cuesta cuatro líneas.
    if (prompt_y + M5.Display.fontHeight() > M5.Display.height()) {
        M5.Display.fillScreen(BLACK);
        prompt_y = 0;
    }

    // Vaciar el buffer para la línea siguiente. Una línea vacía también se
    // confirma: Enter sin haber escrito nada es un salto de línea.
    linea_n  = 0;
    linea[0] = '\0';
}


// ---------------------------------------------------------------------------
// REFLEJO EN PANTALLA
// ---------------------------------------------------------------------------

static void redibujar_linea()
{
    const int alto = M5.Display.fontHeight();

    // Con el wrap la línea crece hacia abajo MIENTRAS se escribe, no solo al
    // pulsar Enter. Si su última fila ya no cabe en el display, se aplica la
    // misma política que en confirmar(): limpiar y recolocar la línea arriba.
    // Cabe seguro: CAPACIDAD se eligió para que una línea llena no pase de
    // media pantalla.
    if (prompt_y + filas_linea() * alto > M5.Display.height()) {
        M5.Display.fillScreen(BLACK);
        prompt_y = 0;
    }

    // Borrar es pintar: el ST7789 no sabe qué es un carácter, solo recibe
    // píxeles. fillRect manda el color de fondo, que es la única forma de hacer
    // desaparecer el carácter que el Backspace acaba de quitar del buffer.
    //
    // Se limpia desde prompt_y hasta el BORDE INFERIOR del display, no solo las
    // filas que la línea ocupa ahora. Si la línea tenía 21 caracteres (dos
    // filas) y un Backspace la deja en 20 (una), limpiar "las filas actuales"
    // dejaría el carácter 21 huérfano en la fila de abajo. Limpiar hasta abajo
    // no necesita recordar cuántas filas ocupaba antes, y es correcto porque
    // por debajo del prompt nunca hay nada que conservar: lo confirmado queda
    // siempre encima.
    M5.Display.fillRect(0, prompt_y, M5.Display.width(),
                        M5.Display.height() - prompt_y, BLACK);

    // Y volver a dibujarla entera desde el buffer. print() sin 'ln': el salto
    // de fila al confirmar lo lleva prompt_y. Los saltos DENTRO de la línea los
    // hace M5GFX por su cuenta (_textwrap_x, activo por defecto): al ir a
    // dibujar un carácter que no cabe, pone el cursor en x = 0 y baja una fila.
    M5.Display.setCursor(0, prompt_y);
    M5.Display.print(linea);

    // Por qué la línea entera y no solo el hueco del carácter borrado: cada
    // operación de dibujo se traduce en fijar una ventana en el controlador
    // (comandos CASET/RASET) y volcar el rectángulo de píxeles con RAMWR, ver
    // docs/04-protocols/spi-st7789.md. Calcular el rectángulo exacto de un
    // carácter es posible, pero con 80 caracteres como mucho a 40 MHz de SPI el
    // ahorro no compensa la complejidad. Optimizar antes de medir es adivinar.
}


// Cuántas filas de pantalla ocupa la línea en curso.
static int filas_linea()
{
    // La línea vacía también ocupa su fila: es donde está el prompt.
    if (linea_n == 0) {
        return 1;
    }

    // M5GFX salta de fila al ir a dibujar el carácter que NO cabe
    // (LGFXBase.cpp:2413-2419: 'if (_cursor_x + w > rlimit)'), no al llenar la
    // fila. Con 20 caracteres el último acaba justo en el píxel 240 y no hay
    // salto: 1 fila. El 21 es el que abre la segunda.
    //
    // De ahí el -1 antes de dividir (la división entera trunca):
    //   linea_n = 20 -> 19 / 20 = 0 -> 1 fila
    //   linea_n = 21 -> 20 / 20 = 1 -> 2 filas
    // Sin el -1, 20 / 20 + 1 daría 2 y confirmar() dejaría una fila en blanco
    // de más cada vez que una línea llenara la fila exacta.
    return (linea_n - 1) / COLS_FILA + 1;
}
