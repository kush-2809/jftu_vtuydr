#include <Arduino.h>
#include "SmartTankState.h"
#include "UiState.h"
#include "DisplayEngine.h"

#define ENC_CLK 4
#define ENC_DT  5
#define ENC_SW  6
#define BUTTON_DEBOUNCE 35
#define LONG_PRESS_TIME 700

static const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void initializeEncoder() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);
  int clk = digitalRead(ENC_CLK);
  int dt  = digitalRead(ENC_DT);
  lastEncoderState = (clk << 1) | dt;
  Serial.println("Rotary encoder initialized");
}

void updateEncoder() {
  int clk = digitalRead(ENC_CLK);
  int dt  = digitalRead(ENC_DT);
  int currentState = (clk << 1) | dt;
  int index = (lastEncoderState << 2) | currentState;
  encoderSteps += encoderTable[index];
  lastEncoderState = currentState;

  if (encoderSteps >= 4)  { encoderSteps = 0; displayWake(); handleEncoderRotation( 1); }
  if (encoderSteps <= -4) { encoderSteps = 0; displayWake(); handleEncoderRotation(-1); }

  unsigned long now = millis();
  int reading = digitalRead(ENC_SW);
  if (reading != lastButtonReading) { lastDebounceTime = now; lastButtonReading = reading; }

  if (now - lastDebounceTime >= BUTTON_DEBOUNCE) {
    if (reading != stableButtonState) {
      stableButtonState = reading;
      displayWake();
      if (stableButtonState == LOW) { buttonDownAt = now; longPressHandled = false; }
      else if (!longPressHandled)   { handleShortPress(); }
    }
  }

  if (stableButtonState == LOW && !longPressHandled &&
      now - buttonDownAt >= LONG_PRESS_TIME) {
    longPressHandled = true;
    handleLongPress();
  }
}