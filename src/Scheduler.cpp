#include "Scheduler.h"
#include "TimeService.h"
#include "PumpController.h"
#include "Management.h"
#include "SmartTankState.h"
#include <Preferences.h>
#include <time.h>
#include <string.h>

static Preferences prefs;
static TimeWindow win;
static TimerRun run;
static SleepWindow sw;
static int32_t lastFiredDate = 0;

static int32_t dateKey(const struct tm& tmv) {
  return (tmv.tm_year + 1900) * 10000 + (tmv.tm_mon + 1) * 100 + tmv.tm_mday;
}

static void load() {
  prefs.begin("alarm-config", true);
  win.onHour = prefs.getUChar("onH", 5);
  win.onMinute = prefs.getUChar("onM", 0);
  win.enabled = prefs.getBool("schEn", false);
  uint8_t mask = prefs.getUChar("schDays", 0x7F);
  for (int i = 0; i < 7; ++i) win.days[i] = (mask >> i) & 1;
  sw.startHour = prefs.getUChar("slSH", 1);
  sw.startMinute = prefs.getUChar("slSM", 0);
  sw.endHour = prefs.getUChar("slEH", 4);
  sw.endMinute = prefs.getUChar("slEM", 30);
  sw.enabled = prefs.getBool("slEn", false);
  lastFiredDate = prefs.getInt("firedDate", 0);
  prefs.end();

  if (win.onHour > 23) win.onHour = 5;
  if (win.onMinute > 59) win.onMinute = 0;
  if (sw.startHour > 23) sw.startHour = 1;
  if (sw.endHour > 23) sw.endHour = 4;
  if (sw.startMinute > 59) sw.startMinute = 0;
  if (sw.endMinute > 59) sw.endMinute = 30;
}

void schedulerInit() {
  load();
  memset(&run, 0, sizeof(run));
}

TimeWindow& schedulerWindow() { return win; }
SleepWindow& sleepWindow() { return sw; }
bool timerActive() { return run.active; }

void schedulerFormatWindow(char* out, size_t n) {
  snprintf(out, n, "Daily start %02u:%02u", win.onHour, win.onMinute);
}

bool inSleepWindow() {
  if (!sw.enabled || !timeIsValid()) return false;
  time_t now = nowLocal();
  struct tm tmv;
  localtime_r(&now, &tmv);
  const int nowMin = tmv.tm_hour * 60 + tmv.tm_min;
  const int startMin = sw.startHour * 60 + sw.startMinute;
  const int endMin = sw.endHour * 60 + sw.endMinute;
  if (startMin <= endMin) return nowMin >= startMin && nowMin < endMin;
  return nowMin >= startMin || nowMin < endMin;
}

void timerStart(uint32_t minutes) {
  if (minutes == 0 || !managementAllowsAlarm()) return;
  if (run.active) timerCancel();
  if (!startAlarmFill("pump timer")) return;
  run.active = true;
  run.totalMinutes = minutes;
  run.startedAt = millis();
}

void timerCancel() {
  if (!run.active && !alarmFillSessionActive()) return;
  run.active = false;
  if (alarmFillSessionActive()) stopAutomaticFilling("timer cancelled", false);
}

uint32_t timerRemainingSeconds() {
  if (!run.active) return 0;
  const unsigned long elapsed = millis() - run.startedAt;
  const unsigned long total = run.totalMinutes * 60UL * 1000UL;
  if (elapsed >= total) return 0;
  return (total - elapsed) / 1000UL;
}

void prefsSaveScheduler() {
  prefs.begin("alarm-config", false);
  prefs.putUChar("onH", win.onHour);
  prefs.putUChar("onM", win.onMinute);
  prefs.putBool("schEn", win.enabled);
  uint8_t mask = 0;
  for (int i = 0; i < 7; ++i) if (win.days[i]) mask |= (1 << i);
  prefs.putUChar("schDays", mask);
  prefs.end();
}

void prefsSaveSleep() {
  prefs.begin("alarm-config", false);
  prefs.putUChar("slSH", sw.startHour);
  prefs.putUChar("slSM", sw.startMinute);
  prefs.putUChar("slEH", sw.endHour);
  prefs.putUChar("slEM", sw.endMinute);
  prefs.putBool("slEn", sw.enabled);
  prefs.end();
}

void schedulerTick() {
  if (run.active && timerRemainingSeconds() == 0) {
    timerCancel();
  } else if (run.active && !alarmFillSessionActive() && autoPumpState == AUTO_IDLE) {
    // The full threshold ended a timer-controlled fill before its timer expired.
    run.active = false;
  }

  if (!win.enabled || !timeIsValid() || !managementAllowsAlarm()) return;

  const time_t now = nowLocal();
  struct tm tmv;
  localtime_r(&now, &tmv);
  if (tmv.tm_wday < 0 || tmv.tm_wday > 6 || !win.days[tmv.tm_wday]) return;

  const int32_t today = dateKey(tmv);
  if (lastFiredDate == today) return;
  const int nowMinute = tmv.tm_hour * 60 + tmv.tm_min;
  const int scheduledMinute = win.onHour * 60 + win.onMinute;
  if (nowMinute < scheduledMinute) return;

  // If the tank is already at the stop threshold, the scheduled goal has
  // already been met. Consume today's event without briefly energising SSR.
  if (sensorReadingValid && remoteConnected && waterLevelValid &&
      waterLevelPercent >= pumpAutoOffLevel) {
    lastFiredDate = today;
    prefs.begin("alarm-config", false);
    prefs.putInt("firedDate", lastFiredDate);
    prefs.end();
    return;
  }

  if (startAlarmFill("daily scheduler")) {
    lastFiredDate = today;
    prefs.begin("alarm-config", false);
    prefs.putInt("firedDate", lastFiredDate);
    prefs.end();
  } else if (autoPumpState == AUTO_FILLING || manualPumpState) {
    // Another valid controller session is already filling the tank.
    lastFiredDate = today;
    prefs.begin("alarm-config", false);
    prefs.putInt("firedDate", lastFiredDate);
    prefs.end();
  }
}
