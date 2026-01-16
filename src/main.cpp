#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TM1638plus.h>
#include <HardwareSerial.h>
//#include <RotaryEncoder.h>
#include <SD.h>
#include <SPI.h>

// ============================================
// PIN DEFINITIONS
// ============================================

// TFT Display (SPI) - Sin Touch
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_SCK  18
#define TFT_MISO 19
//#define TFT_BL   -1

// TM1638 Module 1 (Steps 1-8)
#define TM1638_1_STB 33
#define TM1638_1_CLK 25
#define TM1638_1_DIO 26

// TM1638 Module 2 (Steps 9-16)
#define TM1638_2_STB 32
#define TM1638_2_CLK 25
#define TM1638_2_DIO 27

// Rotary Encoder (5 pins)
#define ENCODER_CLK 21
#define ENCODER_DT  22
#define ENCODER_SW  13

// 3 BUTTON PANEL - SMART OPEN ANALOG BUTTONS (3 pins: GND, VCC, SIG)
#define ANALOG_BUTTONS_PIN 34  // Pin analógico para leer los 3 botones

// Rangos de valores ADC para cada botón (CALIBRADOS según medición real)
#define BTN_PLAY_STOP_MIN  550   // Botón 1: Play/Stop (medido: ~663)
#define BTN_PLAY_STOP_MAX  800
#define BTN_MUTE_MIN       1350  // Botón 2: Mute/Clear (medido: ~1503)
#define BTN_MUTE_MAX       1650
#define BTN_BACK_MIN       2200  // Botón 3: Back (medido: ~2333)
#define BTN_BACK_MAX       2500
#define BTN_NONE_THRESHOLD 50    // Ningún botón presionado

// Rotary Angle Potentiometer (3 pins)
#define ROTARY_ANGLE_PIN 35

// DFPlayer Mini (MP3/WAV Player) - Comunicación directa
#define DFPLAYER_TX 17  // ESP32 TX -> DFPlayer RX (Pin 2)
#define DFPLAYER_RX 16  // ESP32 RX -> DFPlayer TX (Pin 3)

// SD Card Reader (6 pins SPI)
#define SD_CS   15
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK  18

// ============================================
// CONSTANTS
// ============================================
#define MAX_STEPS 16
#define MAX_TRACKS 8
#define MAX_PATTERNS 16
#define MAX_KITS 3
#define MIN_BPM 40
#define MAX_BPM 240
#define DEFAULT_BPM 120
#define DEFAULT_VOLUME 15
#define MAX_SAMPLES 8
#define DFPLAYER_VOLUME 25  // Volumen DFPlayer (0-30)

// ============================================
// SISTEMA DE TEMAS VISUALES
// ============================================
struct ColorTheme {
    const char* name;
    uint16_t bg;
    uint16_t primary;
    uint16_t primaryLight;
    uint16_t accent;
    uint16_t accent2;
    uint16_t text;
    uint16_t textDim;
    uint16_t success;
    uint16_t warning;
    uint16_t error;
    uint16_t border;
};

// TEMA 1: RED808 - Rojo Corporativo
const ColorTheme THEME_RED808 = {
    "RED808",
    0x1800,      // BG: Rojo muy oscuro
    0xC000,      // PRIMARY: Rojo intenso
    0xE186,      // PRIMARY_LIGHT: Rojo claro
    0xF800,      // ACCENT: Rojo brillante puro
    0xFD20,      // ACCENT2: Naranja-rojo
    0xFFFF,      // TEXT: Blanco
    0xE73C,      // TEXT_DIM: Gris cálido
    0x07E0,      // SUCCESS: Verde
    0xFFE0,      // WARNING: Amarillo
    0xF800,      // ERROR: Rojo
    0x9800       // BORDER: Rojo oscuro
};

// TEMA 2: NAVY - Azul Profesional (original)
const ColorTheme THEME_NAVY = {
    "NAVY",
    0x0821,      // BG: Azul oscuro
    0x1082,      // PRIMARY: Navy
    0x2945,      // PRIMARY_LIGHT: Navy claro
    0x3D8F,      // ACCENT: Azul medio
    0x04FF,      // ACCENT2: Cian
    0xFFFF,      // TEXT: Blanco
    0xBDF7,      // TEXT_DIM: Gris azulado
    0x07E0,      // SUCCESS: Verde
    0xFD20,      // WARNING: Naranja
    0xF800,      // ERROR: Rojo
    0x4A49       // BORDER: Azul gris
};

// TEMA 3: CYBERPUNK - Púrpura/Magenta
const ColorTheme THEME_CYBER = {
    "CYBER",
    0x1006,      // BG: Púrpura muy oscuro
    0x780F,      // PRIMARY: Púrpura intenso
    0xA817,      // PRIMARY_LIGHT: Púrpura claro
    0xF81F,      // ACCENT: Magenta brillante
    0x07FF,      // ACCENT2: Cian eléctrico
    0xFFFF,      // TEXT: Blanco
    0xDEFB,      // TEXT_DIM: Gris lavanda
    0x07E0,      // SUCCESS: Verde neón
    0xFFE0,      // WARNING: Amarillo
    0xF81F,      // ERROR: Magenta
    0x8010       // BORDER: Púrpura medio
};

// TEMA 4: EMERALD - Verde/Menta
const ColorTheme THEME_EMERALD = {
    "EMERALD",
    0x0420,      // BG: Verde muy oscuro
    0x0540,      // PRIMARY: Verde bosque
    0x2E86,      // PRIMARY_LIGHT: Verde medio
    0x07E0,      // ACCENT: Verde brillante
    0x07FF,      // ACCENT2: Turquesa
    0xFFFF,      // TEXT: Blanco
    0xCE79,      // TEXT_DIM: Gris verdoso
    0x07E0,      // SUCCESS: Verde
    0xFFE0,      // WARNING: Amarillo
    0xF800,      // ERROR: Rojo
    0x0460       // BORDER: Verde oscuro
};

// Array de temas disponibles
const ColorTheme* THEMES[] = {&THEME_RED808, &THEME_NAVY, &THEME_CYBER, &THEME_EMERALD};
const int THEME_COUNT = 4;

// Tema actual (RED808 por defecto)
int currentTheme = 0;
const ColorTheme* activeTheme = &THEME_RED808;

// Macros para acceso rápido a colores del tema activo
#define COLOR_BG           (activeTheme->bg)
#define COLOR_PRIMARY      (activeTheme->primary)
#define COLOR_PRIMARY_LIGHT (activeTheme->primaryLight)
#define COLOR_ACCENT       (activeTheme->accent)
#define COLOR_ACCENT2      (activeTheme->accent2)
#define COLOR_TEXT         (activeTheme->text)
#define COLOR_TEXT_DIM     (activeTheme->textDim)
#define COLOR_SUCCESS      (activeTheme->success)
#define COLOR_WARNING      (activeTheme->warning)
#define COLOR_ERROR        (activeTheme->error)
#define COLOR_BORDER       (activeTheme->border)

// Compatibilidad con código anterior
#define COLOR_NAVY         (activeTheme->primary)
#define COLOR_NAVY_LIGHT   (activeTheme->primaryLight)

// COLORES VIVOS PARA CADA INSTRUMENTO (8 colores luminosos)
#define COLOR_INST_KICK    0xF800  // Rojo brillante - Bass Drum
#define COLOR_INST_SNARE   0xFD20  // Naranja - Snare Drum
#define COLOR_INST_CLHAT   0xFFE0  // Amarillo - Closed Hi-Hat
#define COLOR_INST_OPHAT   0x07FF  // Cian - Open Hi-Hat
#define COLOR_INST_CLAP    0xF81F  // Magenta - Clap
#define COLOR_INST_TOMLO   0x07E0  // Verde - Tom Low
#define COLOR_INST_TOMHI   0x3E7F  // Verde agua - Tom High
#define COLOR_INST_CYMBAL  0x4A7F  // Azul claro - Cymbal

// ============================================
// ENUMS
// ============================================
enum Screen {
    SCREEN_BOOT,
    SCREEN_MENU,
    SCREEN_LIVE,
    SCREEN_SEQUENCER,
    SCREEN_SETTINGS,
    SCREEN_DIAGNOSTICS
};

enum DisplayMode {
    DISPLAY_BPM,
    DISPLAY_VOLUME,
    DISPLAY_PATTERN,
    DISPLAY_KIT,
    DISPLAY_INSTRUMENT,
    DISPLAY_STEP
};

enum SamplerMode {
    SAMPLER_ONESHOT,   // Reproducir una vez
    SAMPLER_LOOP,      // Loop continuo
    SAMPLER_HOLD,      // Mantener mientras se presiona
    SAMPLER_REVERSE    // Reproducir al revés (si es posible)
};

enum SamplerFunction {
    FUNC_NONE = 0,
    FUNC_LOOP = 8,      // S9: Toggle Loop mode
    FUNC_HOLD = 9,      // S10: Toggle Hold mode  
    FUNC_STOP = 10,     // S11: Stop all
    FUNC_VOLUME_UP = 11,// S12: Volumen +
    FUNC_VOLUME_DN = 12,// S13: Volumen -
    FUNC_PREV = 13,     // S14: Sample anterior
    FUNC_NEXT = 14,     // S15: Sample siguiente
    FUNC_CLEAR = 15     // S16: Clear/Reset
};

// ============================================
// STRUCTURES
// ============================================
struct Pattern {
    bool steps[MAX_TRACKS][MAX_STEPS];
    bool muted[MAX_TRACKS];
    String name;
};

struct DrumKit {
    String name;
    int folder;
};

struct SampleInfo {
    String name;
    int number;        // 1-8
    bool isLooping;
    bool isPlaying;
    int volume;
    SamplerMode mode;
    
    SampleInfo() : 
        name(""),
        number(0),
        isLooping(false),
        isPlaying(false),
        volume(DFPLAYER_VOLUME),
        mode(SAMPLER_ONESHOT) {}
};

struct DiagnosticInfo {
    bool tftOk;
    bool tm1638_1_Ok;
    bool tm1638_2_Ok;
    bool encoderOk;
    bool sdCardOk;
    bool dfplayerOk;
    int filesFound;
    String lastError;
    
    DiagnosticInfo() : 
        tftOk(false), 
        tm1638_1_Ok(false), 
        tm1638_2_Ok(false), 
        encoderOk(false), 
        sdCardOk(false), 
        dfplayerOk(false),
        filesFound(0), 
        lastError("") {}
};

// ============================================
// GLOBAL OBJECTS
// ============================================
TFT_eSPI tft = TFT_eSPI();
TM1638plus tm1(TM1638_1_STB, TM1638_1_CLK, TM1638_1_DIO, true);
TM1638plus tm2(TM1638_2_STB, TM1638_2_CLK, TM1638_2_DIO, true);
HardwareSerial dfplayerSerial(1);  // UART1 para DFPlayer (pines configurables)
// RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::TWO03);

// GLOBAL VARIABLES
// ============================================
Pattern patterns[MAX_PATTERNS];
DrumKit kits[MAX_KITS];
DiagnosticInfo diagnostic;

Screen currentScreen = SCREEN_BOOT;

int currentPattern = 0;
int currentKit = 0;
int currentStep = 0;
int tempo = DEFAULT_BPM;
unsigned long lastStepTime = 0;
unsigned long stepInterval = 0;
bool isPlaying = false;

int selectedTrack = 0;
int selectedStep = 0;
int menuSelection = 0;

bool needsFullRedraw = true;
bool needsHeaderUpdate = false;
bool needsGridUpdate = false;
int lastDisplayedStep = -1;
int lastToggledTrack = -1; // Track que cambió para actualizar solo su fila

int lastEncoderPos = 0;

