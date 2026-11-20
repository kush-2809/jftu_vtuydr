#include "UiPages.h"
#include "UiState.h"
#include "Theme.h"
#include "TimeService.h"
#include "Scheduler.h"
#include "DisplayEngine.h"
#include "SmartTankState.h"
#include "Buzzer.h"           // ← ADD THIS LINE
#include "Telemetry.h"        // (from the last drop)
#include "WiFiManager.h"
#include "Management.h"
#include "FeatureControl.h"
#include "PumpController.h"
#include <math.h>  
// Shared drawing helpers
static void pageHeader(const char* title) {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 30, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 8);
  canvas.print(title);
}
static void pageFooter(const char* hint) {
  const Theme& t = themeGet();
  canvas.fillRect(0, H - 19, W, 19, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13);
  canvas.print(hint);
}
static int clampI(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ============================================================
// ALARM → TIMER
// ============================================================
static uint32_t timerDraftMin = 10;
static bool timerStartError = false;

void uiAlarmTimerTick() {
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { timerStartError = false; uiPop(); return; }
  if (dir != 0) {
    timerDraftMin = (uint32_t)clampI((int)timerDraftMin + dir, 1, 60);
    timerStartError = false;
  }
  if (uiConsumePress()) {
    timerStart(timerDraftMin);
    if (timerActive()) {
      timerStartError = false;
      uiPop();
    } else {
      timerStartError = true;
    }
  }
}
void uiAlarmTimerDraw() {
  const Theme& t = themeGet();
  pageHeader("TIMER");
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 45); canvas.print("RUN PUMP FOR");
  canvas.setTextSize(4); canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 70); canvas.printf("%u", (unsigned)timerDraftMin);
  canvas.setTextSize(2); canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(90, 88); canvas.print("min");

  uint32_t remain = timerRemainingSeconds();       // ← re-add this
  if (remain > 0) {
    canvas.setTextSize(2); canvas.setTextColor(t.ok, t.bg);
    canvas.setCursor(10, 140);
    canvas.printf("RUNNING %02u:%02u",
                  (unsigned)(remain / 60),
                  (unsigned)(remain % 60));
  } else {
    canvas.setTextSize(1);
    canvas.setTextColor(timerStartError ? t.err : t.muted, t.bg);
    canvas.setCursor(10, 140);
    canvas.print(timerStartError ? "Start blocked: check mode / sensor" : "Not running");
  }

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 174);
  canvas.print("Maximum timer: 60 minutes");
  pageFooter("ROTATE: ADJUST  PRESS: START  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// ALARM → DAILY SCHEDULER (starts once per selected day and fills to OFF level)
// ============================================================
static int schStep = 0;
static bool schedulerAllDays() {
  TimeWindow& w = schedulerWindow();
  for (int i = 0; i < 7; ++i) if (!w.days[i]) return false;
  return true;
}
static void setSchedulerAllDays(bool enabled) {
  TimeWindow& w = schedulerWindow();
  for (int i = 0; i < 7; ++i) w.days[i] = enabled;
}

void uiAlarmSchedulerTick() {
  TimeWindow& w = schedulerWindow();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { schStep = 0; uiPop(); return; }
  if (uiConsumePress()) {
    if (schStep >= 4) {
      prefsSaveScheduler();
      schStep = 0;
      uiPop();
    } else {
      ++schStep;
    }
    return;
  }
  if (dir == 0) return;
  switch (schStep) {
    case 0: w.onHour = (uint8_t)clampI(w.onHour + dir, 0, 23); break;
    case 1: w.onMinute = (uint8_t)clampI(w.onMinute + dir, 0, 59); break;
    case 2: setSchedulerAllDays(!schedulerAllDays()); break;
    case 3: w.enabled = !w.enabled; break;
  }
}

void uiAlarmSchedulerDraw() {
  const Theme& t = themeGet();
  TimeWindow& w = schedulerWindow();
  pageHeader("DAILY SCHEDULER");
  const char* labels[] = {"Start hour", "Start minute", "Every day", "Enabled", "SAVE"};
  for (int i = 0; i < 5; ++i) {
    int y = 55 + i * 39;
    bool selected = i == schStep;
    if (selected) canvas.fillRoundRect(8, y - 3, W - 16, 31, 5, t.selBg);
    canvas.setTextSize(1);
    canvas.setTextColor(selected ? t.selFg : t.muted, selected ? t.selBg : t.bg);
    canvas.setCursor(14, y + 5);
    canvas.print(labels[i]);
    char val[12] = "";
    switch (i) {
      case 0: snprintf(val, sizeof(val), "%02u", w.onHour); break;
      case 1: snprintf(val, sizeof(val), "%02u", w.onMinute); break;
      case 2: snprintf(val, sizeof(val), "%s", schedulerAllDays() ? "YES" : "NO"); break;
      case 3: snprintf(val, sizeof(val), "%s", w.enabled ? "ON" : "OFF"); break;
      case 4: snprintf(val, sizeof(val), "PRESS"); break;
    }
    canvas.setTextSize(2);
    canvas.setTextColor(selected ? t.selFg : t.accent, selected ? t.selBg : t.bg);
    canvas.setCursor(154, y + 3);
    canvas.print(val);
  }
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 257);
  canvas.print("Starts filling at this time; stops at full threshold.");
  pageFooter("ROTATE: EDIT  PRESS: NEXT / SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// ALARM → SLEEP
// ============================================================
static int sleepStep = 0;

void uiAlarmSleepTick() {
  SleepWindow& s = sleepWindow();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { sleepStep = 0; uiPop(); return; }
  if (uiConsumePress()) {
    sleepStep = (sleepStep + 1) % 6;
    if (sleepStep == 5) { prefsSaveSleep(); uiPop(); }
    return;
  }
  if (dir == 0) return;
  switch (sleepStep) {
    case 0: s.startHour   = (uint8_t)clampI(s.startHour   + dir, 0, 23); break;
    case 1: s.startMinute = (uint8_t)clampI(s.startMinute + dir, 0, 59); break;
    case 2: s.endHour     = (uint8_t)clampI(s.endHour     + dir, 0, 23); break;
    case 3: s.endMinute   = (uint8_t)clampI(s.endMinute   + dir, 0, 59); break;
    case 4: s.enabled = !s.enabled; break;
  }
}

void uiAlarmSleepDraw() {
  const Theme& t = themeGet();
  SleepWindow& s = sleepWindow();
  pageHeader("SLEEP WINDOW");
  canvas.setTextSize(1);
  const char* labels[] = {"Start hour","Start minute","End hour","End minute","Enabled","SAVE"};
  for (int i = 0; i < 6; i++) {
    int y = 42 + i * 28;
    if (i == sleepStep) canvas.fillRoundRect(8, y - 2, W - 16, 24, 5, t.selBg);
    canvas.setTextColor(i == sleepStep ? t.selFg : t.muted,
                        i == sleepStep ? t.selBg : t.bg);
    canvas.setCursor(12, y); canvas.print(labels[i]);
    char buf[8]; const char* val = "";
    switch (i) {
      case 0: snprintf(buf, sizeof(buf), "%02u", s.startHour);   val = buf; break;
      case 1: snprintf(buf, sizeof(buf), "%02u", s.startMinute); val = buf; break;
      case 2: snprintf(buf, sizeof(buf), "%02u", s.endHour);     val = buf; break;
      case 3: snprintf(buf, sizeof(buf), "%02u", s.endMinute);   val = buf; break;
      case 4: val = s.enabled ? "ON" : "OFF"; break;
    }
    canvas.setTextColor(i == sleepStep ? t.selFg : t.accent,
                        i == sleepStep ? t.selBg : t.bg);
    canvas.setCursor(160, y); canvas.print(val);
  }
  pageFooter("ROTATE: EDIT  PRESS: NEXT  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// DISPLAY → TIMEOUT
// ============================================================
static uint16_t toDraft = 15;
static bool toDraftLoaded = false;

void uiDisplayTimeoutTick() {
  if (!toDraftLoaded) { toDraft = displayGetTimeoutSeconds(); toDraftLoaded = true; }
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { toDraftLoaded = false; uiPop(); return; }
  if (uiConsumePress()) { displaySetTimeoutSeconds(toDraft); toDraftLoaded = false; uiPop(); return; }
  if (dir != 0) toDraft = (uint16_t)clampI((int)toDraft + dir * 5, 0, 600);
}

void uiDisplayTimeoutDraw() {
  const Theme& t = themeGet();
  pageHeader("SCREEN TIMEOUT");
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 50); canvas.print("DIMMING STARTS AFTER");
  canvas.setTextSize(4); canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 75); canvas.printf("%u", toDraft);
  canvas.setTextSize(2); canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(90, 95); canvas.print("s");
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 150); canvas.print("0 = disabled");
  pageFooter("ROTATE: ADJUST  PRESS: SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// DISPLAY → DIM STEP
// ============================================================
static uint8_t dimDraft = 10;
static bool dimDraftLoaded = false;

void uiDisplayDimTick() {
  if (!dimDraftLoaded) { dimDraft = displayGetDimStep(); dimDraftLoaded = true; }
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { dimDraftLoaded = false; uiPop(); return; }
  if (uiConsumePress()) { displaySetDimStep(dimDraft); dimDraftLoaded = false; uiPop(); return; }
  if (dir != 0) dimDraft = (uint8_t)clampI((int)dimDraft + dir, 10, 20);
}

void uiDisplayDimDraw() {
  const Theme& t = themeGet();
  pageHeader("DIM STEP");
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 50); canvas.print("BRIGHTNESS DROPS BY");
  canvas.setTextSize(4); canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 75); canvas.printf("%u%%", dimDraft);
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 140); canvas.print("Minimum brightness: 40%");
  pageFooter("ROTATE: ADJUST  PRESS: SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}
// ============================================================
// DISPLAY → DATE & TIME (sub-menu)
// ============================================================
static int dtMenuSel = 0;
static const char* dtMenuLabels[] = {
  "Set manually",
  "Sync now (NTP)",
  "Change NTP server"
};
static const int DT_MENU_COUNT = 3;

static int  dtStep = 0;
static int  dtv[6] = {2026, 1, 1, 0, 0, 0};

// The editor sub-page — reached via dtMenuSel 0
static bool dtEditorActive = false;

void uiDisplayDateTimeDraw() {
  // If we're not in editor mode, draw the sub-menu
  if (!dtEditorActive) {
    const Theme& t = themeGet();
    pageHeader("DATE & TIME");
    canvas.setTextSize(1);
    for (int i = 0; i < DT_MENU_COUNT; i++) {
      int y = 50 + i * 34;
      if (i == dtMenuSel) canvas.fillRoundRect(8, y - 2, W - 16, 30, 6, t.selBg);
      canvas.setTextColor(i == dtMenuSel ? t.selFg : t.fg,
                          i == dtMenuSel ? t.selBg : t.bg);
      canvas.setTextSize(2);
      canvas.setCursor(16, y + 4);
      canvas.print(dtMenuLabels[i]);
    }
    canvas.setTextSize(1);
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(10, H - 42);
    canvas.printf("Now: %s", timeFormatFull().c_str());
    canvas.setCursor(10, H - 30);
    canvas.printf("NTP: %s", timeNtpSynced() ? "SYNCED" : "WAITING");
    pageFooter("ROTATE: MOVE  PRESS: OPEN  HOLD: BACK");
    canvas.pushSprite(0, 0);
    return;
  }

  // ── editor mode ──
  const Theme& t = themeGet();
  pageHeader("SET DATE & TIME");
  canvas.setTextSize(1);
  const char* labels[] = {"Year","Month","Day","Hour","Minute","Second","SAVE"};
  for (int i = 0; i < 7; i++) {
    int y = 40 + i * 20;
    if (i == dtStep) canvas.fillRoundRect(8, y - 2, W - 16, 19, 4, t.selBg);
    canvas.setTextColor(i == dtStep ? t.selFg : t.muted,
                        i == dtStep ? t.selBg : t.bg);
    canvas.setCursor(12, y); canvas.print(labels[i]);
    canvas.setTextColor(i == dtStep ? t.selFg : t.accent,
                        i == dtStep ? t.selBg : t.bg);
    if (i < 6) {
      char buf[8];
      snprintf(buf, sizeof(buf), i == 0 ? "%04d" : "%02d", dtv[i]);
      canvas.setCursor(150, y); canvas.print(buf);
    } else {
      canvas.setCursor(150, y); canvas.print("PRESS");
    }
  }
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 194);
  canvas.printf("Now: %s", timeFormatFull().c_str());
  pageFooter("ROTATE: EDIT  PRESS: NEXT/SAVE  HOLD: CANCEL");
  canvas.pushSprite(0, 0);
}

void uiDisplayDateTimeTick() {
  if (!dtEditorActive) {
    // sub-menu input
    int dir = uiConsumeRotation();
    if (uiConsumeLongPress()) { uiPop(); return; }
    if (dir != 0)
      dtMenuSel = (dtMenuSel + (dir > 0 ? 1 : DT_MENU_COUNT - 1)) % DT_MENU_COUNT;
    if (uiConsumePress()) {
      if (dtMenuSel == 0) {
        // Load current time
        if (timeIsValid()) {
          time_t t = nowLocal(); struct tm tmv; localtime_r(&t, &tmv);
          dtv[0] = tmv.tm_year + 1900; dtv[1] = tmv.tm_mon + 1; dtv[2] = tmv.tm_mday;
          dtv[3] = tmv.tm_hour;        dtv[4] = tmv.tm_min;    dtv[5] = tmv.tm_sec;
        }
        dtStep = 0;
        dtEditorActive = true;
      } else if (dtMenuSel == 1) {
        timeRequestNtpSync();
        buzzerBeep(60, buzzerVolume());
      } else if (dtMenuSel == 2) {
        uiPush(Page::DisplayNTP);
      }
    }
    return;
  }
  // editor input
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) {
    dtEditorActive = false;
    return;
  }
  if (uiConsumePress()) {
    if (dtStep < 6) {
      ++dtStep;
    } else {
      // Reject impossible calendar dates rather than normalising them silently.
      const bool leap = (dtv[0] % 4 == 0 && dtv[0] % 100 != 0) || (dtv[0] % 400 == 0);
      const int monthDays[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      if (dtv[2] >= 1 && dtv[2] <= monthDays[dtv[1] - 1]) {
        timeSetManual(dtv[0], dtv[1], dtv[2], dtv[3], dtv[4], dtv[5]);
        buzzerBeep(60, buzzerVolume());
        dtEditorActive = false;
      }
      return;
    }
  }
  if (dir == 0 || dtStep >= 6) return;
  const int lim[6][2] = {{2020,2099},{1,12},{1,31},{0,23},{0,59},{0,59}};
  dtv[dtStep] = clampI(dtv[dtStep] + dir, lim[dtStep][0], lim[dtStep][1]);
  if (dtStep == 2) {
    const bool leap = (dtv[0] % 4 == 0 && dtv[0] % 100 != 0) || (dtv[0] % 400 == 0);
    const int monthDays[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    dtv[2] = clampI(dtv[2], 1, monthDays[dtv[1] - 1]);
  }
  if (dtStep == 1 && dtv[2] > 0) {
    const bool leap = (dtv[0] % 4 == 0 && dtv[0] % 100 != 0) || (dtv[0] % 400 == 0);
    const int monthDays[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    dtv[2] = clampI(dtv[2], 1, monthDays[dtv[1] - 1]);
  }
}
// ============================================================
// DISPLAY → NTP SERVER
// ============================================================
static int ntpPreset = 0;
static bool ntpPresetLoaded = false;
static const char* ntpPresets[] = {
  "pool.ntp.org", "time.nist.gov", "time.google.com", "time.cloudflare.com"
};

void uiDisplayNTPTick() {
  if (!ntpPresetLoaded) {
    const String active = timeGetNtpServer();
    for (int i = 0; i < 4; ++i) {
      if (active == ntpPresets[i]) { ntpPreset = i; break; }
    }
    ntpPresetLoaded = true;
  }
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { ntpPresetLoaded = false; uiPop(); return; }
  if (dir != 0) {
    ntpPreset = (ntpPreset + (dir > 0 ? 1 : 3)) % 4;
  }
  if (uiConsumePress()) {
    timeSetNtpServer(String(ntpPresets[ntpPreset]));
    ntpPresetLoaded = false;
    uiPop();
  }
}

void uiDisplayNTPDraw() {
  const Theme& t = themeGet();
  pageHeader("NTP SERVER");
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 45); canvas.print("PRESET SERVER");
  canvas.setTextSize(2); canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 65); canvas.print(ntpPresets[ntpPreset]);
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 110);
  canvas.printf("Active: %s", timeGetNtpServer().c_str());
  canvas.setCursor(10, 130);
  canvas.printf("NTP status: %s", timeNtpSynced() ? "SYNCED" : "WAITING");
  canvas.setCursor(10, 160);
  canvas.printf("Now: %s", timeFormatFull().c_str());
  pageFooter("ROTATE: PRESET  PRESS: SAVE+SYNC  HOLD: BACK");
  canvas.pushSprite(0, 0);
}
// ============================================================
// TELEMETRY
// ============================================================

