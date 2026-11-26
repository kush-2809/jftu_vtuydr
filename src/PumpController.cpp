#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <time.h>

#include "SmartTankState.h"
#include "PumpController.h"
#include "Management.h"
#include "FeatureControl.h"
#include "TimeService.h"
#include "Buzzer.h"

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
static const unsigned long RETRY_WARNING_BEEP_MS = 10UL * 60UL * 1000UL;

static bool alarmFillActive = false;
static float manualStartLevel = 0.0f;
static bool manualRiseConfirmed = false;
static Preferences retryPrefs;
static bool manualRetryEnabled = false;
static bool manualRetryAttemptActive = false;
static uint8_t manualRetryHour = 18;
static uint8_t manualRetryMinute = 0;
static time_t manualRetryTarget = 0;
static time_t retryTargetEpoch = 0;
static bool retryWarningWasActive = false;
static unsigned long lastRetryWarningBeep = 0;

static time_t calculateNextRetryTarget() {
  if (!timeIsValid()) return 0;
  time_t now = nowLocal();
  struct tm local;
  localtime_r(&now, &local);
  local.tm_hour = manualRetryHour;
  local.tm_min = manualRetryMinute;
  local.tm_sec = 0;
  time_t target = mktime(&local);
  if (target <= now) {
    local.tm_mday += 1;
    target = mktime(&local);
  }
  return target;
}

static bool sensorIsAvailable() {
  return remoteConnected && sensorReadingValid && waterLevelValid;
}

static void persistRetryRuntime() {
  retryPrefs.begin("retry-config", false);
  retryPrefs.putUChar("fails", failedAttempts);
  retryPrefs.putBool("alarmFill", alarmFillActive);
  retryPrefs.putULong64("retryEp", (uint64_t)retryTargetEpoch);
  retryPrefs.end();
}

static void clearRetryRuntime() {
  failedAttempts = 0;
  alarmFillActive = false;
  retryTargetEpoch = 0;
  retryAt = 0;
  persistRetryRuntime();
}

// ==========================================
// PUMP OUTPUT CONTROL
// ==========================================
void drivePumpOutput(bool on) {
  // The SSR feature interlock always wins. A disabled SSR can never be
  // energised by a manual, scheduled, or automatic request.
  if (on && !featureSsrEnabled()) on = false;
  manualPumpState = on;
  const int onLevel = SSR_ACTIVE_HIGH ? HIGH : LOW;
  const int offLevel = SSR_ACTIVE_HIGH ? LOW : HIGH;
  digitalWrite(SSR_PIN, on ? onLevel : offLevel);
  Serial.printf("PUMP GPIO OUTPUT: %s\n", on ? "ON" : "OFF");
}

void setAutoPendingOn(bool on) {
  if (autoPendingOn == on) return;
  autoPendingOn = on;
  tankPrefs.putBool("auto-pend", autoPendingOn);
  Serial.printf("AUTO-PEND persisted: %s\n", on ? "ON" : "OFF");
}

void setOverridePendingOn(bool on) {
  tankPrefs.putBool("ovr-pend", on);
}

void initializeManualPump() {
  pinMode(SSR_PIN, OUTPUT);
  drivePumpOutput(false);

  pinMode(TOUCH_PIN, INPUT);
  lastTouchReading = digitalRead(TOUCH_PIN);
  stableTouchState = lastTouchReading;
  lastTouchChange = millis();
  Serial.println("TTP223 ready");

  retryPrefs.begin("retry-config", true);
  manualRetryEnabled = retryPrefs.getBool("timeEn", false);
  manualRetryHour = retryPrefs.getUChar("timeH", 18);
  manualRetryMinute = retryPrefs.getUChar("timeM", 0);
  failedAttempts = retryPrefs.getUChar("fails", 0);
  alarmFillActive = retryPrefs.getBool("alarmFill", false);
  retryTargetEpoch = (time_t)retryPrefs.getULong64("retryEp", 0);
  retryPrefs.end();
  if (manualRetryHour > 23) manualRetryHour = 18;
  if (manualRetryMinute > 59) manualRetryMinute = 0;
  if (failedAttempts > 0) {
    // millis() resets on reboot. If the stored absolute deadline cannot yet
    // be checked (no valid RTC/NTP clock), restart one full interval safely.
    retryAt = millis() + getRetryInterval();
    autoPumpState = AUTO_RETRY_WAIT;
  }
  manualRetryTarget = 0;
}