// Rotary Encoder variables (sin biblioteca)
volatile int encoderPos = 0;
volatile bool encoderChanged = false;
static uint8_t encoderState = 0;
static const int8_t encoderStates[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

int volumeLevel = DEFAULT_VOLUME;
int lastVolumeLevel = DEFAULT_VOLUME;
unsigned long lastVolumeRead = 0;

unsigned long ledOffTime[16] = {0};
bool ledActive[16] = {false};

int audioLevels[8] = {0};
unsigned long lastVizUpdate = 0;

DisplayMode currentDisplayMode = DISPLAY_BPM;
unsigned long lastDisplayChange = 0;
int lastInstrumentPlayed = -1;
unsigned long instrumentDisplayTime = 0;

// Debug analog buttons
unsigned long lastButtonDebug = 0;
int lastAdcValue = -1;

// Sampler variables
SampleInfo samples[MAX_SAMPLES];
int currentSample = 0;
int dfplayerVolume = DFPLAYER_VOLUME;
SamplerMode globalSamplerMode = SAMPLER_ONESHOT;
bool samplerInitialized = false;
int lastPlayedSample = -1;
unsigned long samplePlayTime = 0;

// Variables para repetición de samples al mantener botón
uint16_t lastButtonState = 0;
unsigned long buttonPressTime[16] = {0};
unsigned long lastRepeatTime[16] = {0};
const unsigned long HOLD_THRESHOLD = 250;  // Reducido para respuesta más rápida
const unsigned long REPEAT_INTERVAL = 120; // Más rápido y estable

// ============================================
// DATA
// ============================================
const char* kitNames[MAX_KITS] = {
    "808 CLASSIC",
    "808 BRIGHT",
    "808 DRY"
};

const char* trackNames[MAX_TRACKS] = {
    "BD", "SD", "CH", "OH",
    "CL", "T1", "T2", "CY"
};

const char* instrumentNames[MAX_TRACKS] = {
    "KICK    ",
    "SNARE   ",
    "CL-HAT  ",
    "OP-HAT  ",
    "CLAP    ",
    "TOM-LO  ",
    "TOM-HI  ",
    "CYMBAL  "
};

const char* menuItems[] = {
    "LIVE PAD",
    "SEQUENCER",
    "SETTINGS",
    "DIAGNOSTICS"
};
const int menuItemCount = 4;

// Colores por instrumento (BD=KICK, SD=SNARE, CH=CL-HAT, OH=OP-HAT, CL=CLAP, T1=TOM-LO, T2=TOM-HI, CY=CYMBAL)
const uint16_t instrumentColors[MAX_TRACKS] = {
    COLOR_INST_KICK,   // BD (Bass Drum = KICK)
    COLOR_INST_SNARE,  // SD (Snare Drum = SNARE)
    COLOR_INST_CLHAT,  // CH (Closed Hi-Hat = CL-HAT)
    COLOR_INST_OPHAT,  // OH (Open Hi-Hat = OP-HAT)
    COLOR_INST_CLAP,   // CL (Clap = CLAP)
    COLOR_INST_TOMLO,  // T1 (Tom Low = TOM-LO)
    COLOR_INST_TOMHI,  // T2 (Tom High = TOM-HI)
    COLOR_INST_CYMBAL  // CY (Cymbal = CYMBAL)
};

// ============================================
// FUNCTION PROTOTYPES
// ============================================
void setupKits();
void setupPatterns();
void setupSDCard();
void calculateStepInterval();
void updateSequencer();
void handleButtons();
void handleEncoder();
void handleBackButton();
void handlePlayStopButton();
void handleMuteButton();
void handleVolume();
void debugAnalogButtons();
void triggerDrum(int track);
void testButtonsOnBoot();
void setupDFPlayer();
void testAllSamples();
void playSample(int sampleNum);
void stopAllSamples();
void toggleSampleLoop(int sampleNum);
void changeSamplerMode(SamplerMode mode);
void handleSamplerFunction(SamplerFunction func);
void drawSamplerUI();
void drawBootScreen();
void drawConsoleBootScreen();
void drawSpectrumAnimation();
void drawMainMenu();
void drawMenuItems(int oldSelection, int newSelection);
void drawLiveScreen();
void drawSequencerScreen();
void drawSettingsScreen();
void drawDiagnosticsScreen();
void drawHeader();
void drawVUMeters();
void updateTM1638Displays();
void updateStepLEDs();
void updateStepLEDsForTrack(int track);
void updateLEDFeedback();
void updateAudioVisualization();
void changeTempo(int delta);
void changePattern(int delta);
void changeKit(int delta);
void changeTheme(int delta);
void toggleStep(int track, int step);
void changeScreen(Screen newScreen);
void showInstrumentOnTM1638(int track);
void showBPMOnTM1638();
void showVolumeOnTM1638();
void setLED(int ledIndex, bool state);
void setAllLEDs(uint16_t pattern);
uint16_t readAllButtons();

// ============================================
// TM1638 HELPERS
// ============================================
void setLED(int ledIndex, bool state) {
    if (ledIndex < 8) {
        tm1.setLED(ledIndex, state ? 1 : 0);
    } else {
        tm2.setLED(ledIndex - 8, state ? 1 : 0);
    }
}

void setAllLEDs(uint16_t pattern) {
    uint8_t low = pattern & 0xFF;
    uint8_t high = (pattern >> 8) & 0xFF;
    tm1.setLEDs(low);
    tm2.setLEDs(high);
}

uint16_t readAllButtons() {
    uint8_t btn1 = tm1.readButtons();
    uint8_t btn2 = tm2.readButtons();
    return btn1 | (btn2 << 8);
}

uint16_t getInstrumentColor(int track) {
    if (track >= 0 && track < MAX_TRACKS) {
        return instrumentColors[track];
    }
    return COLOR_ACCENT;
}

// ============================================
// SETUP FUNCTIONS
// ============================================
void setupKits() {
    for (int i = 0; i < MAX_KITS; i++) {
        kits[i].name = String(kitNames[i]);
        kits[i].folder = i + 1;
    }
}

void setupPatterns() {
    for (int p = 0; p < MAX_PATTERNS; p++) {
        patterns[p].name = "PTN-" + String(p + 1);
        for (int t = 0; t < MAX_TRACKS; t++) {
            patterns[p].muted[t] = false;
            for (int s = 0; s < MAX_STEPS; s++) {
                patterns[p].steps[t][s] = false;
            }
        }
    }
    
    // Pattern 1: HOUSE
    patterns[0].name = "HOUSE";
    for (int i = 0; i < MAX_STEPS; i += 4) patterns[0].steps[0][i] = true;
    patterns[0].steps[1][4] = true;
    patterns[0].steps[1][12] = true;
    for (int i = 0; i < MAX_STEPS; i += 2) patterns[0].steps[2][i] = true;
    
    // Pattern 2: TECHNO
    patterns[1].name = "TECHNO";
    for (int i = 0; i < MAX_STEPS; i += 4) patterns[1].steps[0][i] = true;
    for (int i = 0; i < MAX_STEPS; i++) patterns[1].steps[2][i] = true;
    patterns[1].steps[1][8] = true;
    
    // Pattern 3: BREAKBEAT
    patterns[2].name = "BREAK";
    patterns[2].steps[0][0] = true;
    patterns[2].steps[0][6] = true;
    patterns[2].steps[0][10] = true;
    patterns[2].steps[1][4] = true;
    patterns[2].steps[1][12] = true;
    for (int i = 0; i < MAX_STEPS; i += 2) patterns[2].steps[2][i] = true;
    patterns[2].steps[3][2] = true;
    patterns[2].steps[3][14] = true;
}

void calculateStepInterval() {
    stepInterval = (60000 / tempo) / 4;
}
/*
void setupSDCard() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(120, 130);
    tft.println("Checking SD Card...");
    
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  SD CARD INITIALIZATION                    ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println("Pin Configuration:");
    Serial.printf("  CS   = Pin %d\n", SD_CS);
    Serial.printf("  MOSI = Pin %d\n", SD_MOSI);
    Serial.printf("  MISO = Pin %d\n", SD_MISO);
    Serial.printf("  SCK  = Pin %d\n", SD_SCK);
    
    // Asegurar que TFT_CS está en HIGH para no interferir
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    
    // Configurar CS como OUTPUT y ponerlo en HIGH antes de inicializar
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(100);
    
    // Test de conectividad MISO
    Serial.println("\nTesting MISO line...");
    pinMode(SD_MISO, INPUT_PULLUP);
    int misoState = digitalRead(SD_MISO);
    Serial.printf("  MISO state: %s\n", misoState ? "HIGH (OK)" : "LOW (check connection)");
    
    // Test de conectividad CS
    Serial.println("\nTesting CS line...");
    digitalWrite(SD_CS, LOW);
    delay(10);
    digitalWrite(SD_CS, HIGH);
    Serial.println("  CS toggle: OK");
    
    // Enviar 80 pulsos de reloj con CS alto para resetear la SD (protocolo SPI SD)
    Serial.println("\nSending SD reset sequence...");
    digitalWrite(SD_CS, HIGH);
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 10; i++) {
        SPI.transfer(0xFF);  // 10 bytes = 80 bits de reloj
    }
    SPI.endTransaction();
    delay(100);
    Serial.println("  Reset sequence sent");
    
    // Intentar varias configuraciones comunes
    bool sdOk = false;
    
    // Intento 1: Frecuencia muy baja (250kHz - modo ultra seguro)
    Serial.println("\n[1] Trying 250kHz (ultra safe mode)...");
    digitalWrite(TFT_CS, HIGH);  // TFT desactivado
    
    if (SD.begin(SD_CS, SPI, 250000)) {
        sdOk = true;
        Serial.println("  ✓ SUCCESS with 250kHz");
    } else {
        Serial.println("  ✗ Failed");
    }
    
    // Intento 2: 1MHz
    if (!sdOk) {
        Serial.println("[2] Trying 1MHz...");
        SD.end();
        delay(100);
        if (SD.begin(SD_CS, SPI, 1000000)) {
            sdOk = true;
            Serial.println("  ✓ SUCCESS with 1MHz");
        } else {
            Serial.println("  ✗ Failed");
        }
    }
    
    // Intento 3: 4MHz
    if (!sdOk) {
        Serial.println("[3] Trying 4MHz...");
        SD.end();
        delay(100);
        if (SD.begin(SD_CS, SPI, 4000000)) {
            sdOk = true;
            Serial.println("  ✓ SUCCESS with 4MHz");
        } else {
            Serial.println("  ✗ Failed");
        }
    }
    
    // Intento 4: Sin especificar frecuencia
    if (!sdOk) {
        Serial.println("[4] Trying default frequency...");
        SD.end();
        delay(100);
        if (SD.begin(SD_CS)) {
            sdOk = true;
            Serial.println("  ✓ SUCCESS with default");
        } else {
            Serial.println("  ✗ Failed");
        }
    }
    
    // Intento 5: Reiniciar SPI manualmente
    if (!sdOk) {
        Serial.println("[5] Trying manual SPI restart...");
        SPI.end();
        delay(100);
        SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
        delay(100);
        if (SD.begin(SD_CS, SPI, 400000)) {
            sdOk = true;
            Serial.println("  ✓ SUCCESS with manual SPI");
        } else {
            Serial.println("  ✗ Failed");
        }
    }
    
    if (sdOk) {
        diagnostic.sdCardOk = true;
        
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("  ✓ SD Card: %lluMB\n", cardSize);
        
        int totalFiles = 0;
        for (int kit = 1; kit <= 3; kit++) {
            char path[10];
            snprintf(path, 10, "/%02d", kit);
            File dir = SD.open(path);
            if (dir && dir.isDirectory()) {
                File file = dir.openNextFile();
                while (file) {
                    if (!file.isDirectory()) {
                        totalFiles++;
                    }
                    file = dir.openNextFile();
                }
                dir.close();
            }
        }
        diagnostic.filesFound = totalFiles;
        Serial.printf("  ✓ Sound files found: %d\n", totalFiles);
        
        tft.setTextColor(COLOR_SUCCESS);
        tft.setCursor(120, 160);
        tft.printf("SD: %lluMB - %d files", cardSize, totalFiles);
        
    } else {
        diagnostic.sdCardOk = false;
        diagnostic.lastError = "SD Card failed";
        Serial.println("\n✗✗✗ ALL METHODS FAILED ✗✗✗");
        Serial.println("\nPossible issues:");
        Serial.println("  1. SD card not inserted or not making contact");
        Serial.println("  2. SD card corrupted or wrong format (use FAT32)");
        Serial.println("  3. Wiring issue - check all 6 connections");
        Serial.println("  4. Power issue - SD needs stable 3.3V");
        Serial.println("  5. Wrong SD module type");
        Serial.println("\nContinuing without SD card...");
        
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(120, 160);
        tft.println("SD Card not found");
    }
    
    delay(1000);
}
/*
void setupSDCard() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(120, 130);
    tft.println("Checking SD Card...");
    
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  SD CARD INITIALIZATION                    ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println("Pin Configuration:");
    Serial.printf("  CS   = Pin %d\n", SD_CS);
    Serial.printf("  MOSI = Pin %d\n", SD_MOSI);
    Serial.printf("  MISO = Pin %d\n", SD_MISO);
    Serial.printf("  SCK  = Pin %d\n", SD_SCK);
    
    // CRÍTICO: Desactivar TODOS los dispositivos SPI antes de inicializar SD
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    
    delay(100);
    
    // Secuencia de reset de la SD card (muy importante)
    Serial.println("\nSending SD reset sequence...");
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1); // Inicializar SPI explícitamente
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    digitalWrite(SD_CS, HIGH);
    for (int i = 0; i < 10; i++) {
        SPI.transfer(0xFF);
    }
    SPI.endTransaction();
    delay(100);
    
    // MÉTODO CORRECTO: Pasar SPI como segundo parámetro
    bool sdOk = false;
    
    // Intento 1: Con SPI explícito y frecuencia muy baja (400kHz)
    Serial.println("\n[1] Trying 400kHz with explicit SPI...");
    digitalWrite(TFT_CS, HIGH); // TFT desactivado
    digitalWrite(SD_CS, HIGH);  // SD desactivado
    delay(10);
    
    // IMPORTANTE: SD.begin(CS_PIN, SPI_OBJECT, FREQUENCY, MOUNT_POINT, MAX_FILES)
    if (SD.begin(SD_CS, SPI, 400000, "/sd", 5)) {
        sdOk = true;
        Serial.println("  ✓ SUCCESS with 400kHz");
    }
    
    if (!sdOk) {
        Serial.println("[2] Trying 250kHz...");
        SD.end();
        delay(200);
        if (SD.begin(SD_CS, SPI, 250000, "/sd", 5)) {
            sdOk = true;
            Serial.println("  ✓ SUCCESS with 250kHz");
        }
    }
    
    if (!sdOk) {
        Serial.println("[3] Trying with SD_MMC mode (alternative)...");
        // Nota: Esto solo funciona si tu módulo soporta modo MMC
    }
    
    if (sdOk) {
        diagnostic.sdCardOk = true;
        
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        uint8_t cardType = SD.cardType();
        
        Serial.printf("  ✓ SD Card Type: ");
        switch(cardType) {
            case CARD_MMC: Serial.println("MMC"); break;
            case CARD_SD: Serial.println("SDSC"); break;
            case CARD_SDHC: Serial.println("SDHC"); break;
            default: Serial.println("UNKNOWN"); break;
        }
        
        Serial.printf("  ✓ SD Card Size: %lluMB\n", cardSize);
        
        // Contar archivos
        int totalFiles = 0;
        for (int kit = 1; kit <= 3; kit++) {
            char path[10];
            snprintf(path, 10, "/sd/%02d", kit); // IMPORTANTE: Agregar /sd/ al path
            File dir = SD.open(path);
            if (dir && dir.isDirectory()) {
                File file = dir.openNextFile();
                while (file) {
                    if (!file.isDirectory()) {
                        totalFiles++;
                        Serial.printf("    Found: %s\n", file.name());
                    }
                    file = dir.openNextFile();
                }
                dir.close();
            } else {
                Serial.printf("  ! Folder %s not found\n", path);
            }
        }
        
        diagnostic.filesFound = totalFiles;
        Serial.printf("  ✓ Total sound files: %d\n", totalFiles);
        
        // Mostrar en pantalla
        digitalWrite(TFT_CS, LOW); // Reactivar TFT
        delay(10);
        tft.setTextColor(COLOR_SUCCESS);
        tft.setCursor(120, 160);
        tft.printf("SD: %lluMB - %d files", cardSize, totalFiles);
        digitalWrite(TFT_CS, HIGH);
        
    } else {
        diagnostic.sdCardOk = false;
        diagnostic.lastError = "SD Card failed";
        
        Serial.println("\n✗✗✗ ALL METHODS FAILED ✗✗✗");
        Serial.println("\nDIAGNOSTIC CHECKLIST:");
        Serial.println("  [1] SD card inserted correctly?");
        Serial.println("  [2] SD formatted as FAT32?");
        Serial.println("  [3] Check wiring:");
        Serial.println("      - CS  -> Pin 15");
        Serial.println("      - SCK -> Pin 18 (shared)");
        Serial.println("      - MOSI-> Pin 23 (shared)");
        Serial.println("      - MISO-> Pin 19 (shared)");
        Serial.println("      - VCC -> 5V");
        Serial.println("      - GND -> GND");
        Serial.println("  [4] Try different SD card (some cards are incompatible)");
        Serial.println("  [5] Check SD module has voltage regulator (3.3V/5V)");
        
        digitalWrite(TFT_CS, LOW);
        delay(10);
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(120, 160);
        tft.println("SD Card ERROR!");
        digitalWrite(TFT_CS, HIGH);
    }
    
    delay(2000);
}  */
/*
void setupSDCard() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(120, 130);
    tft.println("Checking SD Card...");
    
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  SD CARD INITIALIZATION                    ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println("Pin Configuration:");
    Serial.printf("  CS   = Pin %d (GPIO 14)\n", SD_CS);
    Serial.printf("  MOSI = Pin %d (shared)\n", SD_MOSI);
    Serial.printf("  MISO = Pin %d (shared)\n", SD_MISO);
    Serial.printf("  SCK  = Pin %d (shared)\n", SD_SCK);
    
    // CRÍTICO: Todos los CS en HIGH antes de inicializar
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    
    delay(100);
    
    Serial.println("\nInitializing SD card...");
    
    // MÉTODO SIMPLE (funciona en ESP32)
    if (!SD.begin(SD_CS)) {
        Serial.println("✗ Card Mount Failed");
        diagnostic.sdCardOk = false;
        diagnostic.lastError = "SD Mount Failed";
        
        tft.setTextColor(COLOR_ERROR);
        tft.setCursor(100, 160);
        tft.println("SD MOUNT FAILED!");
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(80, 185);
        tft.println("Check: Card inserted? Formatted FAT32?");
        tft.setCursor(80, 200);
        tft.printf("CS connected to GPIO %d?", SD_CS);
        
        delay(3000);
        return;
    }
    
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
        Serial.println("✗ No SD card attached");
        diagnostic.sdCardOk = false;
        diagnostic.lastError = "No SD card";
        
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(120, 160);
        tft.println("NO SD CARD!");
        delay(2000);
        return;
    }
    
    // ✓ SUCCESS!
    diagnostic.sdCardOk = true;
    
    Serial.print("✓ SD Card Type: ");
    switch(cardType) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("✓ SD Card Size: %lluMB\n", cardSize);
    
    // Contar archivos
    int totalFiles = 0;
    Serial.println("\nScanning folders:");
    
    for (int kit = 1; kit <= 3; kit++) {
        char path[10];
        snprintf(path, 10, "/%02d", kit);
        
        File dir = SD.open(path);
        if (!dir) {
            Serial.printf("  Creating folder %s\n", path);
            SD.mkdir(path);
            continue;
        }
        
        if (dir.isDirectory()) {
            Serial.printf("  %s/: ", path);
            int count = 0;
            File file = dir.openNextFile();
            while (file) {
                if (!file.isDirectory()) {
                    count++;
                    totalFiles++;
                }
                file.close();
                file = dir.openNextFile();
            }
            Serial.printf("%d files\n", count);
        }
        dir.close();
    }
    
    diagnostic.filesFound = totalFiles;
    Serial.printf("\n✓ Total: %d sound files\n", totalFiles);
    
    // Mostrar en pantalla
    tft.setTextColor(COLOR_SUCCESS);
    tft.setCursor(110, 160);
    tft.printf("SD: %lluMB", cardSize);
    tft.setCursor(110, 185);
    tft.setTextColor(COLOR_ACCENT2);
    tft.printf("%d sound files ready", totalFiles);
    
    delay(1500);
}

void testSDWiring() {
    Serial.println("\n═══ SD WIRING TEST ═══");
    
    pinMode(SD_CS, OUTPUT);
    pinMode(SD_MISO, INPUT);
    
    // Test 1: CS toggle
    Serial.print("CS toggle test: ");
    digitalWrite(SD_CS, HIGH);
    delay(10);
    digitalWrite(SD_CS, LOW);
    delay(10);
    digitalWrite(SD_CS, HIGH);
    Serial.println("OK");
    
    // Test 2: MISO pullup
    Serial.print("MISO state (should be HIGH): ");
    pinMode(SD_MISO, INPUT_PULLUP);
    delay(10);
    int misoState = digitalRead(SD_MISO);
    Serial.println(misoState ? "OK (HIGH)" : "FAIL (LOW - check wiring!)");
    
    // Test 3: SPI communication test
    Serial.print("SPI communication test: ");
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    digitalWrite(SD_CS, LOW);
    uint8_t response = SPI.transfer(0xFF);
    digitalWrite(SD_CS, HIGH);
    SPI.endTransaction();
    Serial.printf("Response: 0x%02X %s\n", response, 
                  (response != 0xFF && response != 0x00) ? "(OK)" : "(Check connection)");
}

void testButtonsOnBoot() {
    Serial.println("\nWaiting for button presses (5 seconds)...");
    Serial.println("Press S1-S8 on TM1638 #1, S9-S16 on TM1638 #2");
    Serial.println("─────────────────────────────────────");
    
    unsigned long testStart = millis();
    int buttonPressCount = 0;
    
    while (millis() - testStart < 5000) {
        uint16_t buttons = readAllButtons();
        
        if (buttons != 0) {
            Serial.printf("► BUTTONS: 0x%04X (", buttons);
            for (int i = 15; i >= 0; i--) {
                Serial.print((buttons >> i) & 1);
                if (i == 8) Serial.print(" ");
            }
            Serial.println(")");
            
            for (int i = 0; i < 16; i++) {
                if (buttons & (1 << i)) {
                    Serial.printf("  ✓ Button S%d pressed\n", i + 1);
                    setLED(i, true);
                    buttonPressCount++;
                }
            }
            
            delay(200);
            setAllLEDs(0x0000);
            delay(100);
        }
        
        delay(10);
    }
    
    Serial.println("─────────────────────────────────────");
    Serial.printf("Test complete! %d button presses detected\n", buttonPressCount);
    
    if (buttonPressCount == 0) {
        Serial.println("⚠ WARNING: No buttons detected!");
    }
    
    Serial.println("");
}
*/

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n╔════════════════════════════════════╗");
    Serial.println("║   RED808 V5 - DRUM MACHINE         ║");
    Serial.println("║   2x TM1638 + SD Card              ║");
    Serial.println("║   NO AUDIO (Visual Only)           ║");
    Serial.println("╚════════════════════════════════════╝\n");
     Serial.println("► SD Wiring Test...");

     // ⚠️ ESTABLECER TODOS LOS CS EN HIGH PRIMERO
    pinMode(TFT_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    
    delay(100);
    //testSDWiring(); // AGREGAR ESTO
    // TFT Init
    Serial.print("► TFT Init... ");
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(COLOR_BG);
    diagnostic.tftOk = true;
    Serial.println("OK (480x320)");
    
    // TM1638 #1
    Serial.print("► TM1638 #1 Init... ");
    tm1.displayBegin();
    tm1.brightness(7);
    tm1.reset();
    tm1.displayText("STEP1-8 ");
    diagnostic.tm1638_1_Ok = true;
    Serial.println("OK");
    delay(200);
    
    // TM1638 #2
    Serial.print("► TM1638 #2 Init... ");
    tm2.displayBegin();
    tm2.brightness(7);
    tm2.reset();
    tm2.displayText("STE9-16 ");
    diagnostic.tm1638_2_Ok = true;
    Serial.println("OK");
    delay(200);
    
    // Encoder
    Serial.print("► Rotary Encoder Init... ");
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    // 3 Button Panel - Analog Buttons
    pinMode(ANALOG_BUTTONS_PIN, INPUT);
    Serial.println("3 Analog Buttons ready on pin 34 (PLAY/STOP, MUTE, BACK)");
    Serial.println("Module: SMART OPEN ANALOG BUTTONS (GND-VCC-SIG)");
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║  CALIBRATION MODE ACTIVE                          ║");
    Serial.println("║  Press each button and note ADC values in Serial  ║");
    Serial.println("║  Then adjust ranges in lines 38-44 of main.cpp    ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
    
    // Configurar interrupciones para CLK y DT
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), []() {
        uint8_t clk = digitalRead(ENCODER_CLK);
        uint8_t dt = digitalRead(ENCODER_DT);
        encoderState = (encoderState << 2) | (clk << 1) | dt;
        encoderState &= 0x0F;
        int8_t change = encoderStates[encoderState];
        if (change != 0) {
            encoderPos += change;
            encoderChanged = true;
        }
    }, CHANGE);
    
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), []() {
        uint8_t clk = digitalRead(ENCODER_CLK);
        uint8_t dt = digitalRead(ENCODER_DT);
        encoderState = (encoderState << 2) | (clk << 1) | dt;
        encoderState &= 0x0F;
        int8_t change = encoderStates[encoderState];
        if (change != 0) {
            encoderPos += change;
            encoderChanged = true;
        }
    }, CHANGE);
    
    diagnostic.encoderOk = true;
    Serial.println("OK");
    
    // ADC
    Serial.print("► ADC Init... ");
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    Serial.println("OK");
    
    // Boot estilo consola UNIX
    drawConsoleBootScreen();
    
    // SD Card desactivada - no necesaria para DFPlayer
    //Serial.println("► SD Card Init...");
    //setupSDCard();
    
    // Inicializar DFPlayer Mini
    Serial.println("► DFPlayer Mini Init...");
    setupDFPlayer();
    
    setupKits();
    setupPatterns();
    calculateStepInterval();
    
    // Test de samples si DFPlayer OK (OPCIONAL - comentar si no se quiere el test)
    // if (samplerInitialized) {
    //     testAllSamples();
    // }
    
    // Actualizar pantalla de boot con estado de SD
    // (ya se muestra en drawConsoleBootScreen pero se actualiza después de setupSDCard)
    
    // LED test rápido
    for (int i = 0; i < 16; i++) {
        setLED(i, true);
        delay(30);
    }
    delay(100);
    setAllLEDs(0x0000);
    
    // Spectrum Analyzer Animation
    drawSpectrumAnimation();
    
    if (samplerInitialized) {
        tm1.displayText("SAMPLER ");
        tm2.displayText("READY   ");
    } else {
        tm1.displayText("READY   ");
        tm2.displayText("RED808  ");
    }
    
    delay(500);
    
    currentScreen = SCREEN_MENU;
    needsFullRedraw = true;
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║       RED808 V5 READY!             ║");
    Serial.println("╚════════════════════════════════════╝\n");
    
    if (samplerInitialized) {
        Serial.println("╔════════════════════════════════════════════╗");
        Serial.println("║  SAMPLER READY                             ║");
        Serial.println("║  Go to LIVE PAD to play samples            ║");
        Serial.println("║  S1-S8: Play samples 001-008               ║");
        Serial.println("║  S9-S16: Functions (LOOP, STOP, VOL, etc)  ║");
        Serial.println("╚════════════════════════════════════════════╝\n");
    }
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
    static unsigned long lastLoopTime = 0;
    unsigned long currentTime = millis();
    
   // encoder.tick();
    
    if (currentTime - lastLoopTime < 16) {
        return;
    }
    lastLoopTime = currentTime;
    
    handleButtons();
    handleEncoder();
    
    // DEBUG: Mostrar valores ADC de botones analógicos
    debugAnalogButtons();
    
    handlePlayStopButton();
    handleMuteButton();
    handleBackButton();
    
    if (currentTime - lastVolumeRead > 100) {
        handleVolume();
        lastVolumeRead = currentTime;
    }
    
    if (isPlaying && currentScreen == SCREEN_SEQUENCER) {
        updateSequencer();
    }
    
    updateAudioVisualization();
    
    if (needsFullRedraw) {
        switch (currentScreen) {
            case SCREEN_MENU:
                drawMainMenu();
                break;
            case SCREEN_LIVE:
                drawLiveScreen();
                break;
            case SCREEN_SEQUENCER:
                drawSequencerScreen();
                break;
            case SCREEN_SETTINGS:
                drawSettingsScreen();
                break;
            case SCREEN_DIAGNOSTICS:
                drawDiagnosticsScreen();
                break;
            default:
                break;
        }
        
        needsFullRedraw = false;
        needsHeaderUpdate = false;
        needsGridUpdate = false;
        
    } else {
        if (needsHeaderUpdate && currentScreen != SCREEN_MENU) {
            drawHeader();
            needsHeaderUpdate = false;
        }
        
        if (needsGridUpdate && currentScreen == SCREEN_SEQUENCER) {
            drawSequencerScreen();
            needsGridUpdate = false;
        }
    }
    
    updateTM1638Displays();
    updateLEDFeedback();
}

