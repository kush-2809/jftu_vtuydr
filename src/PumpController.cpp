#include <Arduino.h>
#include <Preferences.h>
#include <math.h>

#include "SmartTankState.h"
#include "PumpController.h"

#define TOUCH_PIN 1
#define SSR_PIN 2
#define SSR_ACTIVE_HIGH true

static const unsigned long TOUCH_DEBOUNCE_MS = 50;
static const float MIN_CONFIRMED_RISE_PERCENT = 1.0f;
static const unsigned long NO_RISE_TIMEOUT_MS = 15UL * 60UL * 1000UL;
static const unsigned long MAX_PUMP_RUNTIME_MS = 60UL * 60UL * 1000UL;
static const unsigned long RETRY_INTERVALS_MS[] = {
  10UL * 60UL * 1000UL,
  15UL * 60UL * 1000UL,
  30UL * 60UL * 1000UL,
  60UL * 60UL * 1000UL,
  90UL * 60UL * 1000UL
};
static const uint8_t RETRY_INTERVAL_COUNT =
    sizeof(RETRY_INTERVALS_MS) / sizeof(RETRY_INTERVALS_MS[0]);

// ==========================================
// PUMP OUTPUT CONTROL
// ==========================================

// Low-level drive. Does NOT touch persisted state.
void drivePumpOutput(bool on) {
  manualPumpState = on;
  const int onLevel = SSR_ACTIVE_HIGH ? HIGH : LOW;
  const int offLevel = SSR_ACTIVE_HIGH ? LOW : HIGH;
  digitalWrite(SSR_PIN, on ? onLevel : offLevel);
  Serial.printf("PUMP GPIO OUTPUT: %s\n", on ? "ON" : "OFF");
}

// Persisted intent for auto mode. Call when the auto controller
// intentionally changes pump state.
void setAutoPendingOn(bool on) {
  if (autoPendingOn == on) return;
  autoPendingOn = on;
  tankPrefs.putBool("auto-pend", autoPendingOn);
  Serial.printf("AUTO-PEND persisted: %s\n", on ? "ON" : "OFF");
}

// Persisted intent for manual override. Call when override starts/stops.
void setOverridePendingOn(bool on) {
  tankPrefs.putBool("ovr-pend", on);
}

void initializeManualPump() {
  digitalWrite(SSR_PIN, LOW);
  pinMode(SSR_PIN, OUTPUT);
  drivePumpOutput(false);

  pinMode(TOUCH_PIN, INPUT);
  lastTouchReading = digitalRead(TOUCH_PIN);
  stableTouchState = lastTouchReading;
  lastTouchChange = millis();
  Serial.println("TTP223 ready: touch to start/cancel manual override");
}

// ==========================================
// MANUAL OVERRIDE
// ==========================================

void startManualOverride() {
  if (manualOverrideActive) return;

  // Absolute rule: if the auto controller is actively filling,
  // do not let a touch interfere. Pump is already on for a reason.
  if (autoPumpState == AUTO_FILLING) {
    Serial.println(
      "Manual touch ignored: auto filling is active"
    );
    return;
  }

  // Refuse to start override if pump output is already ON for any
  // other reason (defensive; auto filling is the only one).
  if (manualPumpState) {
    Serial.println(
      "Manual touch ignored: pump output already ON"
    );
    return;
  }

  manualOverrideActive = true;
  manualOverrideStartedAt = millis();
  drivePumpOutput(true);
  setOverridePendingOn(true);

  Serial.printf(
    "MANUAL OVERRIDE STARTED for %lu minutes\n",
    (unsigned long)manualOverrideMinutes
  );
}

void stopManualOverride(const char* reason) {
  if (!manualOverrideActive) return;

  manualOverrideActive = false;
  drivePumpOutput(false);
  setOverridePendingOn(false);

  Serial.printf("MANUAL OVERRIDE STOPPED: %s\n", reason);
}

void handleManualPumpTouch() {
  const int reading = digitalRead(TOUCH_PIN);
  const unsigned long now = millis();

  if (reading != lastTouchReading) {
    lastTouchChange = now;
    lastTouchReading = reading;
  }

  if (now - lastTouchChange >= TOUCH_DEBOUNCE_MS &&
      reading != stableTouchState) {
    stableTouchState = reading;

    if (stableTouchState == HIGH) {
      if (manualOverrideActive) {
        // Only override can cancel override. Auto filling is
        // never cancelled by touch because start is blocked
        // while filling, so manualOverrideActive is false then.
        stopManualOverride("user touch cancel");
      } else {
        startManualOverride();
      }
    }
  }
}


// ==========================================
// AUTOMATIC PUMP CONTROL FUNCTIONS
// ==========================================

unsigned long getRetryInterval() {
  uint8_t index = failedAttempts - 1;
  if (index >= RETRY_INTERVAL_COUNT) index = RETRY_INTERVAL_COUNT - 1;
  return RETRY_INTERVALS_MS[index];
}

void startAutomaticFilling(const char* reason) {
  pumpStartLevel = waterLevelPercent;
  pumpStartedAt = millis();
  fillingConfirmed = false;
  autoPumpState = AUTO_FILLING;
  drivePumpOutput(true);
  setAutoPendingOn(true);
  Serial.printf("AUTO FILL STARTED (%s) at %.1f%%\n",
                reason, waterLevelPercent);
}

