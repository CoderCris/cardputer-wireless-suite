#include <M5Unified.h>
#include <M5Cardputer.h>   // ya incluye M5Unified.h; el include de arriba es
                           // redundante pero inofensivo (include guards)

// Máximo de teclas imprimibles simultáneas que recordamos entre vueltas.
// Si mantienes más de MAX_TECLAS pulsadas a la vez, las que sobran no caben en
// el historial y se vuelven a detectar como "nuevas" en cada vuelta, repitiendo
// su carácter. Con 4 no pasa escribiendo normal; es un límite consciente.
constexpr uint8_t MAX_TECLAS = 4;

// Caracteres útiles de la línea en edición: lo que cabe en una fila del display
// (240 px de ancho / 12 px por carácter con setTextSize(2) sobre el font 6x8).
constexpr uint8_t CAPACIDAD = 20;


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


void setup()
{
    // M5Cardputer.begin() hace DOS cosas, y por eso no vale M5.begin():
    //
    //  1. Llama a M5.begin(cfg), que inicializa lo que M5Unified conoce de la
    //     placa: configura el bus SPI y arranca el driver del display
    //     ST7789V2, inicializa el AXP2101 por I2C (rieles de energía y carga
    //     de batería), y prepara botones, altavoz y micrófono.
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

    M5.Display.setTextSize(2);             // Escala del font (multiplicador entero)
    M5.Display.setTextColor(GREEN, BLACK); // Color texto, color fondo
    M5.Display.fillScreen(BLACK);          // Limpia el display completo
    M5.Display.setCursor(0, 0);
    M5.Display.println("Terminal ready");
    M5.Display.println("----------------");

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
    // array. El array tiene 21 bytes; los otros 234 son de otras variables.
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

    // Bajar una fila. fontHeight() devuelve la altura del font con el
    // setTextSize() actual ya aplicado.
    prompt_y += M5.Display.fontHeight();

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

    // Borrar es pintar: el ST7789 no sabe qué es un carácter, solo recibe
    // píxeles. fillRect manda el color de fondo a toda la franja de la línea,
    // que es la única forma de hacer desaparecer el carácter que el Backspace
    // acaba de quitar del buffer.
    M5.Display.fillRect(0, prompt_y, M5.Display.width(), alto, BLACK);

    // Y volver a dibujarla entera desde el buffer. print() sin 'ln': la línea
    // en curso no salta de fila; eso solo ocurre al confirmar.
    M5.Display.setCursor(0, prompt_y);
    M5.Display.print(linea);

    // Por qué la línea entera y no solo el hueco del carácter borrado: cada
    // operación de dibujo se traduce en fijar una ventana en el controlador
    // (comandos CASET/RASET) y volcar el rectángulo de píxeles con RAMWR, ver
    // docs/04-protocols/spi-st7789.md. Calcular el rectángulo exacto de un
    // carácter es posible, pero con 20 caracteres a 40 MHz de SPI el ahorro no
    // compensa la complejidad. Optimizar antes de medir es adivinar.
}
