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
#include "Management.h"
#include "FeatureControl.h"
#include "WeatherService.h"

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
    case Page::Dashboard:          drawDashboard(); return;
    case Page::MainMenu:           uiMenuDraw("MAIN MENU", 0); return;
    case Page::Alarm:              uiMenuDraw("ALARM", 0); return;
    case Page::Display:            uiMenuDraw("DISPLAY", 0); return;
    case Page::Layout:             uiMenuDraw("LAYOUT / THEME", 0); return;
    case Page::Network:            uiMenuDraw("NETWORK", 0); return;
    case Page::SystemClean:        uiMenuDraw("SYSTEM CLEAN", 0); return;
    case Page::Management:         uiMenuDraw("MANAGEMENT", 0); return;
    case Page::AdminConfig:
      if (!adminUnlocked()) { uiPlaceholderDraw("CONFIGURATION", "ADMIN ACCESS REQUIRED"); return; }
      uiMenuDraw("ADMIN CONFIG", 0); return;

    case Page::Telemetry:          uiTelemetryDraw(); return;
    case Page::SystemInfo:         uiSystemInfoDraw(); return;
    case Page::SystemRetry:        uiSystemRetryDraw(); return;

    case Page::AlarmTimer:         uiAlarmTimerDraw(); return;
    case Page::AlarmScheduler:     uiAlarmSchedulerDraw(); return;
    case Page::AlarmSleep:         uiAlarmSleepDraw(); return;
    case Page::DisplayTimeout:     uiDisplayTimeoutDraw(); return;
    case Page::DisplayDim:         uiDisplayDimDraw(); return;
    case Page::DisplayDateTime:    uiDisplayDateTimeDraw(); return;
    case Page::DisplayNTP:         uiDisplayNTPDraw(); return;

    case Page::ManagementMode:     uiManagementModeDraw(); return;
    case Page::ManagementBuzzer:   uiManagementBuzzerDraw(); return;
    case Page::ManagementRotation: uiManagementRotationDraw(); return;
    case Page::NetworkStatus:      uiNetworkStatusDraw(); return;
    case Page::NetworkWiFi:        uiNetworkWiFiDraw(); return;

    case Page::CleanReboot:
    case Page::CleanClearCache:
    case Page::CleanFactoryReset:  uiSystemCleanDraw(); return;

    case Page::AdminTankHeight:    uiAdminTankHeightDraw(); return;
    case Page::AdminTankCapacity:  uiAdminTankCapacityDraw(); return;
    case Page::AdminThresholds:    uiAdminThresholdsDraw(); return;

    default: uiPlaceholderDraw("PAGE", "NOT AVAILABLE"); return;
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
  featureControlInit();
  loadTankSettings();
  managementInit();
  initializeManualPump();
  buzzerBeep(80, buzzerVolume());
  initializeEncoder();
  initializeDisplay();
  initializeLoRa();
  timeServiceInit();
  schedulerInit();
  displayEngineInit();
  wifiManagerBegin();
  weatherServiceBegin();
  telemetryInit();
  drawCurrentScreen();
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  // Expired or logged-out portal admin sessions also lock physical config.
  if (!adminUnlocked() &&
      (uiPage() == Page::AdminConfig ||
       uiPage() == Page::AdminTankHeight ||
       uiPage() == Page::AdminTankCapacity ||
       uiPage() == Page::AdminThresholds)) {
    uiGoRoot();
  }
  updateEncoder();
  handleManualPumpTouch();

  if (loraInitialized) receiveSensorData();

  wifiManagerUpdate();
  wifiManagerHandlePortal();
  updateConnectionStatus();
  updateAutomaticPumpController();

  timeServiceTick();
  schedulerTick();
  telemetryTick();
  displayEngineTick();

  // Per-page tick hooks
  switch (uiPage()) {
    case Page::AlarmTimer:         uiAlarmTimerTick(); break;
    case Page::AlarmScheduler:     uiAlarmSchedulerTick(); break;
    case Page::AlarmSleep:         uiAlarmSleepTick(); break;
    case Page::DisplayTimeout:     uiDisplayTimeoutTick(); break;
    case Page::DisplayDim:         uiDisplayDimTick(); break;
    case Page::DisplayDateTime:    uiDisplayDateTimeTick(); break;
    case Page::DisplayNTP:         uiDisplayNTPTick(); break;
    case Page::Telemetry:          uiTelemetryTick(); break;
    case Page::ManagementMode:    uiManagementModeTick(); break;
    case Page::ManagementBuzzer:  uiManagementBuzzerTick(); break;
    case Page::ManagementRotation: uiManagementRotationTick(); break;
    case Page::NetworkStatus:     uiNetworkStatusTick(); break;
    case Page::NetworkWiFi:       uiNetworkWiFiTick(); break;
    case Page::CleanReboot:
    case Page::CleanClearCache:
    case Page::CleanFactoryReset: uiSystemCleanTick(); break;
    case Page::SystemRetry:       uiSystemRetryTick(); break;
    case Page::AdminTankHeight:   uiAdminTankHeightTick(); break;
    case Page::AdminTankCapacity: uiAdminTankCapacityTick(); break;
    case Page::AdminThresholds:   uiAdminThresholdsTick(); break;
    default: break;
  }

  unsigned long now = millis();
  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    drawCurrentScreen();
  }
}