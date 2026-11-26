#include <Arduino.h>
#include <Preferences.h>
#include <math.h>

#include "SmartTankState.h"
#include "TankSettings.h"

// ==========================================
// TANK SETTINGS + WATER LEVEL CALCULATION
// ==========================================
// Aggregate capacity assumes connected tanks have a common level.
static uint8_t configuredTankCount = 1;
static uint32_t persistedSettingsRevision = 0;
uint32_t tankSettingsRevision() { return persistedSettingsRevision; }
uint8_t tankConfiguredCount() { return configuredTankCount; }
void tankSetConfiguredCount(uint8_t count) {
  if (count < 1 || count > 8) return;
  configuredTankCount = count;
}


void loadTankSettings() {
  tankPrefs.begin("tank-config", false);
  const bool hasSavedThresholds = tankPrefs.isKey("auto-on") && tankPrefs.isKey("auto-off");
  persistedSettingsRevision = tankPrefs.getUInt("revision", 0);
  Serial.printf("[Tank NVS] Saved thresholds: %s, revision: %lu\n",
      hasSavedThresholds ? "present" : "missing (using defaults)",
      (unsigned long)persistedSettingsRevision);
  tankHeightCm = tankPrefs.getFloat("height", 150.0f);
  configuredTankCount = tankPrefs.getUChar("count", 1);
  if (configuredTankCount < 1 || configuredTankCount > 8) configuredTankCount = 1;
  tankCapacityLitres = tankPrefs.getFloat("capacity", 1000.0f);
  pumpAutoOnLevel = tankPrefs.getFloat("auto-on", 30.0f);
  pumpAutoOffLevel = tankPrefs.getFloat("auto-off", 95.0f);
  autoModeEnabled = tankPrefs.getBool("auto-en", false);
  manualOverrideMinutes = tankPrefs.getUInt("ov-min", 10);
  autoPendingOn = tankPrefs.getBool("auto-pend", false);

  // If the persisted "was filling" flag exists, we may want to resume
  // on boot. Clear any stale override-pending flag.
  if (tankPrefs.getBool("ovr-pend", false)) tankPrefs.putBool("ovr-pend", false);

  if (!isfinite(pumpAutoOnLevel) ||
      pumpAutoOnLevel < 0.0f || pumpAutoOnLevel > 99.0f) {
    pumpAutoOnLevel = 30.0f;
  }

  if (!isfinite(pumpAutoOffLevel) ||
      pumpAutoOffLevel < 1.0f || pumpAutoOffLevel > 100.0f) {
    pumpAutoOffLevel = 95.0f;
  }

  if (pumpAutoOffLevel <= pumpAutoOnLevel) {
    pumpAutoOnLevel = 30.0f;
    pumpAutoOffLevel = 95.0f;
  }

  Serial.printf("[Tank NVS] Loaded auto ON %.1f%%, OFF %.1f%%, override %lu min\n",
      pumpAutoOnLevel, pumpAutoOffLevel, (unsigned long)manualOverrideMinutes);
  draftAutoOnLevel = pumpAutoOnLevel;
  draftAutoOffLevel = pumpAutoOffLevel;

  if (manualOverrideMinutes < MIN_OVERRIDE_MINUTES ||
      manualOverrideMinutes > MAX_OVERRIDE_MINUTES) {
    manualOverrideMinutes = 10;
  }
  draftOverrideMinutes = manualOverrideMinutes;

  if (!isfinite(tankHeightCm) || tankHeightCm < MIN_TANK_HEIGHT_CM ||
      tankHeightCm > MAX_TANK_HEIGHT_CM) tankHeightCm = 150.0f;
  if (!isfinite(tankCapacityLitres) || tankCapacityLitres < 1.0f ||
      tankCapacityLitres > 100000.0f) tankCapacityLitres = 1000.0f;

  calibrationDraftHeightCm = tankHeightCm;
  Serial.printf("Tank calibration: %.1f cm, %.0f L\n",
                tankHeightCm, tankCapacityLitres);
  Serial.printf("Override timeout: %lu min\n",
                (unsigned long)manualOverrideMinutes);
  Serial.printf("Auto-pending (from NVS): %s\n",
                autoPendingOn ? "ON" : "OFF");

  // Arm boot resume if we were filling and auto is enabled.
  if (autoModeEnabled && autoPendingOn) {
    bootResumePending = true;
    bootResumeWaitUntil = millis() + BOOT_RESUME_SENSOR_WAIT_MS;
    Serial.println("Boot resume armed: awaiting valid sensor data");
  }
}

void saveTankSettings() {
  tankPrefs.putFloat("height", tankHeightCm);
  tankPrefs.putUChar("count", configuredTankCount);
  tankPrefs.putFloat("capacity", tankCapacityLitres);
  tankPrefs.putFloat("auto-on", pumpAutoOnLevel);
  tankPrefs.putFloat("auto-off", pumpAutoOffLevel);
  tankPrefs.putBool("auto-en", autoModeEnabled);
  tankPrefs.putUInt("ov-min", manualOverrideMinutes);
  // Persisted monotonic revision lets phone and TFT detect edits made on
  // the other interface instead of silently overwriting newer settings.
  const uint32_t nextRevision = persistedSettingsRevision + 1;
  if (tankPrefs.putUInt("revision", nextRevision) == sizeof(uint32_t))
    persistedSettingsRevision = nextRevision;
  else
    Serial.println("[Tank NVS] ERROR: settings revision could not be saved");
}

void calculateWaterLevel(float measuredDistanceCm) {
  if (!isfinite(measuredDistanceCm) ||
      measuredDistanceCm < MIN_SENSOR_DISTANCE_CM ||
      measuredDistanceCm > tankHeightCm) {
    waterLevelValid = false;
    return;
  }

  waterHeightCm = tankHeightCm - measuredDistanceCm;
  waterHeightCm = constrain(waterHeightCm, 0.0f, tankHeightCm);
  waterLevelPercent = (waterHeightCm / tankHeightCm) * 100.0f;
  waterLevelPercent = constrain(waterLevelPercent, 0.0f, 100.0f);
  waterVolumeLitres = (waterLevelPercent / 100.0f) * tankCapacityLitres;
  waterLevelValid = true;
}

void startTankHeightEdit() {
  calibrationDraftHeightCm = tankHeightCm;
  calibrationEditing = true;
  Serial.println("Tank height edit started");
}

void saveTankHeightEdit() {
  if (!isfinite(calibrationDraftHeightCm) ||
      calibrationDraftHeightCm < MIN_TANK_HEIGHT_CM ||
      calibrationDraftHeightCm > MAX_TANK_HEIGHT_CM) {
    Serial.println("Invalid tank height; not saved");
    calibrationEditing = false;
    return;
  }

  tankHeightCm = calibrationDraftHeightCm;
  saveTankSettings();
  calibrationEditing = false;
  if (sensorReadingValid && remoteConnected) calculateWaterLevel(distanceCm);
  Serial.printf("Tank height saved: %.1f cm\n", tankHeightCm);
}

void cancelTankHeightEdit() {
  calibrationDraftHeightCm = tankHeightCm;
  calibrationEditing = false;
  Serial.println("Tank height edit cancelled");
}

