#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "DisplayConfig.h"
#include <LGFX_TFT_eSPI.hpp>

enum AutoPumpState {
  AUTO_IDLE,
  AUTO_FILLING,
  AUTO_RETRY_WAIT,
  AUTO_WAIT_SENSOR
};

// ==========================================
// SHARED APP STATE (definitions in StateDefinitions.cpp)
// ==========================================
extern Preferences tankPrefs;

extern float tankHeightCm;
extern float tankCapacityLitres;
extern float waterHeightCm;
extern float waterLevelPercent;
extern float waterVolumeLitres;
extern bool  waterLevelValid;

extern float pumpAutoOnLevel;
extern float pumpAutoOffLevel;
extern float draftAutoOnLevel;
extern float draftAutoOffLevel;
extern uint32_t manualOverrideMinutes;
extern uint32_t draftOverrideMinutes;

extern int lastTouchReading;
extern int stableTouchState;
extern unsigned long lastTouchChange;
extern bool manualOverrideActive;
extern unsigned long manualOverrideStartedAt;
extern bool autoModeEnabled;
extern bool fillingConfirmed;
extern float pumpStartLevel;
extern unsigned long pumpStartedAt;
extern unsigned long retryAt;
extern uint8_t failedAttempts;
extern AutoPumpState autoPumpState;
extern bool autoPendingOn;
extern bool bootResumePending;
extern unsigned long bootResumeWaitUntil;
extern bool calibrationEditing;
extern float calibrationDraftHeightCm;
extern bool sensorReadingValid;
extern bool remoteConnected;
extern float distanceCm;
extern bool manualPumpState;

extern const uint32_t MIN_OVERRIDE_MINUTES;
extern const uint32_t MAX_OVERRIDE_MINUTES;
extern const float    MIN_TANK_HEIGHT_CM;
extern const float    MAX_TANK_HEIGHT_CM;
extern const float    MIN_SENSOR_DISTANCE_CM;
extern const unsigned long BOOT_RESUME_SENSOR_WAIT_MS;

// ==========================================
// DISPLAY GLOBALS
// ==========================================
extern TFT_eSPI    tft;
extern TFT_eSprite canvas;
extern int  W, H;
extern bool displayReady;

// ==========================================
// ENCODER GLOBALS
// ==========================================
extern int  lastEncoderState;
extern int  encoderSteps;
extern bool longPressHandled;
extern int  lastButtonReading;
extern int  stableButtonState;
extern unsigned long lastDebounceTime;
extern unsigned long buttonDownAt;

// ==========================================
// LORA / SENSOR GLOBALS
// ==========================================
extern bool loraInitialized;
extern int  lastRSSI;
extern float lastSNR;
extern uint32_t packetsReceived;
extern uint32_t sensorErrors;
extern uint32_t invalidPackets;
extern unsigned long lastPacketMillis;
extern unsigned long lastDisplayUpdate;

// ==========================================
// LEGACY MENU (kept for input handlers)
// ==========================================
extern bool menuOpen;
extern int  currentPage;
extern int  menuIndex;

// ==========================================
// EDIT STATES
// ==========================================
extern bool thresholdEditing;
extern uint8_t thresholdEditStep;
extern bool overrideEditing;

// ==========================================
// SHARED FUNCTION PROTOTYPES
// ==========================================
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

int  getSignalLevel(int rssi);
uint16_t getSignalColor(int level);
void drawSignalBars(int x, int y, int level, uint16_t activeColor);
void drawWifiArc(int cx, int cy, int radius, uint16_t color);
void drawWifiIcon(int cx, int cy, int level);
void drawHeader();
void drawTankGraphic();

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

void uiMenuDraw(const char* title, int pageId);
void uiPlaceholderDraw(const char* title, const char* line);
void uiSystemInfoDraw();
void drawCurrentScreen();

void startThresholdEdit();
void saveThresholdEdit();
void cancelThresholdEdit();
void startOverrideEdit();
void saveOverrideEdit();
void cancelOverrideEdit();
// ==========================================
// SHARED CONSTANTS
// ==========================================
const unsigned long DISPLAY_INTERVAL = 150;
const unsigned long REMOTE_TIMEOUT   = 7000;