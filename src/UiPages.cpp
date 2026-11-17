#include "UiPages.h"
#include "UiState.h"
#include "Theme.h"
#include "TimeService.h"
#include "Scheduler.h"
#include "DisplayEngine.h"
#include "SmartTankState.h"
#include "Buzzer.h"           // ← ADD THIS LINE
#include "Telemetry.h"        // (from the last drop)
#include "WiFiManager.h"      // (from the last drop)
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

void uiAlarmTimerTick() {
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (dir != 0) timerDraftMin = (uint32_t)clampI((int)timerDraftMin + dir, 1, 600);
  if (uiConsumePress()) { timerStart(timerDraftMin); uiPop(); }
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
    canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(10, 140); canvas.print("Not running");
  }

  pageFooter("ROTATE: ADJUST  PRESS: START  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// ALARM → SCHEDULER
// ============================================================
static int schStep = 0;

void uiAlarmSchedulerTick() {
  TimeWindow& w = schedulerWindow();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (uiConsumePress()) {
    schStep = (schStep + 1) % 6;
    if (schStep == 5) { prefsSaveScheduler(); uiPop(); }
    return;
  }
  if (dir == 0) return;
  switch (schStep) {
    case 0: w.onHour    = (uint8_t)clampI(w.onHour    + dir, 0, 23); break;
    case 1: w.onMinute  = (uint8_t)clampI(w.onMinute  + dir, 0, 59); break;
    case 2: w.offHour   = (uint8_t)clampI(w.offHour   + dir, 0, 23); break;
    case 3: w.offMinute = (uint8_t)clampI(w.offMinute + dir, 0, 59); break;
    case 4: w.enabled = !w.enabled; break;
  }
}

void uiAlarmSchedulerDraw() {
  const Theme& t = themeGet();
  TimeWindow& w = schedulerWindow();
  pageHeader("SCHEDULER");
  canvas.setTextSize(1);
  const char* labels[] = {"ON hour","ON minute","OFF hour","OFF minute","Enabled","SAVE"};
  for (int i = 0; i < 6; i++) {
    int y = 42 + i * 28;
    if (i == schStep) canvas.fillRoundRect(8, y - 2, W - 16, 24, 5, t.selBg);
    canvas.setTextColor(i == schStep ? t.selFg : t.muted,
                        i == schStep ? t.selBg : t.bg);
    canvas.setCursor(12, y); canvas.print(labels[i]);
    char buf[8]; const char* val = "";
    switch (i) {
      case 0: snprintf(buf, sizeof(buf), "%02u", w.onHour);    val = buf; break;
      case 1: snprintf(buf, sizeof(buf), "%02u", w.onMinute);  val = buf; break;
      case 2: snprintf(buf, sizeof(buf), "%02u", w.offHour);   val = buf; break;
      case 3: snprintf(buf, sizeof(buf), "%02u", w.offMinute); val = buf; break;
      case 4: val = w.enabled ? "ON" : "OFF"; break;
    }
    canvas.setTextColor(i == schStep ? t.selFg : t.accent,
                        i == schStep ? t.selBg : t.bg);
    canvas.setCursor(160, y); canvas.print(val);
  }
  pageFooter("ROTATE: EDIT  PRESS: NEXT  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ============================================================
// ALARM → SLEEP
// ============================================================
static int sleepStep = 0;

void uiAlarmSleepTick() {
  SleepWindow& s = sleepWindow();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
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
static uint16_t toDraft = 0;

void uiDisplayTimeoutTick() {
  if (toDraft == 0) toDraft = displayGetTimeoutSeconds();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (uiConsumePress()) { displaySetTimeoutSeconds(toDraft); uiPop(); return; }
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
static uint8_t dimDraft = 0;

void uiDisplayDimTick() {
  if (dimDraft == 0) dimDraft = displayGetDimStep();
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (uiConsumePress()) { displaySetDimStep(dimDraft); uiPop(); return; }
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
  canvas.setCursor(10, 140); canvas.print("Floor: 40% (hard-coded)");
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

void uiDisplayDateTimeMenuTick() {
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (dir != 0) {
    dtMenuSel = (dtMenuSel + (dir > 0 ? 1 : DT_MENU_COUNT - 1)) % DT_MENU_COUNT;
  }
  if (uiConsumePress()) {
    switch (dtMenuSel) {
      case 0: {
        // Load current values as starting point
        time_t t = nowLocal(); struct tm tmv;
        if (timeIsValid()) {
          localtime_r(&t, &tmv);
          dtv[0] = tmv.tm_year + 1900; dtv[1] = tmv.tm_mon + 1; dtv[2] = tmv.tm_mday;
          dtv[3] = tmv.tm_hour;        dtv[4] = tmv.tm_min;    dtv[5] = tmv.tm_sec;
        }
        dtStep = 0;
        uiPush(Page::DisplayDateTime);   // reuse the same page slot for editor
        // BUT we need to switch mode — handled by the "editor active" flag below
        break;
      }
      case 1: timeRequestNtpSync(); break;
      case 2: uiPush(Page::DisplayNTP); break;
    }
  }
}

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
  const char* labels[] = {"Year","Month","Day","Hour","Minute","Second"};
  for (int i = 0; i < 6; i++) {
    int y = 40 + i * 22;
    if (i == dtStep) canvas.fillRoundRect(8, y - 2, W - 16, 20, 4, t.selBg);
    canvas.setTextColor(i == dtStep ? t.selFg : t.muted,
                        i == dtStep ? t.selBg : t.bg);
    canvas.setCursor(12, y); canvas.print(labels[i]);
    canvas.setTextColor(i == dtStep ? t.selFg : t.accent,
                        i == dtStep ? t.selBg : t.bg);
    char buf[8];
    snprintf(buf, sizeof(buf), "%04d", dtv[i]);
    canvas.setCursor(140, y); canvas.print(buf);
  }
  canvas.setTextSize(1); canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, H - 40);
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
    uiPop();
    return;
  }
  if (uiConsumePress()) {
    dtStep = (dtStep + 1) % 6;
    if (dtStep == 5) {
      // Apply
      timeSetManual(dtv[0], dtv[1], dtv[2], dtv[3], dtv[4], dtv[5]);
      buzzerBeep(60, buzzerVolume());
      dtEditorActive = false;
      uiPop();
      return;
    }
  }
  if (dir == 0) return;
  const int lim[6][2] = {{2020,2099},{1,12},{1,31},{0,23},{0,59},{0,59}};
  dtv[dtStep] = clampI(dtv[dtStep] + dir, lim[dtStep][0], lim[dtStep][1]);
}
// ============================================================
// DISPLAY → NTP SERVER
// ============================================================
static int ntpPreset = 0;
static const char* ntpPresets[] = {
  "pool.ntp.org", "time.nist.gov", "time.google.com", "time.cloudflare.com"
};

void uiDisplayNTPTick() {
  int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (dir != 0) {
    ntpPreset = (ntpPreset + (dir > 0 ? 1 : 3)) % 4;
  }
  if (uiConsumePress()) {
    timeSetNtpServer(String(ntpPresets[ntpPreset]));
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

  auto row = [&](int y, const char* label, const String& value,
                 uint16_t valueColor) {
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(10, y);
    canvas.print(label);
    canvas.setTextColor(valueColor, t.bg);
    canvas.setCursor(130, y);
    canvas.print(value);
  };

  uint8_t  cpu   = telemetryCpuPercent();
  uint8_t  cpu0  = telemetryCpuCore0Percent();
  uint8_t  cpu1  = telemetryCpuCore1Percent();
  uint32_t hFree = telemetryHeapFreeKb();
  uint32_t hUsed = telemetryHeapUsedKb();
  float    temp  = telemetryChipTempC();
  uint32_t upSec = telemetryUptimeSeconds();

  uint16_t cpuColor  = cpu  < 40 ? t.ok : cpu  < 75 ? t.warn : t.err;
  uint16_t tempColor = temp < 50 ? t.ok : temp < 70 ? t.warn : t.err;

  row(44,  "CPU",       String(cpu)  + " %",                     cpuColor);
  row(62,  "  Core 0",  String(cpu0) + " %",                     t.fg);
  row(80,  "  Core 1",  String(cpu1) + " %",                     t.fg);
  row(98,  "RAM free",  String(hFree) + " KB",                   t.fg);
  row(116, "RAM used",  String(hUsed) + " KB",                   t.fg);
  row(134, "Chip temp", String(temp, 1) + " C",                  tempColor);
  row(152, "Uptime",    String(upSec / 3600) + "h " +
                        String((upSec / 60) % 60) + "m " +
                        String(upSec % 60) + "s",                t.fg);

  canvas.drawFastHLine(8, 176, W - 16, t.muted);

  row(184, "LoRa RSSI", remoteConnected ? String(lastRSSI) + " dBm" : "--",   t.accent);
  row(202, "LoRa SNR",  remoteConnected ? String(lastSNR, 1) + " dB" : "--", t.accent);
  row(220, "Wi-Fi RSSI", wifiManagerIsConnected()
                          ? String(wifiManagerGetRSSI()) + " dBm" : "--",     t.accent);

  pageFooter("Live - updates 1x/sec - HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiTelemetryTick() {
  if (uiConsumeLongPress()) { uiPop(); return; }
  uiConsumeRotation();    // ignored
  uiConsumePress();       // ignored
}