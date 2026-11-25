#pragma once
#include <Arduino.h>
struct WeatherSnapshot {
  bool valid = false;
  float temperatureC = 0;
  int weatherCode = -1;
  uint32_t updatedAtMs = 0;
  char city[40] = {};
};
void weatherServiceBegin();
WeatherSnapshot weatherServiceSnapshot();
const char* weatherConditionLabel(int weatherCode);