// ==========================================
// MANUAL OVERRIDE
// ==========================================
uint32_t manualOverrideRemainingSeconds() {
  if (!manualOverrideActive || !manualPumpState) return 0;
  const uint32_t durationMs = (uint32_t)manualOverrideMinutes * 60000UL;
  const uint32_t elapsedMs = (uint32_t)(millis() - manualOverrideStartedAt);
  if (elapsedMs >= durationMs) return 0;
  // Round upward so the visible countdown never shows 00:00 while running.
  return (durationMs - elapsedMs + 999UL) / 1000UL;
}
void startManualOverride() {
  if (!managementAllowsManual()) return;
  // featureTouchEnabled gates only the TTP223 input path. Manual control
  // requested from the web portal remains available when touch is disabled.
  if (manualOverrideActive || !sensorIsAvailable()) return;
  if (waterLevelPercent >= pumpAutoOffLevel) return;
  if (autoPumpState == AUTO_FILLING || autoPumpState == AUTO_RETRY_WAIT || manualPumpState) return;
  if (!featureSsrEnabled()) return;

  manualOverrideActive = true;
  manualOverrideStartedAt = millis();
  manualStartLevel = waterLevelPercent;
  manualRiseConfirmed = false;
  drivePumpOutput(true);
  setOverridePendingOn(true);
  Serial.printf("MANUAL OVERRIDE STARTED for %lu minutes\n",
                (unsigned long)manualOverrideMinutes);
}

void stopManualOverride(const char* reason) {
  if (!manualOverrideActive) return;
  manualOverrideActive = false;
  drivePumpOutput(false);
  setOverridePendingOn(false);
  Serial.printf("MANUAL OVERRIDE STOPPED: %s\n", reason ? reason : "unspecified");
}

void handleManualPumpTouch() {
  if (!featureTouchEnabled() || !managementAllowsManual()) return;
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
      if (manualOverrideActive) stopManualOverride("user touch cancel");
      else startManualOverride();
    }
  }
}

// ==========================================
// RETRY POLICY
// ==========================================
unsigned long getRetryInterval() {
  if (failedAttempts == 0) return RETRY_INTERVALS_MS[0];
  uint8_t index = failedAttempts - 1;
  if (index >= RETRY_INTERVAL_COUNT) index = RETRY_INTERVAL_COUNT - 1;
  return RETRY_INTERVALS_MS[index];
}

bool alarmFillSessionActive() { return alarmFillActive; }
uint8_t pumpFailedAttemptCount() { return failedAttempts; }
bool pumpRetryManualTimeEnabled() { return manualRetryEnabled; }
uint8_t pumpRetryManualHour() { return manualRetryHour; }
uint8_t pumpRetryManualMinute() { return manualRetryMinute; }

bool pumpRetryWarningActive() {
  // Warn after the first failed attempt, including scheduled manual retries.
  return failedAttempts > 0 && autoPumpState == AUTO_RETRY_WAIT;
}

uint32_t pumpRetryRemainingSeconds() {
  if (manualRetryEnabled) {
    if (!timeIsValid()) return 0;
    if (manualRetryTarget == 0) manualRetryTarget = calculateNextRetryTarget();
    time_t now = nowLocal();
    return manualRetryTarget > now ? (uint32_t)(manualRetryTarget - now) : 0;
  }
  if (autoPumpState != AUTO_RETRY_WAIT) return 0;
  if (retryTargetEpoch > 0 && timeIsValid()) {
    const time_t now = nowLocal();
    return retryTargetEpoch > now ? (uint32_t)(retryTargetEpoch - now) : 0;
  }
  const long remaining = (long)(retryAt - millis());
  return remaining > 0 ? (uint32_t)remaining / 1000UL : 0;
}

void pumpRetrySetManualTime(uint8_t hour, uint8_t minute) {
  if (hour > 23 || minute > 59) return;
  manualRetryHour = hour;
  manualRetryMinute = minute;
  manualRetryEnabled = true;
  manualRetryAttemptActive = false;
  manualRetryTarget = 0;
  retryTargetEpoch = 0;
  retryPrefs.begin("retry-config", false);
  retryPrefs.putBool("timeEn", true);
  retryPrefs.putUChar("timeH", hour);
  retryPrefs.putUChar("timeM", minute);
  retryPrefs.end();
  persistRetryRuntime();
  Serial.printf("Manual retry scheduled for %02u:%02u\n", hour, minute);
}

void pumpRetryClearManualTime() {
  manualRetryEnabled = false;
  manualRetryAttemptActive = false;
  manualRetryTarget = 0;
  if (autoPumpState == AUTO_RETRY_WAIT && failedAttempts > 0) {
    const unsigned long interval = getRetryInterval();
    retryAt = millis() + interval;
    retryTargetEpoch = timeIsValid()
        ? nowLocal() + (time_t)(interval / 1000UL) : 0;
  } else {
    retryAt = 0;
    retryTargetEpoch = 0;
  }
  retryPrefs.begin("retry-config", false);
  retryPrefs.putBool("timeEn", false);
  retryPrefs.end();
  persistRetryRuntime();
}

