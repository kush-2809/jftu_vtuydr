#pragma once
#include <Arduino.h>

enum ControlMode : uint8_t {
  CONTROL_ALARM_ONLY = 0,
  CONTROL_AUTO_ONLY,
  CONTROL_AUTO_MANUAL,
  CONTROL_ALL
};

void managementInit();
ControlMode managementMode();
void managementSetMode(ControlMode mode);
const char* managementModeName(ControlMode mode);
bool managementAllowsAlarm();
bool managementModeSupportsAuto();
bool managementAllowsAuto();
bool managementAllowsManual();
