// ==========================================
// ADDITIONS — used by the split .cpp files
// ==========================================
extern TFT_eSPI   tft;
extern TFT_eSprite canvas;
extern int  W, H;
extern bool displayReady;

// Encoder
extern int  lastEncoderState;
extern int  encoderSteps;
extern bool longPressHandled;
extern int  lastButtonReading;
extern int  stableButtonState;
extern unsigned long lastDebounceTime;
extern unsigned long buttonDownAt;

// LoRa / sensor counters
extern bool loraInitialized;
extern int  lastRSSI;
extern float lastSNR;
extern uint32_t packetsReceived;
extern uint32_t sensorErrors;
extern uint32_t invalidPackets;
extern unsigned long lastPacketMillis;
extern unsigned long lastDisplayUpdate;

// UI legacy menu (being phased out)
extern bool menuOpen;
extern int  currentPage;
extern int  menuIndex;

// Threshold/calibration edit state (already partly here)
extern bool thresholdEditing;
extern uint8_t thresholdEditStep;
extern bool overrideEditing;
extern uint32_t draftOverrideMinutes;

// Touch
extern int lastTouchReading;
extern int stableTouchState;
extern unsigned long lastTouchChange;

// Function prototypes (shared across .cpp)
void backlightInit();
void backlightSet(float brightness);
void initializeDisplay();
void initializeEncoder();
void initializeLoRa();
void receiveSensorData();
void updateConnectionStatus();

void handleShortPress();
void handleLongPress();
void handleEncoderRotation(int direction);
void updateEncoder();

void drawHeader();
void drawTankGraphic();
void drawSignalBars(int x, int y, int level, uint16_t activeColor);
void drawWifiArc(int cx, int cy, int radius, uint16_t color);
void drawWifiIcon(int cx, int cy, int level);
int  getSignalLevel(int rssi);
uint16_t getSignalColor(int level);

void drawDashboard();
void drawLinkPage();
void drawDiagnosticsPage();
void drawWaterLevelPage();
void drawTankCalibrationPage();
void drawManualPumpPage();
void drawAutoThresholdsPage();
void drawAutoControlPage();
void drawOverrideTimePage();
void drawMenu();

void uiMenuDraw(const char* title, Page p);
void uiPlaceholderDraw(const char* title, const char* line);
void uiSystemInfoDraw();
void drawCurrentScreen();

// Legacy edit helpers still used by handleShortPress
void startThresholdEdit();
void saveThresholdEdit();
void cancelThresholdEdit();
void startOverrideEdit();
void saveOverrideEdit();
void cancelOverrideEdit();