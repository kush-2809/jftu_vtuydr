#include "SmartTankState.h"
#include "Theme.h"
#include "PumpController.h"
#include "Management.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "DashboardLayout.h"

// Compact feature-phone-style instrument panel for 240x320 / 320x240 ST7789.
// Water tank is deliberately the visual hero in both orientations.
namespace {
void printAt(int x, int y, const String& value, uint8_t size, uint16_t fg, uint16_t bg) {
  canvas.setTextSize(size);
  canvas.setTextColor(fg, bg);
  canvas.setCursor(x, y);
  canvas.print(value);
}
String clipped(String s, int length) {
  if ((int)s.length() > length && length > 2) s = s.substring(0, length - 2) + "..";
  return s;
}
void panel(int x, int y, int w, int h, const Theme& t) {
  canvas.fillRoundRect(x, y, w, h, 9, t.panel);
  canvas.drawRoundRect(x, y, w, h, 9, t.selBg);
}
bool levelOk() { return remoteConnected && sensorReadingValid && waterLevelValid; }

void tank(int x, int y, int w, int h, const Theme& t) {
  // Tank lid, vessel outline, gradations and proportional water fill.
  const int bodyY = y + 8, bodyH = h - 13;
  canvas.fillRoundRect(x + 6, y + 1, w - 12, 9, 4, t.accent);
  canvas.fillRoundRect(x, bodyY, w, bodyH, 11, t.tankEmpty);
  const int inset = 5;
  const int waterH = levelOk() ? (int)((bodyH - inset * 2) *
      constrain(waterLevelPercent, 0.0f, 100.0f) / 100.0f) : 0;
  if (waterH > 0) {
    const int wy = bodyY + bodyH - inset - waterH;
    canvas.fillRoundRect(x + inset, wy, w - inset * 2, waterH, 7, t.tankWater);
    canvas.drawFastHLine(x + inset + 3, wy, w - inset * 2 - 6, t.accent);
  }
  canvas.drawRoundRect(x, bodyY, w, bodyH, 11, t.muted);
  canvas.drawRoundRect(x + 2, bodyY + 2, w - 4, bodyH - 4, 9, t.selBg);
  for (int i = 1; i <= 3; ++i) {
    int tickY = bodyY + (bodyH * i) / 4;
    canvas.drawFastHLine(x + w - 16, tickY, 9, t.muted);
  }
  canvas.fillRoundRect(x + w/2 - 11, bodyY + bodyH - 1, 22, 5, 2, t.accent);
}
const char* stateText() {
  if (manualOverrideActive) return "MANUAL RUN";
  if (autoPumpState == AUTO_FILLING)
    return alarmFillSessionActive() ? "SCHEDULE RUN" : "AUTO FILLING";
  if (autoPumpState == AUTO_RETRY_WAIT) return "RETRY WAITING";
  if (autoPumpState == AUTO_WAIT_SENSOR) return "WAIT FOR SENSOR";
  if (manualPumpState) return "PUMP ON";
  return managementAllowsAuto() ? "AUTO READY" : "PUMP OFF";
}
uint16_t stateColor(const Theme& t) {
  if (pumpRetryWarningActive()) return t.err;
  if (manualOverrideActive || autoPumpState == AUTO_RETRY_WAIT) return t.warn;
  if (autoPumpState == AUTO_FILLING || manualPumpState) return t.ok;
  return t.accent;
}
String retryText(bool shortVersion) {
  const uint32_t secs = pumpRetryRemainingSeconds();
  char result[52];
  if (pumpRetryManualTimeEnabled()) {
    if (shortVersion) {
      snprintf(result, sizeof(result), "SET %02u:%02u  %02lu:%02lu",
        pumpRetryManualHour(), pumpRetryManualMinute(),
        (unsigned long)(secs / 60), (unsigned long)(secs % 60));
    } else {
      snprintf(result, sizeof(result), "AT %02u:%02u  IN %02lu:%02lu:%02lu",
        pumpRetryManualHour(), pumpRetryManualMinute(),
        (unsigned long)(secs / 3600),
        (unsigned long)((secs / 60) % 60), (unsigned long)(secs % 60));
    }
  } else {
    snprintf(result, sizeof(result), shortVersion ? "RETRY %02lu:%02lu" :
        "AUTO RETRY %02lu:%02lu:%02lu",
        (unsigned long)(shortVersion ? secs / 60 : secs / 3600),
        (unsigned long)(shortVersion ? secs % 60 : (secs / 60) % 60),
        (unsigned long)(secs % 60));
  }
  return String(result);
}
void bottom(const Theme& t) {
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.drawFastHLine(0, H - 20, W, t.selBg);
  printAt(7, H - 13, "CLICK: MENU   TURN: BRIGHTNESS", 1, t.fg, t.footerBg);
}
}


