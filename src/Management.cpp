#include "Management.h"
#include "SmartTankState.h"
#include "TankSettings.h"
#include <Preferences.h>

static Preferences modePrefs;
static ControlMode currentMode = CONTROL_ALARM_ONLY;

void managementInit() {
  modePrefs.begin("control-config", true);
  bool hasMode = modePrefs.isKey("mode");
  uint8_t saved = modePrefs.getUChar("mode", 0xFF);
  modePrefs.end();

  if (hasMode && saved <= CONTROL_ALL) {
    currentMode = static_cast<ControlMode>(saved);
  } else {
    // Backward compatibility with installations that used autoModeEnabled.
    currentMode = autoModeEnabled ? CONTROL_AUTO_MANUAL : CONTROL_ALARM_ONLY;
  }
  if (currentMode == CONTROL_ALARM_ONLY) autoModeEnabled = false;
  else if (!hasMode) autoModeEnabled = (currentMode != CONTROL_ALARM_ONLY);
}

ControlMode managementMode() { return currentMode; }

void managementSetMode(ControlMode mode) {
  if (mode > CONTROL_ALL) return;
  currentMode = mode;
  // A mode selection is an explicit user decision: enabling an automatic
  // capable mode must not inherit a stale false autoModeEnabled flag.
  autoModeEnabled = (mode == CONTROL_AUTO_ONLY ||
                     mode == CONTROL_AUTO_MANUAL ||
                     mode == CONTROL_ALL);
  modePrefs.begin("control-config", false);
  modePrefs.putUChar("mode", static_cast<uint8_t>(mode));
  modePrefs.end();
  saveTankSettings();
}

const char* managementModeName(ControlMode mode) {
  switch (mode) {
    case CONTROL_ALARM_ONLY: return "Alarm only";
    case CONTROL_AUTO_ONLY: return "Automatic only";
    case CONTROL_AUTO_MANUAL: return "Auto + manual";
    case CONTROL_ALL: return "All functions";
    default: return "Unknown";
  }
}

bool managementAllowsAlarm() {
  return currentMode == CONTROL_ALARM_ONLY || currentMode == CONTROL_ALL;
}

bool managementModeSupportsAuto() {
  return currentMode == CONTROL_AUTO_ONLY ||
         currentMode == CONTROL_AUTO_MANUAL ||
         currentMode == CONTROL_ALL;
}

bool managementAllowsAuto() {
  return managementModeSupportsAuto() && autoModeEnabled;
}

bool managementAllowsManual() {
  return currentMode == CONTROL_AUTO_MANUAL || currentMode == CONTROL_ALL;
}
