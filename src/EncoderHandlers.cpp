#include <Arduino.h>
#include "SmartTankState.h"
#include "UiState.h"
#include "MenuModel.h"
#include "PumpController.h"
#include "TankSettings.h"
#include "Buzzer.h"
#include "DisplayEngine.h"

static const float MIN_THRESHOLD_GAP = 1.0f;

void startThresholdEdit() {
  draftAutoOnLevel = pumpAutoOnLevel;
  draftAutoOffLevel = pumpAutoOffLevel;
  thresholdEditStep = 0;
  thresholdEditing = true;
}
void saveThresholdEdit() {
  if (thresholdEditStep == 0) { thresholdEditStep = 1; return; }
  if (draftAutoOffLevel <= draftAutoOnLevel ||
      (draftAutoOffLevel - draftAutoOnLevel) < MIN_THRESHOLD_GAP) return;
  pumpAutoOnLevel = draftAutoOnLevel;
  pumpAutoOffLevel = draftAutoOffLevel;
  saveTankSettings();
  thresholdEditing = false; thresholdEditStep = 0;
}
void cancelThresholdEdit() {
  draftAutoOnLevel = pumpAutoOnLevel;
  draftAutoOffLevel = pumpAutoOffLevel;
  thresholdEditing = false; thresholdEditStep = 0;
}
void startOverrideEdit() { draftOverrideMinutes = manualOverrideMinutes; overrideEditing = true; }
void saveOverrideEdit() {
  manualOverrideMinutes = draftOverrideMinutes;
  saveTankSettings(); overrideEditing = false;
}
void cancelOverrideEdit() { draftOverrideMinutes = manualOverrideMinutes; overrideEditing = false; }

void handleShortPress() {
  // Menu pages act immediately and must not leave a pending press for the
  // newly opened child page.
  if (uiInMenu()) {
    menuActivate(uiPage(), uiMenuIndex());
    return;
  }
  if (uiPage() == Page::SystemInfo) {
    uiAdminPress();
    return;
  }

  if (uiPage() == Page::Dashboard) {
    uiPush(Page::MainMenu);
    return;
  }
  uiInjectPress();

  if (currentPage == 4) {
    if (!calibrationEditing) startTankHeightEdit();
    else saveTankHeightEdit();
    return;
  }
  if (currentPage == 6) {
    if (!thresholdEditing) startThresholdEdit();
    else saveThresholdEdit();
    return;
  }
  if (currentPage == 7) {
    autoModeEnabled = !autoModeEnabled;
    stopManualOverride("auto mode toggled");
    drivePumpOutput(false);
    setAutoPendingOn(false);
    autoPumpState = AUTO_IDLE;
    failedAttempts = 0;
    fillingConfirmed = false;
    bootResumePending = false;
    saveTankSettings();
    return;
  }
  if (currentPage == 8) {
    if (!overrideEditing) startOverrideEdit();
    else saveOverrideEdit();
    return;
  }
}

void handleLongPress() {
  if (uiPage() == Page::Dashboard) {
    uiPush(Page::MainMenu);
    return;
  }
  uiInjectLongPress();
  // Parent menus and System Info do not have page tick handlers.
  if (uiInMenu() || uiPage() == Page::SystemInfo) {
    uiConsumeLongPress();
    uiPop();
  }
}

void handleEncoderRotation(int direction) {
      if (buzzerVolume() > 0) {
    buzzerBeep(18, buzzerVolume());
  }
  if (uiInMenu()) {
    const int n = menuItemCount(uiPage());
    if (n <= 0) return;
    int i = uiMenuIndex() + direction;
    if (i < 0) i = n - 1;
    if (i >= n) i = 0;
    uiSetMenuIndex(i);
    return;
  }
  if (uiPage() == Page::SystemInfo) {
    adminGestureFeed(direction);
    uiScrollBy(direction, 252 - (H - 24));
    return;
  }
 if (uiPage() == Page::Dashboard) {
  displayAdjustBrightness(direction);
  return;
}

  uiInjectRotation(direction);
  if (thresholdEditing && currentPage == 6) {
    if (thresholdEditStep == 0) {
      draftAutoOnLevel += direction;
      draftAutoOnLevel = constrain(draftAutoOnLevel, 0.0f, 99.0f);
    } else {
      draftAutoOffLevel += direction;
      draftAutoOffLevel = constrain(draftAutoOffLevel, 1.0f, 100.0f);
    }
    return;
  }
  if (overrideEditing && currentPage == 8) {
    if (direction > 0) {
      if (draftOverrideMinutes < MAX_OVERRIDE_MINUTES) draftOverrideMinutes++;
    } else if (draftOverrideMinutes > MIN_OVERRIDE_MINUTES) {
      draftOverrideMinutes--;
    }
    return;
  }
  if (calibrationEditing && currentPage == 4) {
    calibrationDraftHeightCm += (float)direction;
    calibrationDraftHeightCm = constrain(calibrationDraftHeightCm,
                                         MIN_TANK_HEIGHT_CM, MAX_TANK_HEIGHT_CM);
    return;
  }
}