void uiTelemetryDraw() {
  const Theme& t = themeGet();
  pageHeader("TELEMETRY");
  canvas.setTextSize(1);

  auto row = [&](int y, const char* label, const String& value, uint16_t color) {
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(10, y);
    canvas.print(label);
    canvas.setTextColor(color, t.bg);
    canvas.setCursor(132, y);
    canvas.print(value);
  };

  const int8_t cpu = telemetryCpuPercent();
  const float temp = telemetryChipTempC();
  const uint32_t heapFree = telemetryHeapFreeKb();
  const uint32_t heapUsed = telemetryHeapUsedKb();
  const uint32_t psramFree = telemetryPsramFreeKb();
  const uint32_t psramTotal = telemetryPsramTotalKb();
  const uint32_t uptime = telemetryUptimeSeconds();
  const uint16_t cpuColor = cpu < 0 ? t.warn : cpu < 40 ? t.ok : cpu < 75 ? t.warn : t.err;
  const uint16_t tempColor = temp < 50 ? t.ok : temp < 70 ? t.warn : t.err;

  row(43, "CPU load", cpu >= 0 ? String(cpu) + " %" : "Not configured", cpuColor);
  row(61, "CPU frequency", String(telemetryCpuFrequencyMHz()) + " MHz", t.fg);
  row(79, "SoC temperature", isfinite(temp) ? String(temp, 1) + " C" : "N/A", tempColor);
  row(97, "Heap free", String(heapFree) + " KB", t.fg);
  row(115, "Heap used", String(heapUsed) + " KB", t.fg);
  row(133, "PSRAM free", psramTotal ? String(psramFree) + " KB" : "Not detected", t.fg);
  row(151, "PSRAM total", psramTotal ? String(psramTotal) + " KB" : "Not detected", t.fg);
  row(169, "Uptime", String(uptime / 3600) + "h " + String((uptime / 60) % 60) + "m", t.fg);

  canvas.drawFastHLine(8, 190, W - 16, t.muted);
  canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 201);
  canvas.print("ELECTRICAL MEASUREMENTS");
  const bool acVoltageReady = isfinite(telemetryAcVoltage());
  const bool acCurrentReady = isfinite(telemetryAcCurrent());
  const bool espVoltageReady = isfinite(telemetryEspVoltage());
  const bool espCurrentReady = isfinite(telemetryEspCurrent());
  row(220, "AC voltage", acVoltageReady ? String(telemetryAcVoltage(), 1) + " V" : "Sensor not fitted", t.warn);
  row(237, "AC current", acCurrentReady ? String(telemetryAcCurrent(), 2) + " A" : "Sensor not fitted", t.warn);
  row(254, "ESP V / A", (espVoltageReady && espCurrentReady)
       ? String(telemetryEspVoltage(), 2) + " V / " + String(telemetryEspCurrent(), 2) + " A"
       : "Sensor not fitted", t.warn);

  pageFooter("CPU LOAD NEEDS RTOS STATS  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiTelemetryTick() {
  if (uiConsumeLongPress()) { uiPop(); return; }
  uiConsumeRotation();    // ignored
  uiConsumePress();       // ignored
}