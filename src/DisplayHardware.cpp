#include "SmartTankState.h"
#include "DisplayConfig.h"
#include "DisplayOrientation.h"
#include <LGFX_TFT_eSPI.hpp>
#include <Preferences.h>

#define BL_PIN 7
#define BL_ACTIVE_HIGH true
#define BL_FREQ 5000
#define BL_BITS 8

void backlightInit() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(BL_PIN, BL_FREQ, BL_BITS);
#else
  ledcSetup(0, BL_FREQ, BL_BITS);
  ledcAttachPin(BL_PIN, 0);
#endif
}

void backlightSet(float brightness) {
  if (brightness < 0.0f) brightness = 0.0f;
  if (brightness > 1.00f) brightness = 1.00f;
  int duty = brightness <= 0.0f ? 0 : (int)(255.0f * brightness * brightness);
  if (brightness > 0.0f && duty < 4) duty = 4;
  if (!BL_ACTIVE_HIGH) duty = 255 - duty;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(BL_PIN, duty);
#else
  ledcWrite(0, duty);
#endif
}

static uint8_t activeRotation = 0;
void displayOrientationInit() {
  Preferences prefs; prefs.begin("display-config", true);
  activeRotation = prefs.getUChar("rot", 0); prefs.end();
  if (activeRotation > 3) activeRotation = 0;
}
uint8_t displayRotation() { return activeRotation; }
const char* displayRotationName(uint8_t rotation) {
  switch (rotation & 3) {
    case 0: return "Portrait 0";
    case 1: return "Landscape 90";
    case 2: return "Portrait 180";
    case 3: return "Landscape 270";
    default: return "Portrait 0";
  }
}
bool displaySetRotation(uint8_t rotation) {
  if (rotation > 3) return false;
  if (rotation == activeRotation) return true;
  const uint8_t previous = activeRotation;
  if (displayReady) {
    canvas.deleteSprite();
    tft.setRotation(rotation);
    const int nextW=tft.width(), nextH=tft.height();
    canvas.setColorDepth(16);
    if (canvas.createSprite(nextW,nextH)==nullptr) {
      Serial.println("ERROR: sprite resize failed; restoring previous rotation");
      tft.setRotation(previous); W=tft.width(); H=tft.height();
      if (canvas.createSprite(W,H)==nullptr) { displayReady=false; tft.fillScreen(TFT_RED); }
      return false;
    }
    W=nextW; H=nextH;
    canvas.fillSprite(TFT_BLACK); canvas.pushSprite(0,0);
  }
  activeRotation=rotation;
  Preferences prefs; prefs.begin("display-config",false);
  prefs.putUChar("rot",activeRotation); prefs.end();
  Serial.printf("Display orientation: %s (%d x %d)\\n",displayRotationName(activeRotation),W,H);
  return true;
}
void initializeDisplay() {
  backlightInit(); backlightSet(1.0f);
  displayOrientationInit();
  tft.init(); tft.setRotation(activeRotation);
  W=tft.width(); H=tft.height(); tft.fillScreen(TFT_BLACK);
  canvas.setColorDepth(16);
  if(canvas.createSprite(W,H)==nullptr){
    Serial.println("ERROR: TFT sprite allocation failed!");
    tft.fillScreen(TFT_RED); return;
  }
  displayReady=true;
  Serial.printf("TFT initialized: %d x %d (rotation %u)\\n",W,H,activeRotation);
}