void drawAquaDashboard() {
  if (W <= 0 || H <= 0) return;
  const Theme& t = themeGet();
  const bool wide = W > H, warning = pumpRetryWarningActive();
  const WeatherSnapshot weather = weatherServiceSnapshot();
  canvas.fillSprite(t.bg);
  drawHeader();

  // Aqua is a distinct two-column feature-phone dashboard, not another theme.
  const int left = 8, gap = 7, usable = W - 16;
  const int leftW = wide ? 142 : 112;
  const int rightX = left + leftW + gap;
  const int rightW = usable - leftW - gap;

  const int tankCardTop = wide ? 39 : 43;
  const int tankCardH = wide ? 142 : 176;
  panel(left, tankCardTop, leftW, tankCardH, t);
  printAt(left+8,tankCardTop+8,"WATER LEVEL",1,t.fg,t.panel);
  printAt(left+8,tankCardTop+20,"LIVE TANK",1,t.muted,t.panel);

  const int vesselW = wide ? 54 : 55;
  const int vesselH = wide ? 88 : 113;
  const int tankX = left + (leftW-vesselW)/2 - (wide ? 16 : 0);
  const int tankY = tankCardTop + (wide ? 38 : 38);
  tank(tankX,tankY,vesselW,vesselH,t);

  // Water percentage beside tank in landscape; beneath in portrait.
  if(wide) {
    char pct[16];
    snprintf(pct,sizeof(pct),levelOk()?"%.0f%%":"--%%",waterLevelPercent);
    printAt(left+84,tankCardTop+69,pct,2,levelOk()?t.accent:t.warn,t.panel);
    printAt(left+83,tankCardTop+95,"LEVEL",1,t.muted,t.panel);
    printAt(left+83,tankCardTop+110,"LoRa",1,
        remoteConnected?t.ok:t.err,t.panel);
  } else {
    char pct[16];
    snprintf(pct,sizeof(pct),levelOk()?"%.0f%%":"--%%",waterLevelPercent);
    printAt(left+32,tankCardTop+154,pct,2,levelOk()?t.accent:t.warn,t.panel);
  }

  // Right column: separate phone-like pump card and system-health card.
  const int pumpY=tankCardTop,pumpH=wide?65:81;
  panel(rightX,pumpY,rightW,pumpH,t);
  printAt(rightX+8,pumpY+8,"PUMP CONTROL",1,t.fg,t.panel);
  printAt(rightX+8,pumpY+23,manualPumpState?"ON":"OFF",2,
          manualPumpState?t.ok:t.muted,t.panel);
  printAt(rightX+8,pumpY+(wide?47:55),
    manualOverrideActive?"MANUAL":autoPumpState==AUTO_FILLING?"FILLING":
      managementAllowsAuto()?"AUTO READY":"STANDBY",
    1,stateColor(t),t.panel);
  // Non-interactive indicator only: physical pump controller enforces interlocks.
  const int ledX=rightX+rightW-16,ledY=pumpY+30;
  canvas.fillCircle(ledX,ledY,5,manualPumpState?t.ok:t.muted);

  const int systemY=pumpY+pumpH+6;
  const int systemH=tankCardH-pumpH-6;
  panel(rightX,systemY,rightW,systemH,t);
  printAt(rightX+8,systemY+9,"SYSTEM",1,t.fg,t.panel);
  printAt(rightX+8,systemY+24,remoteConnected?"LoRa ONLINE":"LoRa OFFLINE",1,
    remoteConnected?t.ok:t.warn,t.panel);
  printAt(rightX+8,systemY+37,timeFormatHHMM(),1,t.accent,t.panel);
  if (!wide) printAt(rightX+8,systemY+51,timeNtpSynced()?"NTP SYNCED":"CLOCK",1,t.muted,t.panel);

  const int bottomY=tankCardTop+tankCardH+7;
  const int bottomH=H-20-bottomY-6;
  if(bottomH>28) {
    panel(left,bottomY,W-16,bottomH,t);
    printAt(left+9,bottomY+8,"WEATHER",1,t.muted,t.panel);
    const String city=clipped(String(weather.city[0]?weather.city:"SET CITY IN PORTAL"),wide?18:14);
    printAt(left+9,bottomY+21,city,1,t.fg,t.panel);
    if(weather.valid) {
      char tmp[20]; snprintf(tmp,sizeof(tmp),"%.0f C",weather.temperatureC);
      printAt(wide?left+116:left+144,bottomY+10,tmp,2,t.accent,t.panel);
      printAt(wide?left+186:left+144,bottomY+31,
        clipped(String(weatherConditionLabel(weather.weatherCode)),wide?15:13),1,t.muted,t.panel);
    } else {
      printAt(wide?left+148:left+144,bottomY+14,"-- C",2,t.warn,t.panel);
    }
    if (bottomH>=71) {
      printAt(left+9,bottomY+44,"WATER",1,t.muted,t.panel);
      const String volume=levelOk()?
          String((int)waterVolumeLitres)+" / "+String((int)tankCapacityLitres)+" L":
          String("WAITING FOR SENSOR");
      printAt(left+50,bottomY+44,clipped(volume,23),1,t.fg,t.panel);
    }
  }

  if(warning) {
    const int alertY=wide?194:270;
    const int alertH=wide?25:27;
    canvas.fillRoundRect(8,alertY,W-16,alertH,7,t.err);
    printAt(16,alertY+5,"LOW SUPPLY - RETRY",1,TFT_WHITE,t.err);
    const String remaining=clipped(retryText(true),wide?23:16);
    printAt(wide?180:126,alertY+5,remaining,1,TFT_WHITE,t.err);
  }
  bottom(t);
  canvas.pushSprite(0,0);
}

