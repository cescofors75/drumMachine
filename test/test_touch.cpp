// Test simple para diagnosticar el touch
// Copia este código a main.cpp para probar solo el touch

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔══════════════════════════╗");
  Serial.println("║   TOUCH TEST MODE        ║");
  Serial.println("╚══════════════════════════╝");
  
  // Init display
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // Backlight
  
  tft.init();
  tft.setRotation(1);  // 480x320
  tft.fillScreen(TFT_BLACK);
  
  // Calibración básica (ajustar según tu pantalla)
  uint16_t calData[5] = {275, 3620, 264, 3532, 1};
  tft.setTouch(calData);
  
  // Instrucciones
  tft.setTextSize(3);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(50, 140);
  tft.println("TOUCH TEST MODE");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(80, 180);
  tft.println("Touch anywhere on screen");
  
  Serial.println("► Touch the screen...");
  Serial.println("► Watching for touch events...\n");
}

void loop() {
  uint16_t x, y;
  
  // Verificar touch
  if(tft.getTouch(&x, &y)) {
    // Debug en serial
    Serial.printf("✓ TOUCH DETECTED! X=%d, Y=%d\n", x, y);
    
    // Dibujar círculo en la posición tocada
    tft.fillCircle(x, y, 10, TFT_RED);
    
    // Mostrar coordenadas en pantalla
    tft.fillRect(0, 0, 480, 50, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 10);
    tft.printf("Touch: X=%d Y=%d", x, y);
    
    delay(100);
  }
  
  // Verificar cada 500ms incluso sin touch
  static unsigned long lastCheck = 0;
  if(millis() - lastCheck > 500) {
    lastCheck = millis();
    Serial.println("[INFO] Listening for touch...");
  }
  
  delay(10);
}
