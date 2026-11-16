#include <Arduino.h>
#include "SmartTankState.h"
#include "UiState.h"
#include "MenuModel.h"
#include "PumpController.h"
#include "TankSettings.h"

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
  // Per-page tickers consume press first
  uiInjectPress();
  // SystemInfo admin gesture uses the press too
  if (uiPage() == Page::SystemInfo) { uiAdminPress(); return; }
  // Menu selection
  if (uiInMenu()) { menuActivate(uiPage(), uiMenuIndex()); return; }

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
    failedAttempts = 0; fillingConfirmed = false; bootResumePending = false;
    saveTankSettings();
    return;
  }
  if (currentPage == 8) {
    if (!overrideEditing) startOverrideEdit();
    else saveOverrideEdit();
    return;
  }
  uiPush(Page::MainMenu);
}

void handleLongPress() {
  uiInjectLongPress();
  if (uiPage() != Page::Dashboard) { uiPop(); return; }
  uiPush(Page::MainMenu);
}

void handleEncoderRotation(int direction) {
  uiInjectRotation(direction);
  if (uiPage() == Page::SystemInfo) adminGestureFeed(direction);
  if (uiInMenu()) {
    const int n = menuItemCount(uiPage());
    int i = uiMenuIndex() + direction;
    if (i < 0) i = n - 1;
    if (i >= n) i = 0;
    uiSetMenuIndex(i);
    return;
  }
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
    if (direction > 0) { if (draftOverrideMinutes < MAX_OVERRIDE_MINUTES) draftOverrideMinutes++; }
    else               { if (draftOverrideMinutes > MIN_OVERRIDE_MINUTES) draftOverrideMinutes--; }
    return;
  }
  if (calibrationEditing && currentPage == 4) {
    calibrationDraftHeightCm += (float)direction;
    calibrationDraftHeightCm = constrain(calibrationDraftHeightCm,
                                         MIN_TANK_HEIGHT_CM, MAX_TANK_HEIGHT_CM);
    return;
  }
}