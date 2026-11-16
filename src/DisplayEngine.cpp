#include "DisplayEngine.h"
#include "Scheduler.h"
#include <Preferences.h>
#include <Arduino.h>

extern void backlightSet(float brightness);

static Preferences prefs;
static uint16_t timeoutSec = 15;
static uint8_t  dimStep    = 10;
static unsigned long lastActivity = 0;
static uint8_t  currentBrightness = 100;
static bool     dimmed = false;
static unsigned long lastTickMs = 0;
static const uint8_t BRIGHTNESS_FLOOR = 40;

void displayEngineInit() {
  prefs.begin("display-config", false);
  timeoutSec = prefs.getUShort("tos", 15);
  dimStep    = prefs.getUChar ("dim", 10);
  prefs.end();
  if (dimStep < 10) dimStep = 10;
  if (dimStep > 20) dimStep = 20;
  lastActivity = millis();
  currentBrightness = 100;
  backlightSet(1.0f);
}

void displaySetTimeoutSeconds(uint16_t sec) {
  timeoutSec = sec;
  prefs.begin("display-config", false); prefs.putUShort("tos", sec); prefs.end();
}
uint16_t displayGetTimeoutSeconds() { return timeoutSec; }

void displaySetDimStep(uint8_t percent) {
  if (percent < 10) percent = 10;
  if (percent > 20) percent = 20;
  dimStep = percent;
  prefs.begin("display-config", false); prefs.putUChar("dim", percent); prefs.end();
}
uint8_t displayGetDimStep() { return dimStep; }

bool    displayIsDimmed()          { return dimmed; }
uint8_t displayBrightnessPercent() { return currentBrightness; }

void displayWake() {
  lastActivity = millis();
  if (currentBrightness < 100) {
    currentBrightness = 100;
    backlightSet(1.0f);
    dimmed = false;
  }
}
void displayMarkActivity() { lastActivity = millis(); }

void displayEngineTick() {
  unsigned long now = millis();
  if (now - lastTickMs < 500) return;
  lastTickMs = now;

  if (inSleepWindow()) {
    if (currentBrightness != BRIGHTNESS_FLOOR) {
      currentBrightness = BRIGHTNESS_FLOOR;
      backlightSet(BRIGHTNESS_FLOOR / 100.0f);
      dimmed = true;
    }
    lastActivity = now;
    return;
  }

  if (timeoutSec == 0) return;

  unsigned long idle = now - lastActivity;
  if (idle < (unsigned long)timeoutSec * 1000UL) return;

  if (currentBrightness > BRIGHTNESS_FLOOR) {
    uint8_t next = currentBrightness > dimStep
      ? currentBrightness - dimStep
      : BRIGHTNESS_FLOOR;
    if (next < BRIGHTNESS_FLOOR) next = BRIGHTNESS_FLOOR;
    currentBrightness = next;
    backlightSet(currentBrightness / 100.0f);
    dimmed = true;
    lastActivity = now;
  }
}