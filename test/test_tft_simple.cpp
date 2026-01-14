#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n==================================");
    Serial.println("TFT DISPLAY TEST - Simple Version");
    Serial.println("==================================");
    
    // Verificar pines
    Serial.println("\nPin Configuration:");
    Serial.println("TFT_MISO  = 19");
    Serial.println("TFT_MOSI  = 18");
    Serial.println("TFT_SCLK  = 4");
    Serial.println("TFT_CS    = 32");
    Serial.println("TFT_DC    = 33");
    Serial.println("TFT_RST   = 25");
    
    // Inicializar SPI manualmente
    Serial.println("\nInitializing SPI...");
    SPI.begin(4, 19, 18, 32); // SCLK, MISO, MOSI, CS
    
    // Inicializar TFT
    Serial.println("Initializing TFT...");
    tft.init();
    Serial.println("TFT initialized");
    
    // Probar diferentes rotaciones
    Serial.println("\nTesting rotation 0...");
    tft.setRotation(0);
    tft.fillScreen(TFT_RED);
    delay(1000);
    
    Serial.println("Testing rotation 1...");
    tft.setRotation(1);
    tft.fillScreen(TFT_GREEN);
    delay(1000);
    
    Serial.println("Testing rotation 2...");
    tft.setRotation(2);
    tft.fillScreen(TFT_BLUE);
    delay(1000);
    
    Serial.println("Testing rotation 3...");
    tft.setRotation(3);
    tft.fillScreen(TFT_WHITE);
    delay(1000);
    
    // Dibujar patrón de prueba
    Serial.println("\nDrawing test pattern...");
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);
    
    // Barras de colores
    tft.fillRect(0, 0, 80, 240, TFT_RED);
    tft.fillRect(80, 0, 80, 240, TFT_GREEN);
    tft.fillRect(160, 0, 80, 240, TFT_BLUE);
    tft.fillRect(240, 0, 80, 240, TFT_YELLOW);
    
    // Texto
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(50, 110);
    tft.println("TFT TEST");
    
    Serial.println("\n==================================");
    Serial.println("Test complete!");
    Serial.println("If you see colored bars on the");
    Serial.println("display, the TFT is working.");
    Serial.println("==================================\n");
}

void loop() {
    static unsigned long lastTime = 0;
    static int counter = 0;
    
    if (millis() - lastTime > 1000) {
        counter++;
        Serial.println("Counter: " + String(counter) + " (If display is black, check backlight/connections)");
        
        // Alternar píxel en esquina
        if (counter % 2 == 0) {
            tft.drawPixel(0, 0, TFT_WHITE);
        } else {
            tft.drawPixel(0, 0, TFT_BLACK);
        }
        
        lastTime = millis();
    }
}
