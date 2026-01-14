#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TM1638plus.h>
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
#define TFT_BL   21

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

// SD Card Reader (6 pins SPI)
#define SD_CS   15
//#define SD_MOSI 23
//#define SD_MISO 19
//#define SD_SCK  18

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

// TEMA PROFESIONAL AZUL NAVY
#define COLOR_BG           0x0821
#define COLOR_NAVY         0x1082
#define COLOR_NAVY_LIGHT   0x2945
#define COLOR_ACCENT       0x3D8F
#define COLOR_ACCENT2      0x04FF
#define COLOR_TEXT         0xFFFF
#define COLOR_TEXT_DIM     0xBDF7
#define COLOR_SUCCESS      0x07E0
#define COLOR_WARNING      0xFD20
#define COLOR_ERROR        0xF800
#define COLOR_BORDER       0x4A49

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

struct DiagnosticInfo {
    bool tftOk;
    bool tm1638_1_Ok;
    bool tm1638_2_Ok;
    bool encoderOk;
    bool sdCardOk;
    int filesFound;
    String lastError;
    
    DiagnosticInfo() : 
        tftOk(false), 
        tm1638_1_Ok(false), 
        tm1638_2_Ok(false), 
        encoderOk(false), 
        sdCardOk(false), 
        filesFound(0), 
        lastError("") {}
};

// ============================================
// GLOBAL OBJECTS
// ============================================
TFT_eSPI tft = TFT_eSPI();
TM1638plus tm1(TM1638_1_STB, TM1638_1_CLK, TM1638_1_DIO, true);
TM1638plus tm2(TM1638_2_STB, TM1638_2_CLK, TM1638_2_DIO, true);
// RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::TWO03);

// ============================================
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
void drawBootScreen();
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

void setupSDCard() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(120, 130);
    tft.println("Checking SD Card...");
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, SD_CS);
    
    if (SD.begin(SD_CS)) {
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
        Serial.println("  ✗ SD Card FAILED!");
        
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(120, 160);
        tft.println("SD Card not found");
    }
    
    delay(1000);
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
    
    drawBootScreen();
    
    Serial.println("► SD Card Init...");
    setupSDCard();
    
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║  TESTING 16 BUTTONS (5 SEC)       ║");
    Serial.println("╚═══════════════════════════════════╝");
    
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(3);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(100, 80);
    tft.println("BUTTON TEST");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(60, 130);
    tft.println("Press buttons S1-S16");
    
    tm1.displayText("BTN 1-8 ");
    tm2.displayText("BTN 9-16");
    
    testButtonsOnBoot();
    
    setupKits();
    setupPatterns();
    calculateStepInterval();
    
    // LED animation
    for (int i = 0; i < 16; i++) {
        setLED(i, true);
        delay(50);
    }
    delay(200);
    setAllLEDs(0x0000);
    delay(100);
    setAllLEDs(0xFFFF);
    delay(200);
    setAllLEDs(0x0000);
    
    // Splash
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(6);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(140, 90);
    tft.println("RED808");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(85, 155);
    tft.println("PROFESSIONAL DRUM MACHINE");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(160, 180);
    tft.println("V5 - 16 STEPS - NO AUDIO");
    
    tft.drawFastHLine(80, 200, 320, COLOR_ACCENT2);
    
    tft.fillRoundRect(190, 220, 100, 45, 8, COLOR_SUCCESS);
    tft.setTextSize(3);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(205, 230);
    tft.println("READY");
    
    tm1.displayText("READY   ");
    tm2.displayText("16 STEP ");
    
    delay(1500);
    
    currentScreen = SCREEN_MENU;
    needsFullRedraw = true;
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║       RED808 V5 READY!             ║");
    Serial.println("╚════════════════════════════════════╝\n");
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