// ============================================
// SEQUENCER
// ============================================
void updateSequencer() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastStepTime >= stepInterval) {
        lastStepTime = currentTime;
        
        Pattern& pattern = patterns[currentPattern];
        for (int track = 0; track < MAX_TRACKS; track++) {
            if (pattern.steps[track][currentStep] && !pattern.muted[track]) {
                triggerDrum(track);
            }
        }
        
        updateStepLEDs();
        
        currentStep = (currentStep + 1) % MAX_STEPS;
        needsGridUpdate = true;
    }
}

// ============================================
// DFPLAYER & SAMPLER FUNCTIONS
// ============================================

// Funciones de comandos directos para DFPlayer
void sendCommandFast(byte cmd, byte param1, byte param2) {
    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2, 0x00, 0x00, 0xEF};
    
    // Calcular checksum
    uint16_t checksum = -(buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]);
    buffer[7] = (checksum >> 8) & 0xFF;
    buffer[8] = checksum & 0xFF;
    
    // Enviar sin esperar - MÁXIMA VELOCIDAD
    dfplayerSerial.write(buffer, 10);
}

void sendCommand(byte cmd, byte param1, byte param2) {
    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2, 0x00, 0x00, 0xEF};
    
    // Calcular checksum
    uint16_t checksum = -(buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]);
    buffer[7] = (checksum >> 8) & 0xFF;
    buffer[8] = checksum & 0xFF;
    
    // Limpiar buffer de entrada para evitar comandos acumulados
    while(dfplayerSerial.available()) {
        dfplayerSerial.read();
    }
    
    // Enviar comando rápido
    dfplayerSerial.write(buffer, 10);
    delay(10);  // Delay óptimo: suficiente para que DFPlayer procese sin saturar
}

