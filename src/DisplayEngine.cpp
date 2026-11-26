#include "DisplayEngine.h"
#include "Scheduler.h"
#include "UiState.h"
#include <Preferences.h>
#include <Arduino.h>

extern void backlightSet(float brightness);

static Preferences prefs;
static uint16_t timeoutSec = 15;
static uint8_t dimStep = 10;
static unsigned long lastActivity = 0;
static uint8_t currentBrightness = 100;
static bool dimmed = false;
static bool sleepBacklightOff = false;
static unsigned long lastTickMs = 0;
static const uint8_t BRIGHTNESS_FLOOR = 40;

void displayEngineInit() {
  prefs.begin("display-config", false);
  timeoutSec = prefs.getUShort("tos", 15);
  dimStep = prefs.getUChar("dim", 10);
  prefs.end();
  if (dimStep < 10) dimStep = 10;
  if (dimStep > 20) dimStep = 20;
  if (timeoutSec > 600) timeoutSec = 600;
  lastActivity = millis();
  currentBrightness = 100;
  dimmed = false;
  sleepBacklightOff = false;
  backlightSet(1.0f);
}

void displaySetTimeoutSeconds(uint16_t sec) {
  timeoutSec = sec > 600 ? 600 : sec;
  prefs.begin("display-config", false);
  prefs.putUShort("tos", timeoutSec);
  prefs.end();
  if (timeoutSec == 0) displayWake();
}
uint16_t displayGetTimeoutSeconds() { return timeoutSec; }

void displaySetDimStep(uint8_t percent) {
  if (percent < 10) percent = 10;
  if (percent > 20) percent = 20;
  dimStep = percent;
  prefs.begin("display-config", false);
  prefs.putUChar("dim", percent);
  prefs.end();
}
uint8_t displayGetDimStep() { return dimStep; }
bool displayIsDimmed() { return dimmed; }
uint8_t displayBrightnessPercent() { return currentBrightness; }
void displayAdjustBrightness(int8_t direction) {
  int next = currentBrightness + (direction * 10);

  if (next < BRIGHTNESS_FLOOR) next = BRIGHTNESS_FLOOR;
  if (next > 100) next = 100;

  currentBrightness = next;

  dimmed = currentBrightness < 100;
  sleepBacklightOff = false;

  lastActivity = millis();

  backlightSet(currentBrightness / 100.0f);
}

void displayWake() {
  lastActivity = millis();
  currentBrightness = 100;
  dimmed = false;
  sleepBacklightOff = false;
  backlightSet(1.0f);
}
void displayMarkActivity() { lastActivity = millis(); }

void displayEngineTick() {
  const unsigned long now = millis();
  if (now - lastTickMs < 250) return;
  lastTickMs = now;

  // The sleep window powers down only the TFT backlight. Both ESP32 nodes
  // continue running their sensing, scheduling and pump safety tasks.
  if (inSleepWindow()) {
    if (!sleepBacklightOff) {
      currentBrightness = 0;
      dimmed = true;
      sleepBacklightOff = true;
      backlightSet(0.0f);
    }
    return;
  }

  if (sleepBacklightOff) {
    sleepBacklightOff = false;
    currentBrightness = 100;
    dimmed = false;
    lastActivity = now;
    backlightSet(1.0f);
    return;
  }

  if (timeoutSec == 0) {
    if (currentBrightness < 100) {
      currentBrightness = 100;
      dimmed = false;
      backlightSet(1.0f);
    }
    return;
  }

  if (now - lastActivity < (unsigned long)timeoutSec * 1000UL) return;
  // Always show the live dashboard before inactivity dimming.
  if (uiPage() != Page::Dashboard) uiGoRoot();
  if (currentBrightness > BRIGHTNESS_FLOOR) {
    uint8_t next = currentBrightness > dimStep ? currentBrightness - dimStep : BRIGHTNESS_FLOOR;
    if (next < BRIGHTNESS_FLOOR) next = BRIGHTNESS_FLOOR;
    currentBrightness = next;
    backlightSet(currentBrightness / 100.0f);
    dimmed = currentBrightness < 100;
    lastActivity = now;
  }
}
