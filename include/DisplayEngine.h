#pragma once
#include <Arduino.h>

void displayEngineInit();
void displayEngineTick();
void displayWake();
void displayMarkActivity();

bool    displayIsDimmed();
uint8_t displayBrightnessPercent();

void     displaySetTimeoutSeconds(uint16_t sec);
uint16_t displayGetTimeoutSeconds();
void     displaySetDimStep(uint8_t percent);
uint8_t  displayGetDimStep();
void displayAdjustBrightness(int8_t direction);