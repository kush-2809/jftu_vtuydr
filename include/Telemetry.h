#pragma once
#include <Arduino.h>

void telemetryInit();
void telemetryTick();

uint8_t  telemetryCpuPercent();         // whole-system, 0..100
uint8_t  telemetryCpuCore0Percent();    // core 0 (PRO_CPU on S3)
uint8_t  telemetryCpuCore1Percent();    // core 1 (APP_CPU on S3)
uint32_t telemetryHeapFreeKb();
uint32_t telemetryHeapUsedKb();
float    telemetryChipTempC();
uint32_t telemetryUptimeSeconds();

// Placeholders
float telemetryAcVoltage();
float telemetryAcCurrent();
float telemetryEspVoltage();
float telemetryEspCurrent();