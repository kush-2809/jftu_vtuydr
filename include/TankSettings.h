#pragma once
#include <stdint.h>

// Number of hydraulically linked tanks represented by the ONE level sensor.
// Configured capacity is the aggregate capacity of all tanks.
uint8_t tankConfiguredCount();
// Persisted revision increases on each saved configuration change.
uint32_t tankSettingsRevision();
void tankSetConfiguredCount(uint8_t count);
void loadTankSettings();
void saveTankSettings();
void calculateWaterLevel(float measuredDistanceCm);
void startTankHeightEdit();
void saveTankHeightEdit();
void cancelTankHeightEdit();
