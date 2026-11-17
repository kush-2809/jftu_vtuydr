#include "Telemetry.h"
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint8_t   s_cpuPercent = 0;
static unsigned long s_bootMs = 0;

void telemetryInit() {
  s_bootMs = millis();
  s_cpuPercent = 0;
}

void telemetryTick() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < 500) return;
  last = now;

  // Yield probe: measure how long vTaskDelay(0) actually takes.
  // Idle CPU -> returns in <1ms. Busy CPU -> returns later.
  uint32_t t0 = micros();
  vTaskDelay(0);
  uint32_t delta = micros() - t0;

  // Normalize: 0-500us = low load, 5000us+ = high load
  uint8_t inst = 0;
  if (delta >= 5000)      inst = 90;
  else if (delta >= 2000) inst = 60;
  else if (delta >= 1000) inst = 40;
  else if (delta >= 500)  inst = 20;
  else                    inst = 5;

  // Low-pass filter
  s_cpuPercent = (s_cpuPercent * 3 + inst) / 4;
}

uint8_t  telemetryCpuPercent()      { return s_cpuPercent; }
uint8_t  telemetryCpuCore0Percent() { return s_cpuPercent; }  // same (can't split without trace)
uint8_t  telemetryCpuCore1Percent() { return s_cpuPercent; }

uint32_t telemetryHeapFreeKb()    { return ESP.getFreeHeap() / 1024; }
uint32_t telemetryHeapUsedKb()    { return (ESP.getHeapSize() - ESP.getFreeHeap()) / 1024; }
float    telemetryChipTempC()     { return temperatureRead(); }
uint32_t telemetryUptimeSeconds() { return (millis() - s_bootMs) / 1000UL; }

float telemetryAcVoltage()  { return NAN; }
float telemetryAcCurrent()  { return NAN; }
float telemetryEspVoltage() { return NAN; }
float telemetryEspCurrent() { return NAN; }