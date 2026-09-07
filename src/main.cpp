#include <M5Unified.h>
#include <M5Cardputer.h>   // ya incluye M5Unified.h; el include de arriba es
                           // redundante pero inofensivo (include guards)

constexpr uint8_t MAX_TECLAS = 4; 

void setup(){

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

    // El display usa un buffer en memoria (canvas) que luego se envía
    // completo al ST7789 por SPI. Esto evita flickering: en lugar de
    // escribir píxel a píxel al display, compones todo el frame en RAM
    // y lo envías de golpe. Se llama "double buffering" simplificado.
    M5.Display.setTextSize(2);         // Escala del font (multiplicador entero)
    M5.Display.setTextColor(GREEN, BLACK); // Color texto, color fondo
    M5.Display.fillScreen(BLACK);      // Limpia el display completo
    M5.Display.setCursor(0, 0);        // Posición del cursor: esquina superior izquierda
    M5.Display.println("Terminal ready");
    M5.Display.println("----------------");
}

void loop(){


    static char buffer[21];
    static uint8_t buffer_pointer;

    static char last_word[MAX_TECLAS];
    static uint8_t last_word_n = 0;
    // M5Cardputer.update() llama a 5.update() y además dispara elbarrido del
    // teclado, que es lo que M5.update() por sí solo no hace:
    //   - updateKeyList()   -> recorre los 8 pasos de la matriz. En cada paso
    //                          baja una línea de selección y lee los 7 pines de
    //                          entrada de golpe, empaquetados en un uint8_t.
    //   - updateKeysState() -> traduce esas coordenadas físicas a caracteres,
    //                          en dos pasadas: primero resuelve los
    //                          modificadores (shift, ctrl...), después traduce
    //                          las teclas imprimibles con ese estado ya fijado.
    M5Cardputer.update();
 

    // isChange() compara SOLO el número de teclas pulsadas respecto a la vuelta
    // anterior, no cuáles. Sirve para no repetir el carácter mientras mantienes
    // la tecla, pero pierde el caso de soltar una y pulsar otra en la misma
    // vuelta (el contador pasa de 1 a 1 y no detecta cambio).
    // isPressed() devuelve CUÁNTAS teclas hay pulsadas (uint8_t, no bool).
    const auto& estado = M5Cardputer.Keyboard.keysState() ;
    bool char_found = false;

    for (int i = 0 ; i < estado.word.size() ; i++){
        for (int j = 0; j < last_word_n ; j++){
            if (last_word[j] == estado.word[i]){
                char_found = true;
                break;
            }
        }
        if (! char_found){
            M5.Display.print(estado.word[i]);
        }
        
        char_found = false;
    }
    
    last_word_n = 0;
    for (int i = 0; i < estado.word.size() && i < MAX_TECLAS; i++){
        last_word[i] = estado.word[i];
        last_word_n++;
        /* code */
    }

}
