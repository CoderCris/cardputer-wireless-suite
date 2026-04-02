#include <M5Unified.h>

void setup(){

        // M5.begin() inicializa todos los periféricos detectados del Cardputer:
    // - Configura el bus SPI y arranca el driver del display ST7789V2
    // - Inicializa el AXP2101 vía I2C para gestión de energía
    // - Configura el shift register del teclado para lectura
    // En nivel 2 haremos esto manualmente para entender cada paso.
    auto cfg = M5.config();
    M5.begin(cfg);

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
    M5.update();

    if ( M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        char key = M5Cardputer.Keyboard.lastKeyCode();

        if (key != 0){
            M5.Display.print(key);
        }
    }
}