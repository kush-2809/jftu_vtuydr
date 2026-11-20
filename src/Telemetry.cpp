#include "Telemetry.h"
#include <Arduino.h>
#include <math.h>

static unsigned long s_bootMs = 0;

void telemetryInit() { s_bootMs = millis(); }

void telemetryTick() {
  // CPU percentage is deliberately not guessed. Accurate usage requires
  // FreeRTOS run-time statistics to be enabled with a suitable timer source.
}

int8_t telemetryCpuPercent() { return -1; }
uint32_t telemetryHeapFreeKb() { return ESP.getFreeHeap() / 1024U; }
uint32_t telemetryHeapUsedKb() {
  const uint32_t total = ESP.getHeapSize();
  const uint32_t freeBytes = ESP.getFreeHeap();
  return total > freeBytes ? (total - freeBytes) / 1024U : 0;
}
uint32_t telemetryPsramFreeKb() {
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM)
  return ESP.getFreePsram() / 1024U;
#else
  return 0;
#endif
}
uint32_t telemetryPsramTotalKb() {
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM)
  return ESP.getPsramSize() / 1024U;
#else
  return 0;
#endif
}
float telemetryChipTempC() { return temperatureRead(); }
uint32_t telemetryUptimeSeconds() { return (millis() - s_bootMs) / 1000UL; }
uint32_t telemetryCpuFrequencyMHz() { return ESP.getCpuFreqMHz(); }

float telemetryAcVoltage() { return NAN; }
float telemetryAcCurrent() { return NAN; }
float telemetryEspVoltage() { return NAN; }
float telemetryEspCurrent() { return NAN; }
