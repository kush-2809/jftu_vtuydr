#include <Arduino.h>
#include <Preferences.h>
#include "SmartTankState.h"

TFT_eSPI    tft;
TFT_eSprite canvas(&tft);
int  W = 0, H = 0;
bool displayReady = false;

int  lastEncoderState = 0;
int  encoderSteps = 0;
bool longPressHandled = false;
int  lastButtonReading = HIGH;
int  stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonDownAt = 0;

float distanceCm = 0.0f;
bool  sensorReadingValid = false;
bool  remoteConnected = false;
int   lastRSSI = 0;
float lastSNR = 0.0f;
uint32_t packetsReceived = 0;
uint32_t sensorErrors = 0;
uint32_t invalidPackets = 0;
unsigned long lastPacketMillis = 0;
unsigned long lastDisplayUpdate = 0;
bool loraInitialized = false;

Preferences tankPrefs;
float tankHeightCm = 150.0f;
float tankCapacityLitres = 1000.0f;
float waterHeightCm = 0.0f;
float waterLevelPercent = 0.0f;
float waterVolumeLitres = 0.0f;
bool  waterLevelValid = false;

float pumpAutoOnLevel = 30.0f;
float pumpAutoOffLevel = 95.0f;
float draftAutoOnLevel = 30.0f;
float draftAutoOffLevel = 95.0f;
bool  thresholdEditing = false;
uint8_t thresholdEditStep = 0;

uint32_t manualOverrideMinutes = 10;
bool  overrideEditing = false;
uint32_t draftOverrideMinutes = 10;
bool  manualOverrideActive = false;
unsigned long manualOverrideStartedAt = 0;

AutoPumpState autoPumpState = AUTO_IDLE;
bool  autoModeEnabled = false;
bool  fillingConfirmed = false;
float pumpStartLevel = 0.0f;
unsigned long pumpStartedAt = 0;
unsigned long retryAt = 0;
uint8_t failedAttempts = 0;

bool  autoPendingOn = false;
bool  bootResumePending = false;
unsigned long bootResumeWaitUntil = 0;

bool  calibrationEditing = false;
float calibrationDraftHeightCm = 150.0f;

bool  manualPumpState = false;
int   lastTouchReading = LOW;
int   stableTouchState = LOW;
unsigned long lastTouchChange = 0;

bool menuOpen = false;
int  currentPage = 0;
int  menuIndex = 0;

extern const uint32_t MIN_OVERRIDE_MINUTES = 1;
extern const uint32_t MAX_OVERRIDE_MINUTES = 120;
extern const float    MIN_TANK_HEIGHT_CM   = 20.0f;
extern const float    MAX_TANK_HEIGHT_CM   = 500.0f;
extern const float    MIN_SENSOR_DISTANCE_CM = 5.0f;
extern const unsigned long BOOT_RESUME_SENSOR_WAIT_MS = 30UL * 1000UL;