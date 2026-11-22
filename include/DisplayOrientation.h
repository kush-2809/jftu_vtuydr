#pragma once
#include <stdint.h>
void displayOrientationInit();
uint8_t displayRotation();
const char* displayRotationName(uint8_t rotation);
bool displaySetRotation(uint8_t rotation);