// ==========================================
// FILL SESSION CONTROL
// ==========================================
void startAutomaticFilling(const char* reason) {
  if (!sensorIsAvailable() || !featureSsrEnabled() ||
      waterLevelPercent >= pumpAutoOffLevel) return;
  pumpStartLevel = waterLevelPercent;
  pumpStartedAt = millis();
  fillingConfirmed = false;
  autoPumpState = AUTO_FILLING;
  drivePumpOutput(true);
  setAutoPendingOn(true);
  Serial.printf("FILL STARTED (%s) at %.1f%%\n",
                reason ? reason : "unspecified", waterLevelPercent);
}

bool startAlarmFill(const char* reason) {
  if (!managementAllowsAlarm() || !sensorIsAvailable() ||
      waterLevelPercent >= pumpAutoOffLevel ||
      !featureSsrEnabled() || manualOverrideActive || manualPumpState ||
      autoPumpState == AUTO_FILLING || autoPumpState == AUTO_RETRY_WAIT) {
    return false;
  }
  alarmFillActive = true;
  startAutomaticFilling(reason ? reason : "alarm request");
  if (autoPumpState != AUTO_FILLING || !manualPumpState) {
    alarmFillActive = false;
    return false;
  }
  return true;
}

void stopAutomaticFilling(const char* reason, bool scheduleRetry) {
  drivePumpOutput(false);
  setAutoPendingOn(false);
  Serial.printf("FILL STOPPED: %s\n", reason ? reason : "unspecified");
  fillingConfirmed = false;

  if (scheduleRetry) {
    manualRetryAttemptActive = false;
    if (failedAttempts < 250) ++failedAttempts;
    const unsigned long interval = getRetryInterval();
    retryAt = millis() + interval;
    retryTargetEpoch = timeIsValid()
        ? nowLocal() + (time_t)(interval / 1000UL) : 0;
    autoPumpState = AUTO_RETRY_WAIT;
    persistRetryRuntime();
    Serial.printf("Retry scheduled in %lu minutes (attempt %u)\n",
                  interval / 60000UL, failedAttempts);
  } else {
    clearRetryRuntime();
    autoPumpState = AUTO_IDLE;
    manualRetryAttemptActive = false;
    pumpRetryClearManualTime();
  }
}

void processBootResume() {
  if (!bootResumePending) return;
  // A persisted retry sequence takes precedence over generic power-loss
  // resume. Otherwise a reset during a retry could bypass its cooldown.
  if (autoPumpState == AUTO_RETRY_WAIT) {
    bootResumePending = false;
    setAutoPendingOn(false);
    return;
  }
  const unsigned long now = millis();
  if (!sensorIsAvailable()) {
    if ((long)(now - bootResumeWaitUntil) >= 0) {
      bootResumePending = false;
      setAutoPendingOn(false);
      Serial.println("Boot resume aborted: no valid sensor data");
    }
    return;
  }
  bootResumePending = false;
  if (!managementAllowsAuto() || waterLevelPercent >= pumpAutoOffLevel) {
    setAutoPendingOn(false);
    return;
  }
  startAutomaticFilling("power-loss resume");
}

