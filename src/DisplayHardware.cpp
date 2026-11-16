#include "SmartTankState.h"
#include "DisplayConfig.h"
#include <LGFX_TFT_eSPI.hpp>

#define BL_PIN 7
#define BL_ACTIVE_HIGH true
#define BL_FREQ 5000
#define BL_BITS 8
#define ROTATION 1

void backlightInit() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(BL_PIN, BL_FREQ, BL_BITS);
#else
  ledcSetup(0, BL_FREQ, BL_BITS);
  ledcAttachPin(BL_PIN, 0);
#endif
}

void backlightSet(float brightness) {
  if (brightness < 0.02f) brightness = 0.02f;
  if (brightness > 1.00f) brightness = 1.00f;
  int duty = (int)(255.0f * brightness * brightness);
  if (duty < 4) duty = 4;
  if (!BL_ACTIVE_HIGH) duty = 255 - duty;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(BL_PIN, duty);
#else
  ledcWrite(0, duty);
#endif
}

void initializeDisplay() {
  backlightInit();
  backlightSet(1.0f);
  tft.init();
  tft.setRotation(ROTATION);
  W = tft.width();
  H = tft.height();
  tft.fillScreen(TFT_BLACK);
  canvas.setColorDepth(16);
  if (canvas.createSprite(W, H) == nullptr) {
    Serial.println("ERROR: TFT sprite allocation failed!");
    tft.fillScreen(TFT_RED);
    return;
  }
  displayReady = true;
  Serial.printf("TFT initialized: %d x %d\n", W, H);
}