void playFromFolder(int folder, int file) {
    int folderFile = (folder << 8) | file;  // Combinar folder y file en un solo valor
    sendCommandFast(0x0F, (folderFile >> 8) & 0xFF, folderFile & 0xFF);  // Sin delay
}

void setVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 30) vol = 30;
    sendCommand(0x06, 0x00, vol);
}

void stopDFPlayer() {
    sendCommand(0x16, 0x00, 0x00);  // Comando STOP
}

void resetDFPlayer() {
    sendCommand(0x0C, 0x00, 0x00);  // Comando RESET
    delay(100);  // Reducido para velocidad
}

void setSDSource() {
    sendCommand(0x09, 0x00, 0x02);  // Comando para seleccionar SD card
    delay(30);
}

void queryTotalFiles() {
    sendCommand(0x48, 0x00, 0x00);  // Comando para obtener total de archivos
}

void queryFilesInFolder(int folder) {
    sendCommand(0x4E, 0x00, folder);  // Comando para archivos en carpeta específica
}

void setupDFPlayer() {
    // Inicializar Serial para DFPlayer
    dfplayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    delay(500);
    
    // Reset del módulo
    resetDFPlayer();
    delay(200);
    
    // Seleccionar fuente SD
    setSDSource();
    delay(100);
    
    // Configurar volumen inicial
    setVolume(25);
    dfplayerVolume = 25;
    delay(50);
    
    diagnostic.dfplayerOk = true;
    samplerInitialized = true;
    
    // Inicializar info de samples
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples[i].number = i + 1;
        samples[i].name = "SAMPLE " + String(i + 1);
        samples[i].isLooping = false;
        samples[i].isPlaying = false;
        samples[i].volume = dfplayerVolume;
        samples[i].mode = SAMPLER_ONESHOT;
    }
}

void playSample(int sampleNum) {
    if (!samplerInitialized || sampleNum < 0 || sampleNum >= MAX_SAMPLES) {
        return;
    }
    
    int fileNum = sampleNum + 1;  // 0->1, 1->2, etc.
    
    // Reproducir desde carpeta /01/ - SIN DELAY
    playFromFolder(1, fileNum);
    
    // Actualizar estado
    samples[sampleNum].isPlaying = true;
    lastPlayedSample = sampleNum;
    samplePlayTime = millis();
    
    // Actualizar LED
    setLED(sampleNum, true);
    ledActive[sampleNum] = true;
    ledOffTime[sampleNum] = millis() + 300;
    
    // Mostrar en TM1638
    char display[9];
    snprintf(display, 9, "SMP %d   ", fileNum);
    tm1.displayText(display);
    snprintf(display, 9, "%s", samples[sampleNum].mode == SAMPLER_LOOP ? "LOOP    " : "ONESHOT ");
    tm2.displayText(display);
}

void stopAllSamples() {
    if (!samplerInitialized) return;
    
    // NO enviamos STOP - el módulo queda siempre activo para máxima velocidad
    // Los samples se interrumpen automáticamente al reproducir el siguiente
    
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples[i].isPlaying = false;
    }
    
    setAllLEDs(0x0000);
    
    tm1.displayText("STOPPED ");
    tm2.displayText("        ");
}

void toggleSampleLoop(int sampleNum) {
    if (!samplerInitialized || sampleNum < 0 || sampleNum >= MAX_SAMPLES) {
        return;
    }
    
    samples[sampleNum].isLooping = !samples[sampleNum].isLooping;
    
    // Si está en loop, reproducir el sample en modo loop
    if (samples[sampleNum].isLooping) {
        playFromFolder(1, sampleNum + 1);
    }
}

void changeSamplerMode(SamplerMode mode) {
    globalSamplerMode = mode;
    
    const char* modeNames[] = {"ONESHOT", "LOOP", "HOLD", "REVERSE"};
    char display[9];
    snprintf(display, 9, "%-8s", modeNames[mode]);
    tm2.displayText(display);
}

void handleSamplerFunction(SamplerFunction func) {
    if (!samplerInitialized) return;
    
    switch(func) {
        case FUNC_LOOP:
            changeSamplerMode(SAMPLER_LOOP);
            break;
            
        case FUNC_HOLD:
            changeSamplerMode(SAMPLER_HOLD);
            break;
            
        case FUNC_STOP:
            stopAllSamples();
            break;
            
        case FUNC_VOLUME_UP:
            dfplayerVolume = constrain(dfplayerVolume + 2, 0, 30);
            setVolume(dfplayerVolume);
            break;
            
        case FUNC_VOLUME_DN:
            dfplayerVolume = constrain(dfplayerVolume - 2, 0, 30);
            setVolume(dfplayerVolume);
            break;
            
        case FUNC_PREV:
            currentSample = (currentSample - 1 + MAX_SAMPLES) % MAX_SAMPLES;
            playSample(currentSample);
            break;
            
        case FUNC_NEXT:
            currentSample = (currentSample + 1) % MAX_SAMPLES;
            playSample(currentSample);
            break;
            
        case FUNC_CLEAR:
            stopAllSamples();
            globalSamplerMode = SAMPLER_ONESHOT;
            break;
            
        default:
            break;
    }
}

void testAllSamples() {
    if (!samplerInitialized) {
        Serial.println("⚠ DFPlayer not initialized - skipping sample test");
        return;
    }
    
    Serial.println("\\n╔════════════════════════════════════════════╗");
    Serial.println("║  TESTING ALL SAMPLES (8)                   ║");
    Serial.println("╚════════════════════════════════════════════╝");
    
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(3);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(120, 80);
    tft.println("SAMPLE TEST");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(80, 130);
    tft.println("Playing 8 samples...");
    
    for (int i = 0; i < MAX_SAMPLES; i++) {
        Serial.printf("Testing sample %d/8...\\n", i + 1);
        
        // Mostrar en pantalla
        tft.fillRect(80, 170, 320, 40, COLOR_BG);
        tft.setTextSize(4);
        tft.setTextColor(COLOR_ACCENT);
        tft.setCursor(180, 170);
        tft.printf("SAMPLE %d", i + 1);
        
        // Reproducir
        playSample(i);
        
        // Animar LEDs
        setLED(i, true);
        
        delay(800);  // Esperar entre samples
        
        setLED(i, false);
    }
    
    stopAllSamples();
    
    Serial.println("Sample test complete!\\n");
    delay(500);
}

void triggerDrum(int track) {
    // Si el DFPlayer está disponible, reproducir sample correspondiente
    if (samplerInitialized && track < MAX_SAMPLES) {
        playSample(track);
    }
    
    audioLevels[track] = 100;
    showInstrumentOnTM1638(track);
}