void updateAutomaticPumpController() {
  processBootResume();

  if (!featureSsrEnabled()) {
    if (manualOverrideActive) stopManualOverride("SSR interlock disabled");
    drivePumpOutput(false);
    setAutoPendingOn(false);
    if (autoPumpState == AUTO_FILLING) autoPumpState = AUTO_WAIT_SENSOR;
    alarmFillActive = false;
    return;
  }

  if (manualOverrideActive) {
    if (!sensorIsAvailable()) {
      stopManualOverride("sensor lost during override");
      return;
    }
    if (waterLevelPercent >= pumpAutoOffLevel) {
      stopManualOverride("full threshold reached during manual override");
      return;
    }
    if (!manualRiseConfirmed &&
        waterLevelPercent >= manualStartLevel + MIN_CONFIRMED_RISE_PERCENT) {
      manualRiseConfirmed = true;
    }
    if (!manualRiseConfirmed &&
        millis() - manualOverrideStartedAt >= NO_RISE_TIMEOUT_MS) {
      stopManualOverride("no water rise during manual override");
      return;
    }
    if (millis() - manualOverrideStartedAt >=
        (unsigned long)manualOverrideMinutes * 60UL * 1000UL) {
      stopManualOverride("timeout reached");
    }
    return;
  }

  // A timer or scheduled alarm session remains valid even in Alarm-only mode.
  // Autonomous low-level starts are separately gated by the selected mode.
  if (!managementAllowsAuto() && !alarmFillActive &&
      autoPumpState != AUTO_RETRY_WAIT) {
    if (autoPumpState == AUTO_FILLING) stopAutomaticFilling("automatic mode disabled", false);
    autoPumpState = AUTO_IDLE;
    fillingConfirmed = false;
    return;
  }

  const unsigned long now = millis();
  if (!sensorIsAvailable()) {
    if (autoPumpState == AUTO_FILLING) {
      drivePumpOutput(false);
      setAutoPendingOn(false);
      autoPumpState = AUTO_WAIT_SENSOR;
      alarmFillActive = false;
      Serial.println("SAFETY STOP: LoRa/sensor data unavailable");
    } else if (autoPumpState == AUTO_IDLE) {
      autoPumpState = AUTO_WAIT_SENSOR;
    }
    return;
  }

  if (autoPumpState == AUTO_WAIT_SENSOR) {
    autoPumpState = AUTO_IDLE;
    Serial.println("Sensor restored; controller ready");
  }

  // Warning tone: first warning immediately, then every ten minutes.
  const bool warning = pumpRetryWarningActive();
  if (warning && !retryWarningWasActive) {
    buzzerBeep(120, buzzerVolume());
    lastRetryWarningBeep = now;
  } else if (warning && now - lastRetryWarningBeep >= RETRY_WARNING_BEEP_MS) {
    buzzerBeep(120, buzzerVolume());
    lastRetryWarningBeep = now;
  }
  retryWarningWasActive = warning;

  switch (autoPumpState) {
    case AUTO_IDLE:
      if (managementAllowsAuto() && waterLevelPercent <= pumpAutoOnLevel) {
        startAutomaticFilling("water level at/below ON threshold");
      }
      break;

    case AUTO_FILLING:
      if (waterLevelPercent >= pumpAutoOffLevel) {
        stopAutomaticFilling("full threshold reached", false);
        break;
      }
      if (now - pumpStartedAt >= MAX_PUMP_RUNTIME_MS) {
        stopAutomaticFilling("maximum runtime exceeded", true);
        break;
      }
      if (!fillingConfirmed &&
          waterLevelPercent >= pumpStartLevel + MIN_CONFIRMED_RISE_PERCENT) {
        fillingConfirmed = true;
        Serial.println("Water rise confirmed");
      }
      if (!fillingConfirmed && now - pumpStartedAt >= NO_RISE_TIMEOUT_MS) {
        stopAutomaticFilling("no water rise within 15 minutes", true);
      }
      break;

   case AUTO_RETRY_WAIT: {
      if (waterLevelPercent >= pumpAutoOffLevel) {
        // Water has reached the target while the pump was paused; no retry is needed.
        clearRetryRuntime();
        autoPumpState = AUTO_IDLE;
        manualRetryAttemptActive = false;
        pumpRetryClearManualTime();
        break;
      }
      if (!managementAllowsAuto() && !alarmFillActive) {
        clearRetryRuntime();
        autoPumpState = AUTO_IDLE;
        manualRetryAttemptActive = false;
        pumpRetryClearManualTime();
        break;
      }
      if (manualRetryEnabled) {
        if (timeIsValid()) {
          if (manualRetryTarget == 0) manualRetryTarget = calculateNextRetryTarget();
          if (manualRetryTarget != 0 && nowLocal() >= manualRetryTarget) {
            // Suppress the red alert while the user-requested retry attempt
            // is actually running. If it fails again, scheduleRetry clears
            // this latch and the warning returns.
            manualRetryEnabled = false;
            manualRetryTarget = 0;
            retryTargetEpoch = 0;
            retryPrefs.begin("retry-config", false);
            retryPrefs.putBool("timeEn", false);
            retryPrefs.end();
            manualRetryAttemptActive = true;
            retryAt = millis();
            persistRetryRuntime();
          }
        }
      } else if (!alarmFillActive && !managementAllowsAuto() &&
                 waterLevelPercent > pumpAutoOnLevel) {
        clearRetryRuntime();
        autoPumpState = AUTO_IDLE;
        break;
      }

      bool retryDue = (long)(now - retryAt) >= 0;
      if (retryTargetEpoch > 0 && timeIsValid()) {
        retryDue = nowLocal() >= retryTargetEpoch;
      }
      if (!manualRetryEnabled && retryDue) {
        startAutomaticFilling("retry timer");
      }
         break;
    }

    case AUTO_WAIT_SENSOR:
      break;
  }
}
