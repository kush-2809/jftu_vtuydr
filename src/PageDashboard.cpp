#include "SmartTankState.h"
#include "Theme.h"
#include "PumpController.h"
#include "Management.h"
#include "TimeService.h"
#include "WeatherService.h"

// Premium compact dashboard for ST7789 240x320 (portrait) and 320x240.
// Everything is painted into the existing sprite; only push once per frame.
// Existing encoder, pump controller, LoRa and safety logic remain unchanged.
namespace {
void card(int x, int y, int w, int h, const Theme& t) {
  canvas.fillRoundRect(x, y, w, h, 9, t.panel);
  canvas.drawRoundRect(x, y, w, h, 9, t.selBg);
}
void label(int x, int y, const char* text, const Theme& t) {
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.panel);
  canvas.setCursor(x, y);
  canvas.print(text);
}
void small(int x, int y, const String& text, uint16_t color, uint16_t bg) {
  canvas.setTextSize(1);
  canvas.setTextColor(color, bg);
  canvas.setCursor(x, y);
  canvas.print(text);
}
String shortCity(const char* city, int maxLength) {
  String text = city[0] ? String(city) : String("SET CITY IN PORTAL");
  if (text.length() > (size_t)maxLength) text = text.substring(0, maxLength - 2) + "..";
  return text;
}
void progress(int x, int y, int w, int h, const Theme& t) {
  canvas.fillRoundRect(x, y, w, h, h / 2, t.tankEmpty);
  if (!waterLevelValid || !remoteConnected || !sensorReadingValid) return;
  const float p = constrain(waterLevelPercent, 0.0f, 100.0f);
  const int fill = (int)(w * p / 100.0f);
  if (fill > 0) canvas.fillRoundRect(x, y, fill, h, h / 2, t.tankWater);
}
const char* pumpState() {
  if (manualOverrideActive) return "MANUAL OVERRIDE";
  if (autoPumpState == AUTO_FILLING)
    return alarmFillSessionActive() ? "SCHEDULE FILL" : "AUTO FILLING";
  if (autoPumpState == AUTO_RETRY_WAIT) return "RETRY WAIT";
  if (autoPumpState == AUTO_WAIT_SENSOR) return "SENSOR WAIT";
  if (manualPumpState) return "PUMP OUTPUT ON";
  return managementAllowsAuto() ? "AUTO STANDBY" : "PUMP OFF";
}
uint16_t stateColor(const Theme& t) {
  if (pumpRetryWarningActive()) return t.err;
  if (manualOverrideActive || autoPumpState == AUTO_RETRY_WAIT) return t.warn;
  if (autoPumpState == AUTO_FILLING || manualPumpState) return t.ok;
  return t.accent;
}
void retryLine(int x, int y, const Theme& t, bool compact) {
  const uint32_t secs = pumpRetryRemainingSeconds();
  if (pumpRetryManualTimeEnabled()) {
    char line[50];
    snprintf(line, sizeof(line), compact ? "AT %02u:%02u  %02lu:%02lu LEFT" :
        "SET %02u:%02u  IN %02lu:%02lu:%02lu",
        pumpRetryManualHour(), pumpRetryManualMinute(),
        (unsigned long)(compact ? secs / 60 : secs / 3600),
        (unsigned long)(compact ? secs % 60 : (secs / 60) % 60),
        (unsigned long)(compact ? 0 : secs % 60));
    // Compact layout has only two time fields and uses a separate format.
    if (compact) {
      snprintf(line, sizeof(line), "SET %02u:%02u  %02lu:%02lu LEFT",
          pumpRetryManualHour(), pumpRetryManualMinute(),
          (unsigned long)(secs / 60), (unsigned long)(secs % 60));
    }
    small(x, y, line, t.fg, t.err);
  } else {
    char line[32];
    snprintf(line, sizeof(line), "AUTO RETRY %02lu:%02lu:%02lu",
        (unsigned long)(secs / 3600),
        (unsigned long)((secs / 60) % 60),
        (unsigned long)(secs % 60));
    small(x, y, line, t.fg, t.err);
  }
}
void footer(const Theme& t) {
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.fillRect(0, H - 20, W, 1, t.selBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(9, H - 13);
  canvas.print("PRESS: MENU   ROTATE: BRIGHTNESS");
}
} // namespace

