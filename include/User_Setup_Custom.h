// Setup para ESP32 con ILI9486 480x320 + Touch XPT2046
// Copia este archivo a: .pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h
// O mejor aún, usa este como User_Setup_Select.h

#define USER_SETUP_INFO "User_Custom_Setup"

// Driver
#define ILI9486_DRIVER

// Tamaño
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Pines ESP32 para TFT
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4

// Touch XPT2046 - Usar SPI por software con pines custom
#define TOUCH_CS 33

// Método alternativo: definir todos los pines del touch
// Esto fuerza a TFT_eSPI a usar SPI separado para touch
#define TOUCH_CLK  25
#define TOUCH_MOSI 32
#define TOUCH_MISO 27
#define TOUCH_IRQ  26

// Habilitar lectura táctil
#define TOUCH_DRIVER 0x2046  // XPT2046

// Frecuencias
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Fuentes
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// Support
#define SPI_18BIT_DRIVER
