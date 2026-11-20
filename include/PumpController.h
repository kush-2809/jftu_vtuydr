#pragma once
#include <Arduino.h>


void drivePumpOutput(bool on);
void setAutoPendingOn(bool on);
void setOverridePendingOn(bool on);
void initializeManualPump();
void startManualOverride();
void stopManualOverride(const char* reason);
void handleManualPumpTouch();

unsigned long getRetryInterval();
void startAutomaticFilling(const char* reason);
bool startAlarmFill(const char* reason);
bool alarmFillSessionActive();
bool pumpRetryWarningActive();
uint8_t pumpFailedAttemptCount();
uint32_t pumpRetryRemainingSeconds();
void pumpRetrySetManualTime(uint8_t hour, uint8_t minute);
void pumpRetryClearManualTime();
bool pumpRetryManualTimeEnabled();
uint8_t pumpRetryManualHour();
uint8_t pumpRetryManualMinute();
void stopAutomaticFilling(const char* reason, bool scheduleRetry);
void processBootResume();
void updateAutomaticPumpController();
