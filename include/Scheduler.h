#pragma once
#include <Arduino.h>

struct TimeWindow {
  uint8_t onHour, onMinute, offHour, offMinute;
  bool    enabled;
  bool    days[7];
};
struct TimerRun {
  bool          active;
  uint32_t      totalMinutes;
  unsigned long startedAt;
};
struct SleepWindow {
  uint8_t startHour, startMinute, endHour, endMinute;
  bool    enabled;
};

void schedulerInit();
void schedulerTick();
void schedulerFormatWindow(char* out, size_t n);

TimeWindow&  schedulerWindow();
SleepWindow& sleepWindow();
bool inSleepWindow();

void     timerStart(uint32_t minutes);
void     timerCancel();
uint32_t timerRemainingSeconds();
bool     timerActive();

void prefsSaveScheduler();
void prefsSaveSleep();