void triggerDrum(int track) {
    Serial.printf("► TRIGGER: Track %d (%s) at Step %d\n", 
                  track, trackNames[track], currentStep + 1);
    
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
    
    if (buttons != 0) {
        Serial.printf("► RAW BUTTONS: 0x%04X\n", buttons);
        
        for (int i = 0; i < 16; i++) {
            if (buttons & (1 << i)) {
                Serial.printf("  ✓ S%d pressed\n", i + 1);
                
                if (currentScreen == SCREEN_LIVE) {
                    if (i < 8) {
                        triggerDrum(i);
                        setLED(i, true);
                        ledActive[i] = true;
                        ledOffTime[i] = millis() + 150;
                    }
                } else if (currentScreen == SCREEN_SEQUENCER && !isPlaying) {
                    // Toggle step del instrumento seleccionado
                    toggleStep(selectedTrack, i);
                    
                    // Actualizar todos los LEDs para reflejar el nuevo patrón
                    updateStepLEDsForTrack(selectedTrack);
                    
                    // Forzar actualización COMPLETA de la pantalla TFT
                    bool wasFullRedraw = needsFullRedraw;
                    needsFullRedraw = true;  // Forzar redibujado completo
                    drawSequencerScreen();   // Actualizar inmediatamente
                    needsFullRedraw = wasFullRedraw; // Restaurar estado
                    
                    // Mensaje de feedback
                    bool state = patterns[currentPattern].steps[selectedTrack][i];
                    Serial.printf("  Track %s, Step %d: %s\n", 
                                trackNames[selectedTrack], i + 1, state ? "ON" : "OFF");
                }
            }
        }
        
        delay(150);
    }
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
                // En settings: menos sensible (dividir por 4)
                int delta = rawDelta / 4;
                if (delta != 0) {
                    changeTempo(delta);
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
    int newVol = map(raw, 0, 4095, 0, 30);
    
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
    tft.fillScreen(COLOR_BG);
    
    tft.setTextSize(5);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(160, 100);
    tft.println("RED808");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(125, 155);
    tft.println("DRUM MACHINE V5");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT2);
    tft.setCursor(160, 180);
    tft.println("16 STEPS / 2x TM1638");
    
    tft.drawFastHLine(120, 200, 240, COLOR_ACCENT2);
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
    tft.setCursor(180, 58);
    tft.println("LIVE PADS");
    
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
        
        tft.fillRoundRect(x + 3, y + 3, padW, padH, 10, COLOR_NAVY);
        tft.fillRoundRect(x, y, padW, padH, 10, COLOR_NAVY_LIGHT);
        tft.drawRoundRect(x, y, padW, padH, 10, COLOR_ACCENT);
        
        uint16_t iconColor = (audioLevels[i] > 0) ? COLOR_SUCCESS : COLOR_ACCENT;
        tft.fillRoundRect(x + 10, y + 10, 25, 25, 4, iconColor);
        
        tft.setTextSize(3);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(x + 40, y + 15);
        tft.print(trackNames[i]);
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_DIM);
        int nameLen = strlen(instrumentNames[i]);
        int nameX = x + (padW - nameLen * 6) / 2;
        tft.setCursor(nameX, y + 50);
        tft.print(instrumentNames[i]);
        
        tft.setTextSize(2);
        tft.setTextColor(COLOR_ACCENT2);
        tft.setCursor(x + 8, y + 70);
        tft.printf("S%d", i + 1);
        
        if (audioLevels[i] > 0) {
            int barWidth = map(audioLevels[i], 0, 100, 0, padW - 20);
            tft.fillRoundRect(x + 10, y + padH - 15, barWidth, 8, 4, COLOR_SUCCESS);
        }
    }
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(60, 305);
    tft.println("TM1638:PLAY SOUND | ENCODER:SCROLL | BACK:MENU");
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
    
    const int sectionY = 88;
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(40, sectionY);
    tft.println("DRUM KITS");
    tft.drawFastHLine(40, sectionY + 25, 400, COLOR_BORDER);
    
    for (int i = 0; i < MAX_KITS; i++) {
        int y = sectionY + 40 + i * 48;
        
        if (i == currentKit) {
            tft.fillRoundRect(30, y, 420, 42, 8, COLOR_ACCENT);
            tft.drawRoundRect(30, y, 420, 42, 8, COLOR_ACCENT2);
            tft.setTextSize(3);
            tft.setTextColor(COLOR_TEXT);
        } else {
            tft.fillRoundRect(30, y, 420, 42, 8, COLOR_NAVY_LIGHT);
            tft.setTextSize(2);
            tft.setTextColor(COLOR_TEXT_DIM);
        }
        
        tft.fillCircle(55, y + 21, 12, i == currentKit ? COLOR_ACCENT2 : COLOR_NAVY);
        tft.setTextSize(2);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(50, y + 14);
        tft.printf("%d", i + 1);
        
        tft.setTextSize(i == currentKit ? 3 : 2);
        tft.setTextColor(i == currentKit ? COLOR_TEXT : COLOR_TEXT_DIM);
        tft.setCursor(85, y + (i == currentKit ? 10 : 12));
        tft.print(kits[i].name.c_str());
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_DIM);
        tft.setCursor(350, y + 17);
        tft.printf("/%02d", kits[i].folder);
    }
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(120, 305);
    tft.println("ENCODER: CHANGE KIT  |  BACK: MENU");
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