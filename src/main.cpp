#include <M5Unified.h>
#include <M5Cardputer.h>   // ya incluye M5Unified.h; el include de arriba es
                           // redundante pero inofensivo (include guards)

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

    // isChange() compara SOLO el número de teclas pulsadas respecto a la vuelta
    // anterior, no cuáles. Sirve para no repetir el carácter mientras mantienes
    // la tecla, pero pierde el caso de soltar una y pulsar otra en la misma
    // vuelta (el contador pasa de 1 a 1 y no detecta cambio).
    // isPressed() devuelve CUÁNTAS teclas hay pulsadas (uint8_t, no bool).
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {

        // TODO: coger el estado del teclado por referencia constante.
        //   keysState() devuelve una referencia al struct KeysState con el
        //   estado de este instante. Cógelo por referencia: copiarlo copiaría
        //   también los std::vector que lleva dentro, y eso sería reservar
        //   memoria dinámica en cada pulsación.
        //
        //   const auto& estado = ... ;

        // TODO: recorrer estado.word e imprimir cada carácter en el display.
        //   word es un std::vector<char> con los caracteres imprimibles
        //   pulsados en este instante, ya resueltos con shift y caps lock.
        //   Es un vector y no un char porque el barrido encuentra todas las
        //   teclas cerradas a la vez y no sabe cuál pulsaste "última".
        //
        //   Las teclas especiales (enter, tab, backspace) NO están aquí: se
        //   desvían a hid_keys. Los modificadores tampoco. Por eso ya no hace
        //   falta la comprobación "if (key != 0)": si solo tienes shift
        //   pulsado, word está vacío y el bucle simplemente no itera.
        //
        //   Pista: bucle for basado en rango -> for (char c : ...) { ... }
        //   Para pintar: M5.Display.print(c);
    }
}
