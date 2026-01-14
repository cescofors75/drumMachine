/*
 * Test RAW del Touch XPT2046
 * Este programa lee directamente del chip touch sin usar TFT_eSPI
 */

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Pines del Touch
#define T_CLK  25
#define T_CS   33
#define T_DIN  32  // MOSI
#define T_DO   27  // MISO
#define T_IRQ  26

// Comandos XPT2046
#define CMD_X_READ  0xD0
#define CMD_Y_READ  0x90

SPIClass touchSPI(HSPI);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔══════════════════════════════╗");
  Serial.println("║  XPT2046 RAW TOUCH TEST      ║");
  Serial.println("╚══════════════════════════════╝\n");
  
  // Configurar pantalla (solo para backlight y visual)
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(20, 100);
  tft.println("XPT2046 RAW TEST MODE");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 140);
  tft.println("Touch screen anywhere");
  tft.setCursor(20, 170);
  tft.println("Check Serial Monitor");
  
  // Configurar pines del touch
  pinMode(T_CS, OUTPUT);
  pinMode(T_IRQ, INPUT);
  digitalWrite(T_CS, HIGH);
  
  // Inicializar SPI para touch
  touchSPI.begin(T_CLK, T_DO, T_DIN, T_CS);
  
  Serial.println("✓ Pines configurados:");
  Serial.printf("  T_CLK  = GPIO %d\n", T_CLK);
  Serial.printf("  T_CS   = GPIO %d\n", T_CS);
  Serial.printf("  T_DIN  = GPIO %d (MOSI)\n", T_DIN);
  Serial.printf("  T_DO   = GPIO %d (MISO)\n", T_DO);
  Serial.printf("  T_IRQ  = GPIO %d\n\n", T_IRQ);
  
  Serial.println("✓ SPI Touch inicializado");
  Serial.println("► Esperando toque en pantalla...\n");
  
  delay(1000);
}

uint16_t readTouchRaw(uint8_t command) {
  uint16_t data = 0;
  
  digitalWrite(T_CS, LOW);
  delayMicroseconds(10);
  
  touchSPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));
  touchSPI.transfer(command);
  delayMicroseconds(10);
  
  data = touchSPI.transfer(0x00) << 8;
  data |= touchSPI.transfer(0x00);
  
  touchSPI.endTransaction();
  digitalWrite(T_CS, HIGH);
  
  return data >> 3; // 12-bit value
}

bool isTouched() {
  // T_IRQ es LOW cuando está tocado (activo bajo)
  return (digitalRead(T_IRQ) == LOW);
}

void loop() {
  static unsigned long lastPrint = 0;
  static int touchCount = 0;
  
  // Verificar si está tocado por IRQ
  bool touched = isTouched();
  
  if(touched) {
    // Leer coordenadas raw
    uint16_t rawX = readTouchRaw(CMD_X_READ);
    uint16_t rawY = readTouchRaw(CMD_Y_READ);
    
    // Promediar 3 lecturas para estabilidad
    for(int i = 0; i < 2; i++) {
      rawX += readTouchRaw(CMD_X_READ);
      rawY += readTouchRaw(CMD_Y_READ);
    }
    rawX /= 3;
    rawY /= 3;
    
    touchCount++;
    
    Serial.printf("✓ TOUCH #%d - Raw: X=%d Y=%d | IRQ=%d\n", 
                  touchCount, rawX, rawY, digitalRead(T_IRQ));
    
    // Mostrar en pantalla
    tft.fillRect(0, 210, 480, 100, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 220);
    tft.printf("Touch #%d", touchCount);
    tft.setCursor(20, 250);
    tft.setTextColor(TFT_WHITE);
    tft.printf("Raw X: %d", rawX);
    tft.setCursor(20, 280);
    tft.printf("Raw Y: %d", rawY);
    
    // Dibujar punto
    int screenX = map(rawX, 200, 3800, 0, 480);
    int screenY = map(rawY, 200, 3800, 0, 320);
    tft.fillCircle(screenX, screenY, 8, TFT_RED);
    
    delay(100);
    
  } else {
    // Info periódica cuando no está tocado
    if(millis() - lastPrint > 3000) {
      lastPrint = millis();
      Serial.printf("[INFO] Listening... IRQ=%d (should be HIGH when not touched)\n", 
                    digitalRead(T_IRQ));
      Serial.printf("       Total touches detected: %d\n\n", touchCount);
    }
  }
  
  delay(10);
}
