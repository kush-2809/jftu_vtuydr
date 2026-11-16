#include "Buzzer.h"
#include <Preferences.h>

#define BUZZER_PIN 21

static uint8_t g_vol = 3;
static Preferences p;

void buzzerInit() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(BUZZER_PIN, 2000, 8);
#else
  ledcSetup(1, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 1);
#endif
  p.begin("ui", false);
  g_vol = p.getUChar("buzz", 3);
  p.end();
  if (g_vol > 3) g_vol = 3;
}

void buzzerSetVolume(uint8_t v) {
  g_vol = (v > 3) ? 3 : v;
  p.begin("ui", false); p.putUChar("buzz", g_vol); p.end();
}
uint8_t buzzerVolume() { return g_vol; }

void buzzerBeep(uint16_t ms, uint8_t vol) {
  if (vol == 0 || ms == 0) return;
  int duty = (vol == 1) ? 40 : (vol == 2) ? 120 : 255;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(BUZZER_PIN, duty); delay(ms); ledcWrite(BUZZER_PIN, 0);
#else
  ledcWrite(1, duty);          delay(ms); ledcWrite(1, 0);
#endif
}