void drawDashboard() {
  if (dashboardLayoutIndex() == 0) { drawAquaDashboard(); return; }
  if (W <= 0 || H <= 0) return;
  const Theme& t = themeGet();
  const bool landscape = W > H;
  const bool warning = pumpRetryWarningActive();
  const WeatherSnapshot weather = weatherServiceSnapshot();
  const String place = clipped(String(weather.city[0] ? weather.city : "SET CITY"), 12);
  const String condition = weather.valid ? String(weatherConditionLabel(weather.weatherCode)) :
      String("OFFLINE");

  canvas.fillSprite(t.bg);
  drawHeader();  // Keep original working network header.

  if (!landscape) {
    // Row 1: clock + weather; large tank occupies primary visual area.
    panel(7, 37, 226, 43, t);
    printAt(17, 44, "TIME", 1, t.muted, t.panel);
    printAt(17, 57, timeFormatHHMM(), 2, t.accent, t.panel);
    canvas.drawFastVLine(105, 45, 27, t.selBg);
    printAt(116, 43, place, 1, t.muted, t.panel);
    if (weather.valid) {
      char degree[18];
      snprintf(degree, sizeof(degree), "%.0f C", weather.temperatureC);
      printAt(116, 55, degree, 2, t.fg, t.panel);
      printAt(185, 63, clipped(condition, 7), 1, t.accent, t.panel);
    } else {
      printAt(116, 58, "WEATHER --", 1, t.warn, t.panel);
    }

    panel(7, 86, 226, 144, t);
    printAt(16, 94, "SMART TANK  /  LIVE LEVEL", 1, t.muted, t.panel);
    tank(19, 111, 76, 107, t);
    printAt(108, 120, "WATER LEVEL", 1, t.muted, t.panel);
    if (levelOk()) {
      char pct[16];
      snprintf(pct, sizeof(pct), "%.0f%%", waterLevelPercent);
      printAt(105, 139, pct, 3, t.fg, t.panel);
      char litres[32];
      snprintf(litres, sizeof(litres), "%.0f / %.0f L",
          waterVolumeLitres, tankCapacityLitres);
      printAt(107, 176, litres, 1, t.muted, t.panel);
    } else {
      printAt(107, 143, "--%", 3, t.warn, t.panel);
      printAt(107, 180, "NO SENSOR", 1, t.warn, t.panel);
    }
    printAt(107, 200, remoteConnected && sensorReadingValid ?
        "LoRa CONNECTED" : "LoRa OFFLINE", 1,
        remoteConnected && sensorReadingValid ? t.ok : t.warn, t.panel);

    const uint16_t bg = warning ? t.err : t.panel;
    canvas.fillRoundRect(7, 236, 226, 61, 9, bg);
    printAt(17, 244, warning ? "WATER SUPPLY WARNING" : "PUMP STATUS",
        1, warning ? TFT_WHITE : t.muted, bg);
    printAt(17, 259, warning ? "NO WATER RISE" : stateText(), 2,
        warning ? TFT_WHITE : stateColor(t), bg);
    printAt(17, 284, warning ? clipped(retryText(false), 34) :
        (manualPumpState ? "OUTPUT ACTIVE" : "OUTPUT STANDBY"),
        1, warning ? TFT_WHITE : t.muted, bg);

  } else {
    // Landscape: large left tank, right info stack, pump across lower row.
    panel(7, 35, 152, 153, t);
    printAt(17, 44, "LIVE WATER LEVEL", 1, t.muted, t.panel);
    tank(19, 66, 68, 106, t);
    if (levelOk()) {
      char pct[16];
      snprintf(pct, sizeof(pct), "%.0f%%", waterLevelPercent);
      printAt(92, 82, pct, 2, t.fg, t.panel);
      char litres[24];
      snprintf(litres, sizeof(litres), "%.0f L", waterVolumeLitres);
      printAt(92, 112, litres, 1, t.muted, t.panel);
    } else {
      printAt(93, 85, "--%", 2, t.warn, t.panel);
    }
    printAt(92, 143, remoteConnected ? "LORA OK" : "NO LORA", 1,
        remoteConnected ? t.ok : t.warn, t.panel);

    panel(165, 35, 148, 65, t);
    printAt(175, 44, "LOCAL TIME", 1, t.muted, t.panel);
    printAt(175, 62, timeFormatHHMM(), 2, t.accent, t.panel);
    printAt(262, 72, timeNtpSynced() ? "NTP" : "CLOCK", 1, t.muted, t.panel);

    panel(165, 106, 148, 82, t);
    printAt(175, 115, place, 1, t.muted, t.panel);
    if (weather.valid) {
      char degree[18];
      snprintf(degree, sizeof(degree), "%.0f C", weather.temperatureC);
      printAt(175, 130, degree, 2, t.fg, t.panel);
      printAt(175, 164, condition, 1, t.accent, t.panel);
    } else printAt(175, 146, "WEATHER OFFLINE", 1, t.warn, t.panel);

    const uint16_t bg = warning ? t.err : t.panel;
    canvas.fillRoundRect(7, 194, 306, 25, 7, bg);
    printAt(15, 201, warning ? "NO WATER RISE" : stateText(), 1,
        warning ? TFT_WHITE : stateColor(t), bg);
    if (warning) printAt(118, 201, clipped(retryText(true), 27), 1, TFT_WHITE, bg);
    else printAt(180, 201, manualPumpState ? "PUMP ON" : "PUMP OFF",
        1, t.muted, bg);
  }
  bottom(t);
  canvas.pushSprite(0, 0);
}