void updateStepLEDs() {
    if (isPlaying) {
        // Durante reproducción: solo mostrar step actual
        setAllLEDs(0x0000);
        setLED(currentStep, true);
        
        Pattern& pattern = patterns[currentPattern];
        for (int track = 0; track < MAX_TRACKS; track++) {
            if (pattern.steps[track][currentStep] && !pattern.muted[track]) {
                ledActive[currentStep] = true;
                ledOffTime[currentStep] = millis() + 100;
            }
        }
    }
}

// Nueva función: Actualizar LEDs para mostrar steps del track seleccionado
void updateStepLEDsForTrack(int track) {
    Pattern& pattern = patterns[currentPattern];
    uint16_t ledPattern = 0;
    
    // Construir patrón de LEDs basado en los steps del track
    for (int i = 0; i < MAX_STEPS; i++) {
        if (pattern.steps[track][i]) {
            ledPattern |= (1 << i);
        }
    }
    
    setAllLEDs(ledPattern);
}

// ============================================
// INPUT HANDLING
// ============================================
void handleButtons() {
    uint16_t buttons = readAllButtons();
    unsigned long currentTime = millis();
    
    // Detectar nuevas presiones (flanco de subida)
    uint16_t newPress = buttons & ~lastButtonState;
    
    // Procesar nuevas presiones
    for (int i = 0; i < 16; i++) {
        if (newPress & (1 << i)) {
            buttonPressTime[i] = currentTime;
            lastRepeatTime[i] = currentTime;
            
            if (currentScreen == SCREEN_LIVE) {
                if (i < 8) {
                    // S1-S8: Reproducir samples 1-8
                    if (samplerInitialized) {
                        playSample(i);
                        currentSample = i;
                    } else {
                        triggerDrum(i);
                        setLED(i, true);
                        ledActive[i] = true;
                        ledOffTime[i] = currentTime + 150;
                    }
                } else {
                    // S9-S16: Funciones del sampler
                    SamplerFunction func = (SamplerFunction)i;
                    handleSamplerFunction(func);
                    setLED(i, true);
                    ledActive[i] = true;
                    ledOffTime[i] = currentTime + 200;
                }
                
                if (needsFullRedraw) {
                    drawLiveScreen();
                }
            } else if (currentScreen == SCREEN_SETTINGS) {
                if (i >= 0 && i < MAX_KITS) {
                    changeKit(i - currentKit);
                    drawSettingsScreen();
                } else if (i >= 4 && i < 4 + THEME_COUNT) {
                    int newTheme = i - 4;
                    changeTheme(newTheme - currentTheme);
                    drawSettingsScreen();
                }
            } else if (currentScreen == SCREEN_SEQUENCER && !isPlaying) {
                toggleStep(selectedTrack, i);
                updateStepLEDsForTrack(selectedTrack);
                bool wasFullRedraw = needsFullRedraw;
                needsFullRedraw = true;
                drawSequencerScreen();
                needsFullRedraw = wasFullRedraw;
            }
        }
    }
    
    // Detectar botones mantenidos y repetir samples (solo en LIVE)
    if (currentScreen == SCREEN_LIVE && samplerInitialized) {
        for (int i = 0; i < 8; i++) {
            if (buttons & (1 << i)) {
                unsigned long pressedDuration = currentTime - buttonPressTime[i];
                
                if (pressedDuration > HOLD_THRESHOLD && 
                    (currentTime - lastRepeatTime[i]) > REPEAT_INTERVAL) {
                    
                    playSample(i);
                    lastRepeatTime[i] = currentTime;
                    
                    setLED(i, true);
                    ledActive[i] = true;
                    ledOffTime[i] = currentTime + 300;
                }
            }
        }
    }
    
    lastButtonState = buttons;
}

void handleEncoder() {
    // Manejar rotación del encoder
    if (encoderChanged) {
        encoderChanged = false;
        int currentPos = encoderPos;
        int rawDelta = -(currentPos - lastEncoderPos); // Invertido, sin dividir
        
        if (rawDelta != 0) {
            lastEncoderPos = currentPos;
            
            if (currentScreen == SCREEN_MENU) {
                // En menú: sensibilidad media (dividir por 2)
                int delta = rawDelta / 2;
                if (delta != 0) {
                    int oldSelection = menuSelection;
                    menuSelection += delta;
                    if (menuSelection < 0) menuSelection = menuItemCount - 1;
                    if (menuSelection >= menuItemCount) menuSelection = 0;
                    
                    // Solo redibujar items que cambiaron
                    if (oldSelection != menuSelection) {
                        drawMenuItems(oldSelection, menuSelection);
                    }
                    Serial.printf("► Menu: %d\n", menuSelection);
                }
                
            } else if (currentScreen == SCREEN_SETTINGS) {
                // En settings: ajustar BPM (tempo)
                int delta = rawDelta / 3;
                if (delta != 0) {
                    changeTempo(delta * 5);  // Cambios de 5 en 5 BPM
                    needsHeaderUpdate = true;
                    showBPMOnTM1638();
                    Serial.printf("► BPM: %d\n", tempo);
                }
                
            } else if (currentScreen == SCREEN_SEQUENCER) {
                // En sequencer: más sensible (dividir por 2 para precisión perfecta)
                int delta = rawDelta / 2;
                if (delta != 0) {
                    selectedTrack += delta;
                    if (selectedTrack < 0) selectedTrack = MAX_TRACKS - 1;
                    if (selectedTrack >= MAX_TRACKS) selectedTrack = 0;
                    
                    // Mostrar instrumento en TM1638
                    showInstrumentOnTM1638(selectedTrack);
                    
                    // Actualizar LEDs para mostrar steps del track seleccionado
                    if (!isPlaying) {
                        updateStepLEDsForTrack(selectedTrack);
                    }
                    
                    needsHeaderUpdate = true;
                    Serial.printf("► Track: %d (%s)\n", selectedTrack, trackNames[selectedTrack]);
                }
            }
        }
    }
    
    // Manejar botón del encoder (solo click corto)
    static bool lastBtn = HIGH;
    static unsigned long lastBtnTime = 0;
    bool btn = digitalRead(ENCODER_SW);
    unsigned long currentTime = millis();
    
    // Detectar flanco de subida (cuando se suelta el botón) con debounce
    if (btn == HIGH && lastBtn == LOW && (currentTime - lastBtnTime > 50)) {
        lastBtnTime = currentTime;
        Serial.println("► ENCODER BTN (ENTER)");
        
        if (currentScreen == SCREEN_MENU) {
            // En menú: Enter selecciona opción
            switch (menuSelection) {
                case 0: changeScreen(SCREEN_LIVE); break;
                case 1: changeScreen(SCREEN_SEQUENCER); break;
                case 2: changeScreen(SCREEN_SETTINGS); break;
                case 3: changeScreen(SCREEN_DIAGNOSTICS); break;
            }
            
        } else if (currentScreen == SCREEN_SEQUENCER) {
            // En sequencer: Play/Stop
            isPlaying = !isPlaying;
            if (!isPlaying) {
                currentStep = 0;
                setAllLEDs(0x0000);
                updateStepLEDsForTrack(selectedTrack);
            }
            needsFullRedraw = true;
        }
    }
    lastBtn = btn;
}

void handlePlayStopButton() {
    static bool lastPlayStopPressed = false;
    static unsigned long lastPlayStopBtnTime = 0;
    unsigned long currentTime = millis();
    
    int adcValue = analogRead(ANALOG_BUTTONS_PIN);
    bool playStopPressed = (adcValue >= BTN_PLAY_STOP_MIN && adcValue <= BTN_PLAY_STOP_MAX);
    
    // Detectar liberación del botón con debounce
    if (!playStopPressed && lastPlayStopPressed && (currentTime - lastPlayStopBtnTime > 50)) {
        lastPlayStopBtnTime = currentTime;
        Serial.printf("► PLAY/STOP BUTTON (ADC: %d)\n", adcValue);
        
        // Solo funciona en SEQUENCER
        if (currentScreen == SCREEN_SEQUENCER) {
            isPlaying = !isPlaying;
            if (!isPlaying) {
                currentStep = 0;
                setAllLEDs(0x0000);
                updateStepLEDsForTrack(selectedTrack);
            }
            Serial.printf("   Sequencer: %s\n", isPlaying ? "PLAYING" : "STOPPED");
            needsFullRedraw = true;
        }
    }
    lastPlayStopPressed = playStopPressed;
}

void handleMuteButton() {
    static bool lastMutePressed = false;
    static unsigned long muteBtnPressTime = 0;
    static bool holdProcessed = false;
    static int lastMuteAdcValue = 0;
    unsigned long currentTime = millis();
    
    int adcValue = analogRead(ANALOG_BUTTONS_PIN);
    bool mutePressed = (adcValue >= BTN_MUTE_MIN && adcValue <= BTN_MUTE_MAX);
    bool noButtonPressed = (adcValue < BTN_NONE_THRESHOLD);
    
    // Detectar presión del botón (flanco de subida)
    if (mutePressed && !lastMutePressed) {
        muteBtnPressTime = currentTime;
        holdProcessed = false;
        lastMuteAdcValue = adcValue;
        Serial.printf("► MUTE BUTTON PRESSED (ADC: %d)\n", adcValue);
    }
    
    // Detectar HOLD (mantener presionado >1 segundo) - CLEAR
    if (mutePressed && !holdProcessed && (currentTime - muteBtnPressTime > 1000)) {
        holdProcessed = true;
        Serial.printf("► MUTE BUTTON (HOLD - CLEAR INSTRUMENT) ADC: %d\n", adcValue);
        
        // Solo funciona en SEQUENCER
        if (currentScreen == SCREEN_SEQUENCER) {
            // Limpiar todos los steps del instrumento seleccionado
            Pattern& pattern = patterns[currentPattern];
            for (int s = 0; s < MAX_STEPS; s++) {
                pattern.steps[selectedTrack][s] = false;
            }
            Serial.printf("   ✓ Cleared all steps for Track %d (%s)\n", 
                         selectedTrack, trackNames[selectedTrack]);
            
            // Actualizar display
            updateStepLEDsForTrack(selectedTrack);
            needsFullRedraw = true;
            
            // Feedback visual
            tm1.displayText("CLEARED ");
            tm2.displayText(instrumentNames[selectedTrack]);
        }
    }
    
    // Detectar liberación del botón - TOGGLE MUTE (solo si fue click corto)
    if (noButtonPressed && lastMutePressed && !holdProcessed) {
        unsigned long pressDuration = currentTime - muteBtnPressTime;
        
        // Solo toggle mute si la presión fue corta (<1000ms) y mayor que debounce
        if (pressDuration > 50 && pressDuration < 1000) {
            Serial.printf("► MUTE BUTTON (CLICK - TOGGLE MUTE) Duration: %lums\n", pressDuration);
            
            // Solo funciona en SEQUENCER
            if (currentScreen == SCREEN_SEQUENCER) {
                // Toggle mute del track seleccionado
                Pattern& pattern = patterns[currentPattern];
                pattern.muted[selectedTrack] = !pattern.muted[selectedTrack];
                
                Serial.printf("   ✓ Track %d (%s): %s\n", 
                             selectedTrack, trackNames[selectedTrack],
                             pattern.muted[selectedTrack] ? "MUTED" : "UNMUTED");
                
                // Feedback visual
                if (pattern.muted[selectedTrack]) {
                    tm1.displayText("MUTED  ");
                } else {
                    tm1.displayText("UNMUTED");
                }
                tm2.displayText(instrumentNames[selectedTrack]);
                
                needsFullRedraw = true;
            }
        }
    }
    
    lastMutePressed = mutePressed;
}

void handleBackButton() {
    static bool lastBackPressed = false;
    static unsigned long lastBackBtnTime = 0;
    unsigned long currentTime = millis();
    
    int adcValue = analogRead(ANALOG_BUTTONS_PIN);
    bool backPressed = (adcValue >= BTN_BACK_MIN && adcValue <= BTN_BACK_MAX);
    
    // Detectar liberación del botón con debounce
    if (!backPressed && lastBackPressed && (currentTime - lastBackBtnTime > 50)) {
        lastBackBtnTime = currentTime;
        Serial.printf("► BACK BUTTON (ADC: %d)\n", adcValue);
        
        // Desde cualquier pantalla excepto menú, volver al menú
        if (currentScreen != SCREEN_MENU) {
            changeScreen(SCREEN_MENU);
        }
    }
    lastBackPressed = backPressed;
}