void stopAutomaticFilling(const char* reason, bool scheduleRetry) {
  drivePumpOutput(false);
  setAutoPendingOn(false);
  Serial.printf("AUTO FILL STOPPED: %s\n", reason);
  fillingConfirmed = false;

  if (scheduleRetry) {
    failedAttempts++;
    retryAt = millis() + getRetryInterval();
    autoPumpState = AUTO_RETRY_WAIT;
    Serial.printf("Retry scheduled in %lu minutes\n",
                  getRetryInterval() / 60000UL);
  } else {
    failedAttempts = 0;
    autoPumpState = AUTO_IDLE;
  }
}

// Handle boot resume: decide once we have a valid reading or timeout.
void processBootResume() {
  if (!bootResumePending) return;

  const unsigned long now = millis();
  bool sensorAvailable =
      remoteConnected && sensorReadingValid && waterLevelValid;

  if (sensorAvailable) {
    bootResumePending = false;

    if (!autoModeEnabled) {
      setAutoPendingOn(false);
      return;
    }

    if (waterLevelPercent >= pumpAutoOffLevel) {
      Serial.println(
        "Boot resume skipped: tank already full"
      );
      setAutoPendingOn(false);
      return;
    }

    if (waterLevelPercent <= pumpAutoOnLevel) {
      Serial.printf(
        "Boot resume: level %.1f%% <= ON %.1f%%; resuming fill\n",
        waterLevelPercent, pumpAutoOnLevel
      );
      startAutomaticFilling("power-loss resume");
      return;
    }

    // Between ON and OFF: continue filling (we were filling before
    // power loss) until target reached, consistent with normal
    // filling behaviour which does not stop between thresholds.
    Serial.printf(
      "Boot resume: level %.1f%% between thresholds; resuming fill\n",
      waterLevelPercent
    );
    startAutomaticFilling("power-loss resume");
    return;
  }

  if ((long)(now - bootResumeWaitUntil) >= 0) {
    bootResumePending = false;
    Serial.println(
      "Boot resume aborted: no valid sensor data within window"
    );
    setAutoPendingOn(false);
  }
}

void updateAutomaticPumpController() {
  processBootResume();

  // Manual override has priority; pause auto controller while it runs.
  if (manualOverrideActive) {
    bool sensorAvailable =
        remoteConnected && sensorReadingValid && waterLevelValid;

    if (!sensorAvailable) {
      stopManualOverride("sensor lost during override");
      return;
    }

    if (millis() - manualOverrideStartedAt >=
        (unsigned long)manualOverrideMinutes * 60UL * 1000UL) {
      stopManualOverride("timeout reached");
    }
    return;
  }

  if (!autoModeEnabled) {
    autoPumpState = AUTO_IDLE;
    failedAttempts = 0;
    fillingConfirmed = false;
    return;
  }

  const unsigned long now = millis();

  bool sensorAvailable =
      remoteConnected && sensorReadingValid && waterLevelValid;

  if (!sensorAvailable) {
    if (autoPumpState == AUTO_FILLING) {
      drivePumpOutput(false);
      setAutoPendingOn(false);
      autoPumpState = AUTO_WAIT_SENSOR;
      Serial.println("SAFETY STOP: Sensor/LoRa data unavailable");
    } else if (autoPumpState == AUTO_IDLE) {
      autoPumpState = AUTO_WAIT_SENSOR;
    }
    return;
  }

  if (autoPumpState == AUTO_WAIT_SENSOR) {
    autoPumpState = AUTO_IDLE;
    Serial.println("Sensor restored; automatic controller ready");
  }

  switch (autoPumpState) {
    case AUTO_IDLE:
      if (waterLevelPercent <= pumpAutoOnLevel) {
        startAutomaticFilling("level <= ON threshold");
      }
      break;

    case AUTO_FILLING:
      if (waterLevelPercent >= pumpAutoOffLevel) {
        stopAutomaticFilling("Target water level reached", false);
        break;
      }
      if (now - pumpStartedAt >= MAX_PUMP_RUNTIME_MS) {
        stopAutomaticFilling("Maximum pump runtime exceeded", true);
        break;
      }
      if (!fillingConfirmed &&
          waterLevelPercent >= pumpStartLevel + MIN_CONFIRMED_RISE_PERCENT) {
        fillingConfirmed = true;
        Serial.println("Water rise confirmed; filling continues");
      }
      if (!fillingConfirmed &&
          now - pumpStartedAt >= NO_RISE_TIMEOUT_MS) {
        stopAutomaticFilling("Water level did not rise within 15 minutes", true);
      }
      break;

    case AUTO_RETRY_WAIT:
      if (waterLevelPercent > pumpAutoOnLevel) {
        failedAttempts = 0;
        autoPumpState = AUTO_IDLE;
        break;
      }
      if ((long)(now - retryAt) >= 0) {
        startAutomaticFilling("retry timer");
      }
      break;

    case AUTO_WAIT_SENSOR:
      break;
  }
}

