#include "Scheduler.h"
#include "TimeService.h"
#include "PumpController.h"
#include "SmartTankState.h"
#include <Preferences.h>
#include <time.h>
#include <string.h>

static Preferences prefs;
static TimeWindow  win;
static TimerRun    run;
static SleepWindow sw;
static bool schedulerFiredThisWindow = false;

static void load() {
  prefs.begin("alarm-config", true);
  win.onHour    = prefs.getUChar("onH",  5);
  win.onMinute  = prefs.getUChar("onM",  0);
  win.offHour   = prefs.getUChar("offH", 7);
  win.offMinute = prefs.getUChar("offM", 0);
  win.enabled   = prefs.getBool ("schEn", false);
  uint8_t mask  = prefs.getUChar("schDays", 0x7F);
  for (int i = 0; i < 7; i++) win.days[i] = (mask >> i) & 1;
  sw.startHour   = prefs.getUChar("slSH", 1);
  sw.startMinute = prefs.getUChar("slSM", 0);
  sw.endHour     = prefs.getUChar("slEH", 4);
  sw.endMinute   = prefs.getUChar("slEM", 30);
  sw.enabled     = prefs.getBool ("slEn", false);
  prefs.end();
}

void schedulerInit() {
  load();
  memset(&run, 0, sizeof(run));
}

TimeWindow&  schedulerWindow() { return win; }
SleepWindow& sleepWindow()     { return sw; }
bool         timerActive()     { return run.active; }

void schedulerFormatWindow(char* out, size_t n) {
  snprintf(out, n, "%02d:%02d -> %02d:%02d",
           win.onHour, win.onMinute, win.offHour, win.offMinute);
}

bool inSleepWindow() {
  if (!sw.enabled || !timeIsValid()) return false;
  time_t t = nowLocal(); struct tm tmv; localtime_r(&t, &tmv);
  int nowMin = tmv.tm_hour * 60 + tmv.tm_min;
  int sMin   = sw.startHour * 60 + sw.startMinute;
  int eMin   = sw.endHour   * 60 + sw.endMinute;
  if (sMin <= eMin) return nowMin >= sMin && nowMin < eMin;
  return nowMin >= sMin || nowMin < eMin;
}

void timerStart(uint32_t minutes) {
  if (minutes == 0) return;
  run.active = true; run.totalMinutes = minutes; run.startedAt = millis();
  startAutomaticFilling("timer start");
}
void timerCancel() {
  if (!run.active) return;
  run.active = false;
  if (autoPumpState == AUTO_FILLING) stopAutomaticFilling("timer cancel", false);
}
uint32_t timerRemainingSeconds() {
  if (!run.active) return 0;
  unsigned long elapsed = millis() - run.startedAt;
  unsigned long total   = run.totalMinutes * 60UL * 1000UL;
  if (elapsed >= total) return 0;
  return (total - elapsed) / 1000UL;
}

void prefsSaveScheduler() {
  prefs.begin("alarm-config", false);
  prefs.putUChar("onH",  win.onHour);
  prefs.putUChar("onM",  win.onMinute);
  prefs.putUChar("offH", win.offHour);
  prefs.putUChar("offM", win.offMinute);
  prefs.putBool ("schEn", win.enabled);
  uint8_t mask = 0;
  for (int i = 0; i < 7; i++) if (win.days[i]) mask |= (1 << i);
  prefs.putUChar("schDays", mask);
  prefs.end();
}

void prefsSaveSleep() {
  prefs.begin("alarm-config", false);
  prefs.putUChar("slSH", sw.startHour);
  prefs.putUChar("slSM", sw.startMinute);
  prefs.putUChar("slEH", sw.endHour);
  prefs.putUChar("slEM", sw.endMinute);
  prefs.putBool ("slEn", sw.enabled);
  prefs.end();
}

void schedulerTick() {
  if (run.active && timerRemainingSeconds() == 0) timerCancel();

  if (!win.enabled || !timeIsValid()) return;

  time_t t = nowLocal(); struct tm tmv; localtime_r(&t, &tmv);
  if (tmv.tm_wday < 0 || tmv.tm_wday > 6) return;
  if (!win.days[tmv.tm_wday]) { schedulerFiredThisWindow = false; return; }

  int nowMin = tmv.tm_hour * 60 + tmv.tm_min;
  int onMin  = win.onHour * 60 + win.onMinute;
  int offMin = win.offHour * 60 + win.offMinute;

  bool inWindow = (onMin <= offMin)
      ? (nowMin >= onMin && nowMin < offMin)
      : (nowMin >= onMin || nowMin < offMin);

  if (inWindow && !schedulerFiredThisWindow) {
    schedulerFiredThisWindow = true;
    if (autoPumpState == AUTO_IDLE && !manualOverrideActive)
      startAutomaticFilling("scheduler window");
  }
  if (!inWindow) {
    if (schedulerFiredThisWindow && autoPumpState == AUTO_FILLING)
      stopAutomaticFilling("scheduler window ended", false);
    schedulerFiredThisWindow = false;
  }
}