void debugAnalogButtons() {
    unsigned long currentTime = millis();
    
    // Mostrar valor ADC cada 200ms
    if (currentTime - lastButtonDebug > 200) {
        lastButtonDebug = currentTime;
        
        int adcValue = analogRead(ANALOG_BUTTONS_PIN);
        
        // Solo mostrar si el valor cambió significativamente
        if (abs(adcValue - lastAdcValue) > 50) {
            lastAdcValue = adcValue;
            
            Serial.printf("\n═══════════════════════════════════\n");
            Serial.printf("ADC Value: %4d\n", adcValue);
            
            // Determinar qué botón está presionado
            if (adcValue < BTN_NONE_THRESHOLD) {
                Serial.println("Status: NO BUTTON PRESSED");
            }
            else if (adcValue >= BTN_PLAY_STOP_MIN && adcValue <= BTN_PLAY_STOP_MAX) {
                Serial.println("Button: PLAY/STOP ✓");
            }
            else if (adcValue >= BTN_MUTE_MIN && adcValue <= BTN_MUTE_MAX) {
                Serial.println("Button: MUTE/CLEAR ✓");
            }
            else if (adcValue >= BTN_BACK_MIN && adcValue <= BTN_BACK_MAX) {
                Serial.println("Button: BACK ✓");
            }
            else {
                Serial.println("Status: ⚠️ VALUE OUT OF RANGE!");
                Serial.println("\nSuggested action:");
                
                if (adcValue < BTN_PLAY_STOP_MIN) {
                    Serial.printf("  → Lower BTN_PLAY_STOP_MIN to ~%d\n", adcValue - 50);
                }
                else if (adcValue > BTN_PLAY_STOP_MAX && adcValue < BTN_MUTE_MIN) {
                    Serial.printf("  → Adjust: BTN_PLAY_STOP_MAX=%d, BTN_MUTE_MIN=%d\n", 
                                 adcValue + 100, adcValue - 100);
                }
                else if (adcValue > BTN_MUTE_MAX && adcValue < BTN_BACK_MIN) {
                    Serial.printf("  → Adjust: BTN_MUTE_MAX=%d, BTN_BACK_MIN=%d\n", 
                                 adcValue + 100, adcValue - 100);
                }
                else if (adcValue > BTN_BACK_MAX) {
                    Serial.printf("  → Raise BTN_BACK_MAX to ~%d\n", adcValue + 100);
                }
            }
            Serial.printf("═══════════════════════════════════\n\n");
        }
    }
}

void handleVolume() {
    int raw = analogRead(ROTARY_ANGLE_PIN);
    int newVol = map(raw, 0, 4095, 30, 0);  // Invertido: girar derecha = subir volumen
    
    if (abs(newVol - lastVolumeLevel) > 1) {
        volumeLevel = newVol;
        lastVolumeLevel = newVol;
        
        Serial.printf("► Volume: %d/30\n", volumeLevel);
        showVolumeOnTM1638();
        lastDisplayChange = millis();
        
        needsHeaderUpdate = true;
    }
}

// ============================================
// TM1638 DISPLAYS
// ============================================
void showInstrumentOnTM1638(int track) {
    lastInstrumentPlayed = track;
    instrumentDisplayTime = millis();
    currentDisplayMode = DISPLAY_INSTRUMENT;
    
    tm1.displayText(instrumentNames[track]);
    
    char display[9];
    snprintf(display, 9, "TRACK %d ", track + 1);
    tm2.displayText(display);
}

void showBPMOnTM1638() {
    currentDisplayMode = DISPLAY_BPM;
    char display1[9], display2[9];
    snprintf(display1, 9, "BPM %3d ", tempo);
    snprintf(display2, 9, "STEP %2d ", currentStep + 1);
    tm1.displayText(display1);
    tm2.displayText(display2);
}

void showVolumeOnTM1638() {
    currentDisplayMode = DISPLAY_VOLUME;
    char display1[9], display2[9];
    snprintf(display1, 9, "VOL  %2d ", volumeLevel);
    snprintf(display2, 9, "--------");
    tm1.displayText(display1);
    tm2.displayText(display2);
}

void updateTM1638Displays() {
    unsigned long currentTime = millis();
    
    if (currentDisplayMode == DISPLAY_INSTRUMENT) {
        if (currentTime - instrumentDisplayTime > 2000) {
            currentDisplayMode = DISPLAY_BPM;
            showBPMOnTM1638();
        }
        return;
    }
    
    if (currentScreen == SCREEN_MENU) {
        tm1.displayText("RED 808 ");
        tm2.displayText("  MENU  ");
        
    } else if (currentScreen == SCREEN_LIVE) {
        if (currentTime - lastDisplayChange > 3000) {
            lastDisplayChange = currentTime;
            if (currentDisplayMode == DISPLAY_BPM) {
                showVolumeOnTM1638();
            } else {
                showBPMOnTM1638();
            }
        }
        
    } else if (currentScreen == SCREEN_SEQUENCER) {
        char display1[9], display2[9];
        if (isPlaying) {
            snprintf(display1, 9, "PLAY %2d ", currentStep + 1);
            snprintf(display2, 9, "BPM %3d ", tempo);
        } else {
            // Mostrar instrumento completo en ambos displays
            snprintf(display1, 9, "TR %d    ", selectedTrack + 1);
            tm1.displayText(display1);
            tm2.displayText(instrumentNames[selectedTrack]);
            return; // Salir temprano para no sobrescribir
        }
        tm1.displayText(display1);
        tm2.displayText(display2);
        
    } else if (currentScreen == SCREEN_SETTINGS) {
        char display1[9];
        snprintf(display1, 9, "KIT  %d  ", currentKit + 1);
        tm1.displayText(display1);
        tm2.displayText(kits[currentKit].name.substring(0, 8).c_str());
        
    } else if (currentScreen == SCREEN_DIAGNOSTICS) {
        bool allOk = diagnostic.tftOk && diagnostic.tm1638_1_Ok && 
                     diagnostic.tm1638_2_Ok && diagnostic.encoderOk;
        tm1.displayText(allOk ? "ALL  OK " : " ERROR  ");
        tm2.displayText(diagnostic.sdCardOk ? "SD   OK " : "NO   SD ");
    }
}

void updateLEDFeedback() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < 16; i++) {
        if (ledActive[i]) {
            if (currentTime >= ledOffTime[i]) {
                setLED(i, false);
                ledActive[i] = false;
            }
        }
    }
}

void updateAudioVisualization() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastVizUpdate > 50) {
        lastVizUpdate = currentTime;
        
        for (int i = 0; i < 8; i++) {
            if (audioLevels[i] > 0) {
                audioLevels[i] -= 15;
                if (audioLevels[i] < 0) audioLevels[i] = 0;
            }
        }
        
        if (currentScreen == SCREEN_LIVE) {
            drawVUMeters();
        }
    }
}

// ============================================
// DRAW FUNCTIONS
// ============================================
void drawBootScreen() {
    // Esta función ya no se usa - reemplazada por drawConsoleBootScreen()
}

void drawConsoleBootScreen() {
    // Fondo degradado oscuro moderno
    for (int y = 0; y < 320; y += 2) {
        uint8_t brightness = map(y, 0, 320, 8, 2);
        uint16_t color = tft.color565(brightness, 0, 0);
        tft.drawFastHLine(0, y, 480, color);
    }
    
    // LOGO PRINCIPAL - Grande y con efecto
    tft.fillRoundRect(90, 30, 300, 80, 10, THEME_RED808.primary);
    tft.drawRoundRect(90, 30, 300, 80, 10, THEME_RED808.accent);
    tft.drawRoundRect(92, 32, 296, 76, 8, THEME_RED808.accent2);
    
    tft.setTextSize(6);
    tft.setTextColor(THEME_RED808.accent);
    tft.setCursor(125, 50);
    tft.println("RED808");
    
    // Líneas decorativas
    for (int i = 0; i < 3; i++) {
        tft.drawFastHLine(50, 120 + i, 380, THEME_RED808.accent);
    }
    
    delay(400);
    
    // TARJETAS DE SISTEMA - Diseño moderno tipo dashboard
    const int cardY = 140;
    const int cardH = 50;
    const int cardW = 140;
    const int spacing = 10;
    
    // Tarjeta 1: CPU
    int x1 = 20;
    tft.fillRoundRect(x1, cardY, cardW, cardH, 8, 0x2000);
    tft.drawRoundRect(x1, cardY, cardW, cardH, 8, THEME_RED808.accent);
    tft.setTextSize(1);
    tft.setTextColor(THEME_RED808.accent2);
    tft.setCursor(x1 + 10, cardY + 10);
    tft.println("ESP32");
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF);
    tft.setCursor(x1 + 10, cardY + 25);
    tft.println("240MHz");
    delay(200);
    
    // Tarjeta 2: RAM
    int x2 = x1 + cardW + spacing;
    tft.fillRoundRect(x2, cardY, cardW, cardH, 8, 0x2000);
    tft.drawRoundRect(x2, cardY, cardW, cardH, 8, THEME_RED808.accent);
    tft.setTextSize(1);
    tft.setTextColor(THEME_RED808.accent2);
    tft.setCursor(x2 + 10, cardY + 10);
    tft.println("MEMORY");
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF);
    tft.setCursor(x2 + 10, cardY + 25);
    tft.println("320KB");
    delay(200);
    
    // Tarjeta 3: DISPLAY
    int x3 = x2 + cardW + spacing;
    tft.fillRoundRect(x3, cardY, cardW, cardH, 8, 0x2000);
    tft.drawRoundRect(x3, cardY, cardW, cardH, 8, THEME_RED808.accent);
    tft.setTextSize(1);
    tft.setTextColor(THEME_RED808.accent2);
    tft.setCursor(x3 + 10, cardY + 10);
    tft.println("DISPLAY");
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF);
    tft.setCursor(x3 + 10, cardY + 25);
    tft.println("480x320");
    delay(200);
    
    // BARRA DE PROGRESO MODERNA
    int progY = 210;
    tft.setTextSize(1);
    tft.setTextColor(THEME_RED808.accent2);
    tft.setCursor(20, progY);
    tft.println("INITIALIZING SYSTEM...");
    
    const int barX = 20;
    const int barY = progY + 20;
    const int barW = 440;
    const int barH = 20;
    
    // Fondo de la barra
    tft.fillRoundRect(barX, barY, barW, barH, 10, 0x1800);
    tft.drawRoundRect(barX, barY, barW, barH, 10, THEME_RED808.accent);
    
    // Animación de carga progresiva
    const char* stages[] = {
        "Hardware",
        "Displays",
        "TM1638 #1",
        "TM1638 #2",
        "Encoder",
        "Buttons",
        "SD Card",
        "Audio Kits",
        "Patterns",
        "Sequencer"
    };
    
    for (int i = 0; i < 10; i++) {
        int progress = map(i + 1, 0, 10, 0, barW - 4);
        
        // Barra de progreso con gradiente
        for (int p = 0; p < progress; p += 2) {
            uint8_t red = map(p, 0, progress, 200, 255);
            uint16_t color = tft.color565(red, 0, 0);
            tft.fillRect(barX + 2 + p, barY + 2, 2, barH - 4, color);
        }
        
        // Texto del stage actual
        tft.fillRect(20, progY + 45, 200, 16, 0x0000);
        tft.setTextSize(2);
        tft.setTextColor(THEME_RED808.accent);
        tft.setCursor(20, progY + 45);
        tft.printf("> %s", stages[i]);
        
        // Porcentaje
        tft.fillRect(380, progY + 45, 80, 16, 0x0000);
        tft.setTextColor(0xFFFF);
        tft.setCursor(380, progY + 45);
        tft.printf("%d%%", (i + 1) * 10);
        
        delay(180);
    }
    
    delay(200);
    
    // MENSAJE FINAL - Grande y centrado
    tft.fillRect(0, progY + 70, 480, 40, 0x0000);
    
    // Caja de "READY"
    tft.fillRoundRect(140, progY + 70, 200, 45, 10, THEME_RED808.accent);
    tft.drawRoundRect(140, progY + 70, 200, 45, 10, 0xFFFF);
    tft.setTextSize(3);
    tft.setTextColor(0x0000);
    tft.setCursor(175, progY + 80);
    tft.println("READY");
    
    // Iconos de check
    for (int i = 0; i < 3; i++) {
        tft.fillCircle(120 + i * 5, progY + 92, 3, 0x07E0);
        tft.fillCircle(360 + i * 5, progY + 92, 3, 0x07E0);
    }
    
    delay(800);
}