void drawDashboard() {
  if (W <= 0 || H <= 0) return;
  const Theme& t = themeGet();
  const bool wide = W > H;
  const bool warning = pumpRetryWarningActive();
  const WeatherSnapshot weather = weatherServiceSnapshot();

  canvas.fillSprite(t.bg);
  drawHeader(); // Preserve existing Wi-Fi / LoRa icons and header.

  if (!wide) {
    // Portrait: time, weather, level, and pump safety are always visible.
    card(8, 42, W - 16, 48, t);
    label(18, 49, "LOCAL TIME", t);
    small(18, 64, timeFormatHHMM(), t.accent, t.panel);
    canvas.setTextSize(2);
    canvas.setTextColor(t.fg, t.panel);
    canvas.setCursor(88, 59);
    canvas.print(timeFormatHHMM());
    small(166, 66, timeNtpSynced() ? "NTP" : "CLOCK", t.ok, t.panel);
    small(18, 78, dateFormatDMY(), t.muted, t.panel);

    card(8, 96, W - 16, 46, t);
    label(18, 103, "LOCAL WEATHER", t);
    small(18, 116, shortCity(weather.city, 17), t.muted, t.panel);
    if (weather.valid) {
      canvas.setTextSize(2);
      canvas.setTextColor(t.fg, t.panel);
      canvas.setCursor(144, 105);
      canvas.printf("%.0fC", weather.temperatureC);
      small(144, 127, weatherConditionLabel(weather.weatherCode), t.accent, t.panel);
    } else {
      small(144, 115, "NO DATA", t.warn, t.panel);
    }

    card(8, 148, W - 16, 82, t);
    label(18, 156, "TANK LEVEL", t);
    if (remoteConnected && sensorReadingValid && waterLevelValid) {
      canvas.setTextSize(3);
      canvas.setTextColor(t.fg, t.panel);
      canvas.setCursor(16, 170);
      canvas.printf("%.0f%%", waterLevelPercent);
      canvas.setTextSize(1);
      small(18, 200, String((int)waterVolumeLitres) + " / " +
          String((int)tankCapacityLitres) + " L", t.muted, t.panel);
    } else {
      canvas.setTextSize(2);
      small(18, 179, "WAIT SENSOR", t.warn, t.panel);
    }
    const int tankX = 165, tankY = 156, tankW = 47, tankH = 59;
    canvas.drawRoundRect(tankX, tankY, tankW, tankH, 8, t.muted);
    canvas.fillRoundRect(tankX + 4, tankY + 4, tankW - 8, tankH - 8, 5, t.tankEmpty);
    if (remoteConnected && sensorReadingValid && waterLevelValid) {
      const int h = (int)((tankH - 8) * constrain(waterLevelPercent, 0.0f, 100.0f) / 100.0f);
      if (h) canvas.fillRoundRect(tankX + 4, tankY + tankH - 4 - h,
          tankW - 8, h, 4, t.tankWater);
    }
    progress(18, 218, W - 36, 5, t);

    const uint16_t pumpBg = warning ? t.err : t.panel;
    canvas.fillRoundRect(8, 236, W - 16, 61, 9, pumpBg);
    canvas.setTextSize(1);
    canvas.setTextColor(warning ? TFT_WHITE : t.muted, pumpBg);
    canvas.setCursor(18, 244);
    canvas.print(warning ? "WATER SUPPLY ALERT" : "PUMP CONTROL");
    canvas.setTextColor(warning ? TFT_WHITE : stateColor(t), pumpBg);
    canvas.setCursor(18, 260);
    canvas.print(warning ? "NO WATER RISE DETECTED" : pumpState());
    canvas.setTextColor(warning ? TFT_WHITE : t.muted, pumpBg);
    canvas.setCursor(18, 278);
    if (warning) retryLine(18, 278, t, false);
    else canvas.print(remoteConnected && sensorReadingValid ? "SENSOR ONLINE" : "SENSOR OFFLINE");

  } else {
    // Landscape: two-column instrument cluster, fully within 320x240.
    card(8, 42, 148, 126, t);
    label(18, 51, "TANK LEVEL", t);
    if (remoteConnected && sensorReadingValid && waterLevelValid) {
      canvas.setTextSize(3);
      canvas.setTextColor(t.fg, t.panel);
      canvas.setCursor(18, 70);
      canvas.printf("%.0f%%", waterLevelPercent);
      small(18, 104, String((int)waterVolumeLitres) + " / " +
          String((int)tankCapacityLitres) + " L", t.muted, t.panel);
    } else {
      small(18, 81, "WAIT SENSOR", t.warn, t.panel);
    }
    progress(18, 124, 128, 8, t);
    small(18, 143, remoteConnected && sensorReadingValid ? "LoRa ONLINE" :
        "LoRa OFFLINE", remoteConnected && sensorReadingValid ? t.ok : t.warn, t.panel);

    card(164, 42, 148, 46, t);
    label(174, 50, "CURRENT TIME", t);
    canvas.setTextSize(2);
    canvas.setTextColor(t.accent, t.panel);
    canvas.setCursor(174, 64);
    canvas.print(timeFormatHHMM());
    small(247, 72, timeNtpSynced() ? "NTP" : "--", t.muted, t.panel);

    card(164, 94, 148, 50, t);
    label(174, 101, shortCity(weather.city, 17).c_str(), t);
    if (weather.valid) {
      canvas.setTextSize(2);
      canvas.setTextColor(t.fg, t.panel);
      canvas.setCursor(174, 115);
      canvas.printf("%.0fC", weather.temperatureC);
      small(229, 123, weatherConditionLabel(weather.weatherCode), t.muted, t.panel);
    } else {
      small(174, 123, "WEATHER OFFLINE", t.warn, t.panel);
    }

    const uint16_t pumpBg = warning ? t.err : t.panel;
    canvas.fillRoundRect(164, 150, 148, 43, 9, pumpBg);
    small(174, 159, warning ? "NO WATER RISE" : pumpState(),
        warning ? TFT_WHITE : stateColor(t), pumpBg);
    if (warning) {
      const uint32_t secs = pumpRetryRemainingSeconds();
      char remaining[30];
      snprintf(remaining, sizeof(remaining), "RETRY %02lu:%02lu:%02lu",
          (unsigned long)(secs / 3600),
          (unsigned long)((secs / 60) % 60),
          (unsigned long)(secs % 60));
      small(174, 177, remaining, TFT_WHITE, pumpBg);
    } else {
      small(174, 176, manualPumpState ? "OUTPUT ON" : "OUTPUT OFF", t.muted, pumpBg);
    }

    small(15, 180, dateFormatDMY(), t.muted, t.bg);
    small(15, 196, weather.valid ? "WEATHER: UPDATED" : "WEATHER: WAITING", t.muted, t.bg);
  }
  footer(t);
  canvas.pushSprite(0, 0);
}
