#pragma once
#include <Arduino.h>

void telemetryInit();
void telemetryTick();

int8_t   telemetryCpuPercent();
uint32_t telemetryHeapFreeKb();
uint32_t telemetryHeapUsedKb();
uint32_t telemetryPsramFreeKb();
uint32_t telemetryPsramTotalKb();
float    telemetryChipTempC();
uint32_t telemetryUptimeSeconds();
uint32_t telemetryCpuFrequencyMHz();

// These return NAN until suitable external measurement hardware is installed.
float telemetryAcVoltage();
float telemetryAcCurrent();
float telemetryEspVoltage();
float telemetryEspCurrent();
