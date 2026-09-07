// ============================================================================
// Banco de pruebas: detección de flancos, aislada del Cardputer.
//
// Este archivo NO lo compila PlatformIO (solo mira en src/). Se compila y se
// EJECUTA en el PC, para poder probar la lógica sin flashear nada:
//
//     g++ -Wall -o /tmp/flancos sandbox/flancos.cpp && /tmp/flancos
//
// El problema, reducido a lo esencial: en cada vuelta te dan el conjunto de
// teclas que están cerradas AHORA. Tienes que emitir solo las que no estaban
// cerradas en la vuelta anterior. Nada de teclado, nada de pantalla, nada de
// buffer de línea: solo eso.
//
// Aquí "word" es una cadena normal ("ab" = las teclas a y b están cerradas a la
// vez). En el Cardputer será estado.word, un vector de char. La lógica es la
// misma; lo único que cambia es de dónde salen los datos.
// ============================================================================

#include <cstdio>
#include <cstring>
#include <cstdint>

constexpr uint8_t MAX_TECLAS = 8;

// Estado persistente entre vueltas. En main.cpp serán las variables 'static'
// dentro de loop(); aquí son globales, que tienen la misma duración estática:
// viven en .bss y sobreviven a cada llamada de la función.
static char    anterior[MAX_TECLAS];
static uint8_t anterior_n = 0;

// Equivalente de M5.Display.print(c). Llámala para cada carácter que decidas
// que es nuevo. La implementa el guion de pruebas, al final del archivo.
void emitir(char c);


// ---------------------------------------------------------------------------
// LO QUE TIENES QUE ESCRIBIR TÚ
//
// Se llama una vez por vuelta del loop. Recibe las teclas cerradas en este
// instante: word[0] .. word[word_n - 1].
//
// Debe llamar a emitir(c) SOLO con los caracteres que no estuvieran ya en
// 'anterior'. Si no hay ninguno nuevo, no emite nada.
// ---------------------------------------------------------------------------
void procesar_vuelta(const char* word, uint8_t word_n)
{
    // TODO 1: recorrer word con un índice — for clásico, de 0 a word_n.
    //bool char_existed = false;
    
        // TODO 2: para el carácter de esta vuelta del bucle, averiguar si ya
        //   estaba en anterior[0 .. anterior_n). Necesitas un segundo bucle
        //   DENTRO del primero, y una bandera bool que empiece en false y se
        //   ponga a true si lo encuentras.

        // TODO 3: si la bandera dice que NO estaba, es una tecla nueva:
        //   emitir(word[i]);

    // TODO 4: fuera del bucle de arriba, copiar word en anterior y dejar
    //   anterior_n valiendo cuántos elementos has copiado. Copia como mucho
    //   MAX_TECLAS. Esto se ejecuta SIEMPRE, también cuando word_n es 0.
    bool char_found;
    for(int i = 0; i < word_n; i++){
        char_found = false;
        for(int j = 0; j < anterior_n; j++){
            if (word[i] == anterior[j]){
                char_found = true;
                break;
            }
        }
        if (!char_found){emitir(word[i]);}
    } 

    anterior_n = 0;
    for(int i = 0; i < word_n && i < MAX_TECLAS; i++){
        anterior[i] = word[i];
        anterior_n++;
    }

}


// ============================================================================
// A partir de aquí es el guion de pruebas. No hace falta que lo toques.
// ============================================================================

static char salida[64];
static int  salida_n = 0;

void emitir(char c)
{
    if (salida_n < (int)sizeof(salida) - 1) salida[salida_n++] = c;
}

struct Vuelta {
    const char* pulsadas;   // teclas cerradas en esta vuelta
    const char* esperado;   // lo que debería emitirse
    const char* comentario;
};

// Teclear "ab" sin soltar la a, soltar las dos, y volver a pulsar la a.
static const Vuelta guion[] = {
    { "",   "",  "nada pulsado"                     },
    { "a",  "a", "pulsas la a"                      },
    { "a",  "",  "la mantienes pulsada"             },
    { "a",  "",  "la sigues manteniendo"            },
    { "ab", "b", "pulsas la b SIN soltar la a"      },
    { "ab", "",  "mantienes las dos"                },
    { "a",  "",  "sueltas la b, la a sigue pulsada" },
    { "",   "",  "sueltas la a"                     },
    { "a",  "a", "vuelves a pulsar la a"            },
};

int main()
{
    printf("vuelta  cerradas  esperado  emitido   caso\n");
    printf("------  --------  --------  -------   -------------------------------\n");

    int fallos = 0;
    for (unsigned v = 0; v < sizeof(guion) / sizeof(guion[0]); ++v) {
        const Vuelta& t = guion[v];

        salida_n = 0;
        memset(salida, 0, sizeof(salida));

        procesar_vuelta(t.pulsadas, (uint8_t)strlen(t.pulsadas));

        const bool ok = (strcmp(salida, t.esperado) == 0);
        if (!ok) ++fallos;

        printf("%6u  %-8s  %-8s  %-7s   %s %s\n",
               v,
               t.pulsadas[0] ? t.pulsadas : "-",
               t.esperado[0] ? t.esperado : "-",
               salida[0]     ? salida     : "-",
               ok ? "ok " : "MAL",
               t.comentario);
    }

    printf("\n%s\n", fallos == 0
        ? "Todo correcto. Esta logica es la que va dentro de loop()."
        : "Hay fallos. Traza a mano la primera vuelta que falla antes de tocar nada.");
    return fallos;
}
