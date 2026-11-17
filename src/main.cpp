#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include "SmartTankState.h"
#include "DisplayConfig.h"
#include <LGFX_TFT_eSPI.hpp>
#include "Theme.h"
#include "UiState.h"
#include "MenuModel.h"
#include "Buzzer.h"
#include "WiFiManager.h"
#include "PumpController.h"
#include "TankSettings.h"
#include "TimeService.h"
#include "Scheduler.h"
#include "DisplayEngine.h"
#include "UiPages.h"
#include "Telemetry.h"

// All globals are in StateDefinitions.cpp
// All pages are in Page*.cpp / UiDrawHelpers.cpp / UiPages.cpp
// All input is in EncoderInput.cpp / EncoderHandlers.cpp
// All LoRa is in LoraInput.cpp
// All display HW is in DisplayHardware.cpp

// ==========================================
// DISPLAY ROUTER
// ==========================================
void drawCurrentScreen() {
  if (!displayReady) return;
  switch (uiPage()) {
    case Page::Dashboard:     drawDashboard(); return;
    case Page::MainMenu:      uiMenuDraw("MAIN MENU", 0); return;
    case Page::Alarm:         uiMenuDraw("ALARM", 0); return;
    case Page::Display:       uiMenuDraw("DISPLAY", 0); return;
    case Page::Layout:        uiMenuDraw("LAYOUT", 0); return;
    case Page::Network:       uiMenuDraw("NETWORK", 0); return;
    case Page::SystemClean:   uiMenuDraw("SYSTEM CLEAN", 0); return;
    case Page::SystemRetry:   uiMenuDraw("SYSTEM RETRY", 0); return;
   case Page::Telemetry:      uiTelemetryDraw();    return;
    case Page::SystemInfo:    uiSystemInfoDraw(); return;
    case Page::Management:    uiPlaceholderDraw("MANAGEMENT", "Mode + Buzzer"); return;
    case Page::AdminConfig:
      if (!adminUnlocked()) { uiPlaceholderDraw("CONFIG", "LOCKED"); return; }
      uiPlaceholderDraw("CONFIG", "Admin controls"); return;

    case Page::AlarmTimer:      uiAlarmTimerDraw(); return;
    case Page::AlarmScheduler:  uiAlarmSchedulerDraw(); return;
    case Page::AlarmSleep:      uiAlarmSleepDraw(); return;
    case Page::DisplayTimeout:  uiDisplayTimeoutDraw(); return;
    case Page::DisplayDim:      uiDisplayDimDraw(); return;
  
    case Page::DisplayNTP:      uiDisplayNTPDraw(); return;

    default: uiPlaceholderDraw("PAGE", "TODO"); return;
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n===== SMART TANK (S3) PHASE 6 =====");

  uiInit();
  {
    Preferences p;
    p.begin("ui", true);
    uint8_t t = p.getUChar("theme", 0);
    p.end();
    if (t < themeCount()) themeLoad(t);
  }
  buzzerInit();
  buzzerBeep(80, buzzerVolume());

  initializeManualPump();
  loadTankSettings();
  initializeEncoder();
  initializeDisplay();
  initializeLoRa();
  timeServiceInit();
  schedulerInit();
  displayEngineInit();
  wifiManagerBegin();
telemetryInit();
  drawCurrentScreen();
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  updateEncoder();
  handleManualPumpTouch();

  if (loraInitialized) receiveSensorData();

  wifiManagerUpdate();
  wifiManagerHandlePortal();
  updateConnectionStatus();
  updateAutomaticPumpController();

  timeServiceTick();
  schedulerTick();
  displayEngineTick();

  // Per-page tick hooks
  switch (uiPage()) {
    case Page::AlarmTimer:      uiAlarmTimerTick();      break;
    case Page::AlarmScheduler:  uiAlarmSchedulerTick();  break;
    case Page::AlarmSleep:      uiAlarmSleepTick();      break;
    case Page::DisplayTimeout:  uiDisplayTimeoutTick();  break;
    case Page::DisplayDim:      uiDisplayDimTick();      break;
   case Page::Telemetry:       uiTelemetryTick();       break;
case Page::DisplayDateTime: uiDisplayDateTimeTick(); break;
    case Page::DisplayNTP:      uiDisplayNTPTick();      break;
    default: break;
  }

  unsigned long now = millis();
  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    drawCurrentScreen();
  }
}