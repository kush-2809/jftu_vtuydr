#pragma once

void drivePumpOutput(bool on);
void setAutoPendingOn(bool on);
void setOverridePendingOn(bool on);
void initializeManualPump();
void startManualOverride();
void stopManualOverride(const char* reason);
void handleManualPumpTouch();

unsigned long getRetryInterval();
void startAutomaticFilling(const char* reason);
void stopAutomaticFilling(const char* reason, bool scheduleRetry);
void processBootResume();
void updateAutomaticPumpController();
