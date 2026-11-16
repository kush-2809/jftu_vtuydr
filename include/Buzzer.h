#pragma once
#include <Arduino.h>

void buzzerInit();
void buzzerSetVolume(uint8_t level);
uint8_t buzzerVolume();
void buzzerBeep(uint16_t ms, uint8_t vol = 3);