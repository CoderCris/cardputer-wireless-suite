// ============================================================================
// Banco de pruebas: buffer de línea, aislado del Cardputer.
//
// Se compila y se EJECUTA en el PC, sin flashear nada:
//
//     g++ -Wall -o /tmp/buffer sandbox/buffer.cpp && /tmp/buffer
//
// El problema, reducido a lo esencial: te van llegando eventos ya resueltos
// (llega un carácter, llega un borrado, llega un confirmar) y tienes que
// mantener el estado de la línea que se está escribiendo.
//
// Nada de teclado, nada de flancos, nada de pantalla. Los flancos ya los
// resolviste en flancos.cpp; aquí se dan por hechos.
//
// El buffer se mantiene SIEMPRE terminado en '\0', para poder entregárselo tal
// cual a algo que espere un const char* (en el Cardputer: M5.Display.print).
// ============================================================================

#include <cstdio>
#include <cstring>
#include <cstdint>

// 20 caracteres útiles: es lo que cabe en una fila del display del Cardputer
// (240 px de ancho / 12 px por carácter con setTextSize(2) sobre el font 6x8).
constexpr uint16_t CAPACIDAD = 20;

// Estado persistente entre eventos. En main.cpp serán 'static' dentro de
// loop(); aquí son globales, misma duración estática: viven en .bss.
//
// El +1 es el sitio del terminador. linea_n NO lo cuenta: si linea_n vale 3,
// hay 3 caracteres válidos y linea[3] es el '\0'.
static char    linea[CAPACIDAD + 1];
static uint8_t linea_n = 0;

// Equivalente de "esta línea ya está terminada, haz algo con ella" (en el
// Cardputer: imprimirla y bajar a la fila siguiente). La implementa el guion
// de pruebas al final del archivo.
void entregar_linea(const char* texto);


// ---------------------------------------------------------------------------
// LO QUE TIENES QUE ESCRIBIR TÚ
//
// Tres funciones. Cada una recibe un evento ya resuelto y actualiza el estado.
// Ninguna de las tres puede escribir fuera de linea[0 .. CAPACIDAD].
// ---------------------------------------------------------------------------

// Ha llegado un carácter imprimible (incluido el espacio).
void insertar(char c)
{
    // TODO 1: si la línea ya está llena, no cabe: no hagas nada y sal.
    //   ¿Con qué valor de linea_n está llena? Cuidado con el fuera-por-uno:
    //   con CAPACIDAD = 20, el último índice válido para un carácter es el 19.

    if (linea_n < CAPACIDAD){
        linea[linea_n] = c;
        linea_n ++;
        linea[linea_n] = '\0';
    }

    // TODO 2: escribir c en la primera casilla libre y avanzar el contador.


    // TODO 3: volver a poner el terminador, ahora una casilla más allá.
}

// Ha llegado un borrado (la tecla Backspace, estado.del en el Cardputer).
void borrar()
{
    // TODO 4: si la línea está vacía no hay nada que borrar: sal sin tocar
    //   nada. Si restas igualmente, linea_n es un uint8_t y 0 - 1 vale 255.
    if (linea_n > 0 ){
        linea_n --;
        linea[linea_n] = '\0';

    }
    // TODO 5: retroceder el contador y dejar el terminador en su nuevo sitio.
    //   El carácter viejo no hace falta borrarlo: deja de ser válido solo con
    //   que linea_n ya no lo alcance. Piensa por qué eso basta.
}

// Ha llegado un confirmar (la tecla Enter, estado.enter en el Cardputer).
void confirmar()
{
    // TODO 6: entregar la línea actual con entregar_linea(...) y dejar el
    //   buffer vacío para empezar una nueva. Una línea vacía también se
    //   entrega: confirmar sin haber escrito nada es un salto de línea.
    
    entregar_linea(linea);
    linea_n = 0;
    linea[linea_n] = '\0';
}


// ============================================================================
// A partir de aquí es el guion de pruebas. No hace falta que lo toques.
// ============================================================================

static char entregadas[8][CAPACIDAD + 1];
static int  entregadas_n = 0;

void entregar_linea(const char* texto)
{
    if (entregadas_n < 8) {
        snprintf(entregadas[entregadas_n], sizeof(entregadas[0]), "%s", texto);
        entregadas_n++;
    }
}