void drawSpectrumAnimation() {
    // Fondo negro con degradado sutil
    for (int y = 0; y < 320; y += 4) {
        uint8_t brightness = map(y, 0, 320, 5, 0);
        uint16_t color = tft.color565(brightness, 0, 0);
        tft.fillRect(0, y, 480, 4, color);
    }
    
    // LOGO CON EFECTO GLOW
    // Sombra/glow exterior
    for (int offset = 4; offset > 0; offset--) {
        uint8_t brightness = map(offset, 4, 1, 50, 150);
        uint16_t glowColor = tft.color565(brightness, 0, 0);
        tft.drawRoundRect(86 - offset, 26 - offset, 308 + offset * 2, 88 + offset * 2, 12, glowColor);
    }
    
    // Caja del logo
    tft.fillRoundRect(90, 30, 300, 80, 10, 0x1000);
    tft.drawRoundRect(90, 30, 300, 80, 10, THEME_RED808.accent);
    tft.drawRoundRect(92, 32, 296, 76, 8, THEME_RED808.accent2);
    
    tft.setTextSize(7);
    tft.setTextColor(THEME_RED808.accent);
    tft.setCursor(115, 45);
    tft.println("RED808");
    
    // Subtítulo
    tft.setTextSize(2);
    tft.setTextColor(THEME_RED808.accent2);
    tft.setCursor(160, 125);
    tft.println("DRUM MACHINE");
    
    // VISUALIZADOR DE AUDIO MODERNO - Circular
    const int centerX = 240;
    const int centerY = 210;
    const int numBars = 24;
    const float angleStep = 360.0 / numBars;
    
    // Buffer de alturas previas para suavizado
    static int prevHeights[24] = {0};
    
    for (int frame = 0; frame < 25; frame++) {
        // Borrar área del círculo
        tft.fillCircle(centerX, centerY, 75, 0x0000);
        
        // Dibujar barras radiales
        for (int i = 0; i < numBars; i++) {
            float angle = i * angleStep * PI / 180.0;
            
            // Altura con onda suave
            float wave = sin((frame * 0.3) + (i * 0.4)) * 20 + 40;
            int targetHeight = random(20, 60) * 0.3 + wave * 0.7;
            
            // Suavizado
            prevHeights[i] = (prevHeights[i] * 0.7) + (targetHeight * 0.3);
            int height = prevHeights[i];
            
            // Posición inicial (centro)
            int startRadius = 15;
            int endRadius = startRadius + height;
            
            // Calcular posiciones
            int x1 = centerX + cos(angle) * startRadius;
            int y1 = centerY + sin(angle) * startRadius;
            int x2 = centerX + cos(angle) * endRadius;
            int y2 = centerY + sin(angle) * endRadius;
            
            // Gradiente de color según altura
            uint8_t red = map(height, 20, 60, 150, 255);
            uint16_t color = tft.color565(red, 0, 0);
            
            // Dibujar línea gruesa
            for (int thick = -2; thick <= 2; thick++) {
                int x1t = x1 + cos(angle + PI/2) * thick;
                int y1t = y1 + sin(angle + PI/2) * thick;
                int x2t = x2 + cos(angle + PI/2) * thick;
                int y2t = y2 + sin(angle + PI/2) * thick;
                tft.drawLine(x1t, y1t, x2t, y2t, color);
            }
            
            // Punto brillante en el extremo
            tft.fillCircle(x2, y2, 2, THEME_RED808.accent);
        }
        
        // Círculo central decorativo
        tft.fillCircle(centerX, centerY, 12, THEME_RED808.primary);
        tft.drawCircle(centerX, centerY, 12, THEME_RED808.accent);
        tft.drawCircle(centerX, centerY, 13, THEME_RED808.accent2);
        
        // Texto LOADING centrado con parpadeo suave
        if (frame % 8 < 6) {  // Parpadeo más lento
            tft.setTextSize(2);
            tft.setTextColor(THEME_RED808.accent);
            tft.setCursor(185, 295);
            tft.println("LOADING...");
        } else {
            tft.fillRect(185, 295, 110, 20, 0x0000);
        }
        
        delay(70);  // Animación fluida
    }
    
    // Transición suave final
    delay(500);
}

void drawMainMenu() {
    tft.fillScreen(COLOR_BG);
    
    tft.fillRect(0, 0, 480, 50, COLOR_NAVY);
    tft.drawFastHLine(0, 50, 480, COLOR_ACCENT);
    
    tft.setTextSize(4);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(150, 10);
    tft.println("RED808");
    
    int itemHeight = 52;
    int startY = 70;
    
    for (int i = 0; i < menuItemCount; i++) {
        int y = startY + i * itemHeight;
        
        if (i == menuSelection) {
            tft.fillRoundRect(30, y, 420, 45, 8, COLOR_ACCENT);
            tft.drawRoundRect(30, y, 420, 45, 8, COLOR_ACCENT2);
            tft.setTextSize(3);
            tft.setTextColor(COLOR_TEXT);
        } else {
            tft.fillRoundRect(30, y, 420, 45, 8, COLOR_NAVY_LIGHT);
            tft.setTextSize(2);
            tft.setTextColor(COLOR_TEXT_DIM);
        }
        
        tft.setCursor(50, y + (i == menuSelection ? 12 : 14));
        tft.print(menuItems[i]);
    }
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(150, 305);
    tft.println("ENCODER: SELECT  |  BTN: CONFIRM");
}

// Función optimizada para solo actualizar items del menú sin parpadeo
void drawMenuItems(int oldSelection, int newSelection) {
    int itemHeight = 52;
    int startY = 70;
    
    // Redibujar item antiguo (no seleccionado)
    if (oldSelection >= 0 && oldSelection < menuItemCount) {
        int y = startY + oldSelection * itemHeight;
        tft.fillRoundRect(30, y, 420, 45, 8, COLOR_NAVY_LIGHT);
        tft.setTextSize(2);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(50, y + 14);
        tft.print(menuItems[oldSelection]);
    }
    
    // Redibujar item nuevo (seleccionado)
    if (newSelection >= 0 && newSelection < menuItemCount) {
        int y = startY + newSelection * itemHeight;
        tft.fillRoundRect(30, y, 420, 45, 8, COLOR_ACCENT);
        tft.drawRoundRect(30, y, 420, 45, 8, COLOR_ACCENT2);
        tft.setTextSize(3);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(50, y + 12);
        tft.print(menuItems[newSelection]);
    }
}

void drawLiveScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader();
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(150, 58);
    tft.println("SAMPLE PADS");
    
    // Estado del DFPlayer
    tft.setTextSize(1);
    if (samplerInitialized) {
        tft.setTextColor(COLOR_SUCCESS);
        tft.setCursor(340, 65);
        tft.printf("VOL:%02d", dfplayerVolume);
    } else {
        tft.setTextColor(COLOR_ERROR);
        tft.setCursor(340, 65);
        tft.println("NO DFPLAYER");
    }
    
    // ========== SAMPLES S1-S8 ==========
    const int padW = 108;
    const int padH = 80;
    const int startX = 10;
    const int startY = 88;
    const int spacingX = 8;
    const int spacingY = 8;
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(startX, startY - 12);
    tft.println("SAMPLES [S1-S8]");
    
    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        int x = startX + col * (padW + spacingX);
        int y = startY + row * (padH + spacingY);
        
        // Sombra
        tft.fillRoundRect(x + 2, y + 2, padW, padH, 8, 0x1000);
        
        // Pad principal
        uint16_t padColor = COLOR_PRIMARY_LIGHT;
        if (samples[i].isPlaying) {
            padColor = COLOR_ACCENT;
        } else if (i == currentSample) {
            padColor = COLOR_PRIMARY;
        }
        
        tft.fillRoundRect(x, y, padW, padH, 8, padColor);
        tft.drawRoundRect(x, y, padW, padH, 8, COLOR_ACCENT);
        
        // Número del sample
        tft.fillRoundRect(x + 5, y + 5, 24, 24, 4, COLOR_ACCENT);
        tft.setTextSize(2);
        tft.setTextColor(0x0000);
        tft.setCursor(x + 11, y + 10);
        tft.printf("%d", i + 1);
        
        // Nombre
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(x + 35, y + 10);
        tft.println("SAMPLE");
        tft.setCursor(x + 35, y + 22);
        tft.printf("%03d.WAV", i + 1);
        
        // Modo
        tft.setTextSize(1);
        tft.setTextColor(samples[i].isLooping ? COLOR_WARNING : COLOR_TEXT_DIM);
        tft.setCursor(x + 8, y + 40);
        if (samples[i].isLooping) {
            tft.println("LOOP");
        } else {
            tft.println("1-SHOT");
        }
        
        // Indicador de reproducción
        if (samples[i].isPlaying) {
            tft.fillRoundRect(x + 5, y + padH - 20, padW - 10, 15, 4, COLOR_SUCCESS);
            tft.setTextSize(1);
            tft.setTextColor(0x0000);
            tft.setCursor(x + 30, y + padH - 17);
            tft.println("PLAYING");
        }
        
        // Botón TM1638
        tft.setTextSize(1);
        tft.setTextColor(COLOR_ACCENT2);
        tft.setCursor(x + padW - 20, y + padH - 12);
        tft.printf("S%d", i + 1);
    }
    
    // ========== FUNCIONES S9-S16 ==========
    const int funcY = 267;
    const int funcW = 56;
    const int funcH = 28;
    const int funcSpacing = 4;
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(startX, funcY - 12);
    tft.println("FUNCTIONS [S9-S16]");
    
    const char* funcLabels[] = {
        "LOOP",    // S9
        "HOLD",    // S10
        "STOP",    // S11
        "VOL+",    // S12
        "VOL-",    // S13
        "PREV",    // S14
        "NEXT",    // S15
        "CLEAR"    // S16
    };
    
    for (int i = 0; i < 8; i++) {
        int x = startX + i * (funcW + funcSpacing);
        
        // Fondo del botón
        uint16_t btnColor = COLOR_PRIMARY;
        
        // Destacar modo activo
        if (i == 0 && globalSamplerMode == SAMPLER_LOOP) {
            btnColor = COLOR_WARNING;
        } else if (i == 1 && globalSamplerMode == SAMPLER_HOLD) {
            btnColor = COLOR_WARNING;
        }
        
        tft.fillRoundRect(x, funcY, funcW, funcH, 5, btnColor);
        tft.drawRoundRect(x, funcY, funcW, funcH, 5, COLOR_ACCENT);
        
        // Texto
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        int labelLen = strlen(funcLabels[i]);
        int labelX = x + (funcW - labelLen * 6) / 2;
        tft.setCursor(labelX, funcY + 4);
        tft.println(funcLabels[i]);
        
        // Número del botón
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(x + funcW - 14, funcY + funcH - 10);
        tft.printf("%d", 9 + i);
    }
    
    // Footer con instrucciones
    tft.fillRect(0, 305, 480, 15, COLOR_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(30, 307);
    tft.println("S1-8:PLAY | S9-16:FUNCTIONS | ENCODER:SELECT | BACK:MENU");
}

