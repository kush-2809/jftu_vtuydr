#pragma once
#include <Arduino.h>
struct WeatherSnapshot {
  bool valid = false;
  float temperatureC = 0;
  int weatherCode = -1;
  uint32_t updatedAtMs = 0;
  char city[40] = {};
  char status[40] = "Waiting for location";
};
void weatherServiceBegin();
WeatherSnapshot weatherServiceSnapshot();
const char* weatherConditionLabel(int weatherCode);

// Portal browser may pass validated weather when ESP Wi-Fi has no internet.
bool weatherServiceAcceptPortal(float latitude, float longitude,
                                float temperatureC, int weatherCode);