enum Tipo { TEXTO, BORRAR, CONFIRMAR };

struct Evento {
    Tipo        tipo;
    const char* texto;        // solo para TEXTO: se inserta carácter a carácter
    const char* linea_esp;    // cómo debe quedar el buffer tras el evento
    const char* entrega_esp;  // qué debe haberse entregado ("-" = nada)
    const char* comentario;
};

static const Evento guion[] = {
    { TEXTO,     "hola",                 "hola",                 "-",                    "escribes cuatro letras"          },
    { BORRAR,    nullptr,                "hol",                  "-",                    "backspace borra la ultima"       },
    { TEXTO,     "a mundo",              "hola mundo",           "-",                    "sigues escribiendo, con espacio" },
    { CONFIRMAR, nullptr,                "",                     "hola mundo",           "enter entrega y vacia"           },
    { BORRAR,    nullptr,                "",                     "-",                    "backspace con la linea vacia"    },
    { BORRAR,    nullptr,                "",                     "-",                    "y otra vez, no debe romperse"    },
    { CONFIRMAR, nullptr,                "",                     "",                     "enter en vacio: linea vacia"     },
    { TEXTO,     "abcdefghijklmnopqrst", "abcdefghijklmnopqrst", "-",                    "20 caracteres: justo el limite"  },
    { TEXTO,     "XYZ",                  "abcdefghijklmnopqrst", "-",                    "se descartan, no caben"          },
    { BORRAR,    nullptr,                "abcdefghijklmnopqrs",  "-",                    "borrar desde lleno"              },
    { TEXTO,     "Z",                    "abcdefghijklmnopqrsZ", "-",                    "ahora si cabe uno"               },
    { CONFIRMAR, nullptr,                "",                     "abcdefghijklmnopqrsZ", "entrega la linea llena entera"   },
};

int main()
{
    memset(linea, 0, sizeof(linea));

    printf("evt  evento                 buffer esperado       buffer real           entrega\n");
    printf("---  ---------------------  --------------------  --------------------  ---------------------\n");

    int fallos = 0;
    int canario = 0;   // detecta escrituras una casilla más allá del array

    for (unsigned v = 0; v < sizeof(guion) / sizeof(guion[0]); ++v) {
        const Evento& e = guion[v];

        entregadas_n = 0;

        char etiqueta[40];
        switch (e.tipo) {
            case TEXTO:
                snprintf(etiqueta, sizeof(etiqueta), "teclear \"%s\"", e.texto);
                for (const char* p = e.texto; *p; ++p) insertar(*p);
                break;
            case BORRAR:
                snprintf(etiqueta, sizeof(etiqueta), "backspace");
                borrar();
                break;
            case CONFIRMAR:
                snprintf(etiqueta, sizeof(etiqueta), "enter");
                confirmar();
                break;
        }

        const bool ok_linea = (strcmp(linea, e.linea_esp) == 0);

        bool ok_entrega;
        if (strcmp(e.entrega_esp, "-") == 0) {
            ok_entrega = (entregadas_n == 0);
        } else {
            ok_entrega = (entregadas_n == 1 && strcmp(entregadas[0], e.entrega_esp) == 0);
        }

        // El buffer siempre debe estar terminado dentro de sus límites.
        const bool ok_term = (linea_n <= CAPACIDAD && linea[linea_n] == '\0');
        if (!ok_term) canario++;

        const bool ok = ok_linea && ok_entrega && ok_term;
        if (!ok) ++fallos;

        printf("%3u  %-21s  %-20s  %-20s  %-21s %s %s\n",
               v, etiqueta,
               e.linea_esp[0] ? e.linea_esp : "(vacio)",
               linea[0]       ? linea       : "(vacio)",
               entregadas_n == 0 ? "-" : (entregadas[0][0] ? entregadas[0] : "(vacio)"),
               ok ? "ok " : "MAL",
               e.comentario);
    }

    if (canario) printf("\nAVISO: linea_n o el terminador se han salido del array.\n");

    printf("\n%s\n", fallos == 0
        ? "Todo correcto. Esta logica es la que va dentro de loop()."
        : "Hay fallos. Traza a mano el primer evento que falla antes de tocar nada.");
    return fallos;
}