void drawSequencerScreen() {
    static int lastStep = -1;
    
    if (needsFullRedraw) {
        tft.fillScreen(COLOR_BG);
        drawHeader();
    }
    
    const int infoY = 58;
    
    if (needsFullRedraw) {
        tft.setTextSize(2);
        tft.setTextColor(COLOR_ACCENT2);
        tft.setCursor(10, infoY);
        tft.print("SEQ");
        
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(60, infoY);
        tft.printf("PATTERN %d", currentPattern + 1);
        
        tft.setCursor(200, infoY);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.print(patterns[currentPattern].name.c_str());
        
        // Mostrar instrumento actual resaltado con su color
        tft.fillRoundRect(298, infoY - 2, 165, 22, 4, getInstrumentColor(selectedTrack));
        tft.setTextColor(COLOR_BG);
        tft.setCursor(305, infoY);
        String instName = String(instrumentNames[selectedTrack]);
        instName.trim();
        tft.printf("%d:%s", selectedTrack + 1, instName.c_str());
        
        if (isPlaying) {
            tft.fillRoundRect(400, 56, 70, 24, 4, COLOR_SUCCESS);
            tft.setTextColor(COLOR_BG);
            tft.setCursor(412, 60);
            tft.print("PLAY");
        } else {
            tft.drawRoundRect(400, 56, 70, 24, 4, COLOR_BORDER);
            tft.setTextColor(COLOR_TEXT_DIM);
            tft.setCursor(412, 60);
            tft.print("STOP");
        }
    }
    
    const int gridX = 8;
    const int gridY = 88;
    const int cellW = 27;
    const int cellH = 24;
    const int labelW = 38;
    
    Pattern& pattern = patterns[currentPattern];
    
    if (needsFullRedraw) {
        tft.fillRoundRect(gridX - 2, gridY - 2, 468, 210, 8, COLOR_NAVY);
        
        tft.setTextSize(1);
        for (int s = 0; s < MAX_STEPS; s++) {
            int x = gridX + labelW + s * (cellW + 1);
            tft.setTextColor((s % 4 == 0) ? COLOR_ACCENT : COLOR_TEXT_DIM);
            tft.setCursor(x + 6, gridY - 10);
            tft.printf("%d", (s + 1) % 10);
        }
        
        tft.setTextSize(2);
        for (int t = 0; t < MAX_TRACKS; t++) {
            int y = gridY + 2 + t * (cellH + 2);
            // Mostrar MUTED con color diferente
            if (pattern.muted[t]) {
                tft.setTextColor(COLOR_ERROR);
                tft.setCursor(gridX + 4, y + 2);
                tft.print("M");
                tft.setCursor(gridX + 14, y + 2);
                tft.print(trackNames[t]);
            } else {
                // Usar color único por instrumento
                tft.setTextColor(getInstrumentColor(t));
                tft.setCursor(gridX + 4, y + 2);
                tft.print(trackNames[t]);
            }
        }
        
        lastStep = -1;
    }
    
    for (int t = 0; t < MAX_TRACKS; t++) {
        for (int s = 0; s < MAX_STEPS; s++) {
            if (needsFullRedraw || (s == currentStep || s == lastStep)) {
                int x = gridX + labelW + s * (cellW + 1);
                int y = gridY + 2 + t * (cellH + 2);
                
                uint16_t color;
                uint16_t border = COLOR_NAVY;
                
                if (isPlaying && s == currentStep) {
                    // Durante reproducción, usar color del instrumento si está activo
                    color = pattern.steps[t][s] ? getInstrumentColor(t) : COLOR_NAVY_LIGHT;
                    border = pattern.steps[t][s] ? getInstrumentColor(t) : COLOR_WARNING;
                } else if (pattern.steps[t][s]) {
                    // Step activo: usar color del instrumento
                    color = getInstrumentColor(t);
                    border = getInstrumentColor(t);
                } else {
                    color = COLOR_NAVY_LIGHT;
                }
                
                tft.fillRoundRect(x, y, cellW, cellH, 3, color);
                if (border != COLOR_NAVY) {
                    tft.drawRoundRect(x, y, cellW, cellH, 3, border);
                }
                
                if (s % 4 == 0 && needsFullRedraw) {
                    tft.drawFastVLine(x - 1, gridY, 200, COLOR_ACCENT);
                }
            }
        }
    }
    
    lastStep = currentStep;
    
    if (needsFullRedraw) {
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(5, 305);
        tft.println("S1-S16:TOGGLE | ENC:TRACK | PLAY/STOP | MUTE(HOLD:CLEAR) | BACK");
    }
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader();
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(180, 58);
    tft.println("SETTINGS");
    
    // ========== COLUMNA IZQUIERDA: DRUM KITS ==========
    const int leftX = 20;
    const int sectionY = 95;
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(leftX, sectionY);
    tft.println("DRUM KITS");
    tft.drawFastHLine(leftX, sectionY + 25, 200, COLOR_BORDER);
    
    for (int i = 0; i < MAX_KITS; i++) {
        int y = sectionY + 40 + i * 45;
        
        if (i == currentKit) {
            tft.fillRoundRect(leftX, y, 200, 38, 6, COLOR_ACCENT);
            tft.drawRoundRect(leftX, y, 200, 38, 6, COLOR_ACCENT2);
            tft.setTextSize(2);
            tft.setTextColor(COLOR_TEXT);
        } else {
            tft.fillRoundRect(leftX, y, 200, 38, 6, COLOR_PRIMARY_LIGHT);
            tft.setTextSize(2);
            tft.setTextColor(COLOR_TEXT_DIM);
        }
        
        // Número del kit
        tft.fillCircle(leftX + 20, y + 19, 10, i == currentKit ? COLOR_ACCENT2 : COLOR_PRIMARY);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(leftX + 16, y + 13);
        tft.printf("%d", i + 1);
        
        // Nombre del kit
        tft.setTextSize(2);
        tft.setTextColor(i == currentKit ? COLOR_TEXT : COLOR_TEXT_DIM);
        tft.setCursor(leftX + 40, y + 11);
        tft.print(kits[i].name.c_str());
    }
    
    // ========== COLUMNA DERECHA: VISUAL THEMES ==========
    const int rightX = 260;
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(rightX, sectionY);
    tft.println("THEMES");
    tft.drawFastHLine(rightX, sectionY + 25, 200, COLOR_BORDER);
    
    for (int i = 0; i < THEME_COUNT; i++) {
        int y = sectionY + 40 + i * 45;
        const ColorTheme* theme = THEMES[i];
        
        if (i == currentTheme) {
            tft.fillRoundRect(rightX, y, 200, 38, 6, theme->accent);
            tft.drawRoundRect(rightX, y, 200, 38, 6, theme->accent2);
            tft.setTextSize(2);
            tft.setTextColor(COLOR_TEXT);
        } else {
            tft.fillRoundRect(rightX, y, 200, 38, 6, theme->primary);
            tft.setTextSize(2);
            tft.setTextColor(theme->textDim);
        }
        
        // Preview de color (cuadrado con color del tema)
        tft.fillRoundRect(rightX + 10, y + 9, 20, 20, 3, theme->accent);
        tft.drawRoundRect(rightX + 10, y + 9, 20, 20, 3, theme->accent2);
        
        // Nombre del tema
        tft.setTextSize(2);
        tft.setTextColor(i == currentTheme ? COLOR_TEXT : theme->text);
        tft.setCursor(rightX + 40, y + 11);
        tft.print(theme->name);
    }
    
    // Footer con instrucciones
    tft.fillRect(0, 295, 480, 25, COLOR_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, 305);
    tft.print("S1-S3: KIT");
    tft.setCursor(110, 305);
    tft.print("|");
    tft.setCursor(125, 305);
    tft.print("S5-S8: THEME");
    tft.setCursor(235, 305);
    tft.print("|");
    tft.setCursor(250, 305);
    tft.print("ENCODER: BPM");
    tft.setCursor(360, 305);
    tft.print("|");
    tft.setCursor(375, 305);
    tft.print("BACK: MENU");
}


void drawDiagnosticsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader();
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(160, 58);
    tft.println("DIAGNOSTICS");
    
    int y = 88;
    const int lineHeight = 34;
    
    const char* items[] = {
        "TFT DISPLAY", 
        "TM1638 #1 (S1-8)", 
        "TM1638 #2 (S9-16)", 
        "ROTARY ENCODER",
        "ROTARY ANGLE",
        "SD CARD"
    };
    bool status[] = {
        diagnostic.tftOk,
        diagnostic.tm1638_1_Ok,
        diagnostic.tm1638_2_Ok,
        diagnostic.encoderOk,
        true,
        diagnostic.sdCardOk
    };
    
    for (int i = 0; i < 6; i++) {
        tft.fillRoundRect(32, y + 2, 416, 28, 6, COLOR_NAVY);
        tft.fillRoundRect(30, y, 416, 28, 6, COLOR_NAVY_LIGHT);
        
        uint16_t indicatorColor = status[i] ? COLOR_SUCCESS : COLOR_ERROR;
        tft.fillCircle(50, y + 14, 7, indicatorColor);
        tft.drawCircle(50, y + 14, 8, indicatorColor);
        
        tft.setTextSize(2);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(70, y + 6);
        tft.print(items[i]);
        
        tft.setTextSize(2);
        tft.setTextColor(status[i] ? COLOR_SUCCESS : COLOR_ERROR);
        tft.setCursor(390, y + 6);
        tft.print(status[i] ? "OK" : "ERR");
        
        y += lineHeight;
    }
    
    y += 8;
    tft.fillRoundRect(30, y, 420, 26, 6, COLOR_NAVY);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(45, y + 9);
    tft.printf("FILES:%d  VOL:%d/30  16-STEPS  2xTM1638", 
               diagnostic.filesFound, volumeLevel);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(180, 305);
    tft.println("BACK: MENU");
}

void drawHeader() {
    tft.fillRect(0, 0, 480, 48, COLOR_NAVY);
    tft.drawFastHLine(0, 48, 480, COLOR_ACCENT);
    
    tft.setTextSize(3);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(10, 10);
    tft.println("R808");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(100, 14);
    tft.printf("%d", tempo);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(140, 20);
    tft.print("BPM");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(180, 14);
    tft.printf("VOL %d", volumeLevel);
    
    if (currentScreen == SCREEN_SEQUENCER) {
        tft.setTextSize(2);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(260, 14);
        tft.printf("P%d", currentPattern + 1);
        
        // Mostrar instrumento completo con su color
        tft.setTextColor(getInstrumentColor(selectedTrack));
        tft.setCursor(300, 14);
        String instName = String(instrumentNames[selectedTrack]);
        instName.trim();
        tft.print(instName.c_str());
    }
    
    if (isPlaying) {
        tft.fillCircle(455, 24, 10, COLOR_SUCCESS);
        tft.fillTriangle(450, 18, 450, 30, 460, 24, COLOR_BG);
    } else {
        tft.drawCircle(455, 24, 10, COLOR_BORDER);
        tft.fillRect(451, 19, 3, 10, COLOR_TEXT_DIM);
        tft.fillRect(456, 19, 3, 10, COLOR_TEXT_DIM);
    }
}

void drawVUMeters() {
    const int padW = 112;
    const int padH = 100;
    const int startX = 6;
    const int startY = 88;
    const int spacingX = 6;
    const int spacingY = 6;
    
    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        int x = startX + col * (padW + spacingX);
        int y = startY + row * (padH + spacingY);
        
        tft.fillRect(x + 10, y + padH - 15, padW - 20, 8, COLOR_NAVY_LIGHT);
        
        if (audioLevels[i] > 0) {
            int barWidth = map(audioLevels[i], 0, 100, 0, padW - 20);
            tft.fillRoundRect(x + 10, y + padH - 15, barWidth, 8, 4, COLOR_SUCCESS);
        }
    }
}

// ============================================
// CONTROL FUNCTIONS
// ============================================
void changeTempo(int delta) {
    tempo = constrain(tempo + delta, MIN_BPM, MAX_BPM);
    calculateStepInterval();
}

void changePattern(int delta) {
    currentPattern = (currentPattern + delta + MAX_PATTERNS) % MAX_PATTERNS;
    currentStep = 0;
    needsFullRedraw = true;
}

void changeKit(int delta) {
    currentKit = (currentKit + delta + MAX_KITS) % MAX_KITS;
    needsFullRedraw = true;
}

void changeTheme(int delta) {
    currentTheme = (currentTheme + delta + THEME_COUNT) % THEME_COUNT;
    activeTheme = THEMES[currentTheme];
    needsFullRedraw = true;
    
    // Actualizar TM1638 con nombre del tema
    char display[9];
    snprintf(display, 9, "%-8s", activeTheme->name);
    tm1.displayText(display);
    tm2.displayText(display);
    
    Serial.printf("► Theme changed to: %s\n", activeTheme->name);
}

void changeScreen(Screen newScreen) {
    currentScreen = newScreen;
    needsFullRedraw = true;
    lastDisplayChange = millis();
    setAllLEDs(0x0000);
    
    // Si entramos al sequencer, mostrar LEDs del track seleccionado
    if (newScreen == SCREEN_SEQUENCER && !isPlaying) {
        updateStepLEDsForTrack(selectedTrack);
        showInstrumentOnTM1638(selectedTrack);
    }
}

void toggleStep(int track, int step) {
    patterns[currentPattern].steps[track][step] = 
        !patterns[currentPattern].steps[track][step];
    needsGridUpdate = true;
    
    Serial.printf("► TOGGLE: Track %d, Step %d = %s\n", 
                  track, step, 
                  patterns[currentPattern].steps[track][step] ? "ON" : "OFF");
}