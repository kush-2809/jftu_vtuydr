#include "SmartTankState.h"
#include "Theme.h"
#include "PumpController.h"
#include "Management.h"
#include "TimeService.h"
#include "WeatherService.h"
#include <math.h>

// Fixed TFT-safe regions:
//   portrait  240x320: status 0..40, time/weather 45..88,
//                      tank 94..237, pump 244..298, footer 300..319
//   landscape 320x240: status 0..40, hero 45..181,
//                      pump 187..219, footer 220..239
namespace {
void text(int x, int y, const String& str, int size, uint16_t fg, uint16_t bg) {
  canvas.setTextSize(size);
  canvas.setTextColor(fg, bg);
  canvas.setCursor(x, y);
  canvas.print(str);
}
void card(int x, int y, int w, int h, const Theme& t) {
  canvas.fillRoundRect(x, y, w, h, 10, t.panel);
  canvas.drawRoundRect(x, y, w, h, 10, t.selBg);
}
String shorten(const String& str, unsigned max) {
  if (str.length() <= max) return str;
  return str.substring(0, max - 2) + "..";
}
bool sensorOnline() {
  return remoteConnected && sensorReadingValid && waterLevelValid;
}
bool fillingNow() {
  return manualPumpState || manualOverrideActive || autoPumpState == AUTO_FILLING;
}
void drawWaterTank(int x, int y, int w, int h, const Theme& t) {
  const int top = y + 7;
  const int bottom = y + h - 5;
  const int innerX = x + 5, innerW = w - 10;
  const int innerTop = top + 5, innerBottom = bottom - 5;
  const int innerH = innerBottom - innerTop;

  // Raised lid, smooth casing and glassy interior.
  canvas.fillRoundRect(x + 9, y + 1, w - 18, 8, 3, t.accent);
  canvas.fillRoundRect(x, top, w, bottom - top + 1, 12, t.muted);
  canvas.fillRoundRect(x + 3, top + 3, w - 6, bottom - top - 5, 9, t.tankEmpty);
  canvas.drawRoundRect(x, top, w, bottom - top + 1, 12, t.selBg);

  if (sensorOnline()) {
    const float pct = constrain(waterLevelPercent, 0.0f, 100.0f);
    const int fill = (int)(innerH * pct / 100.0f);
    if (fill > 0) {
      // A moving 2px water surface gives continuous visual feedback without
      // modifying the actual measured level or causing fictitious sensor rise.
      const float phase = (float)(millis() % 10000UL) * 0.0065f;
      const int baseY = innerBottom - fill;
      for (int px = innerX; px < innerX + innerW; ++px) {
        const int wave = fill > 3 ?
          (int)roundf(1.6f * sinf(phase + (px - innerX) * 0.17f)) : 0;
        int surface = baseY + wave;
        if (surface < innerTop) surface = innerTop;
        if (surface > innerBottom) surface = innerBottom;
        canvas.drawFastVLine(px, surface, innerBottom - surface + 1, t.tankWater);
        if (fill > 3 && ((px - innerX) & 3) == 0)
          canvas.drawPixel(px, surface, t.accent);
      }
      // Glass reflection, kept within liquid bounds.
      if (fill > 10)
        canvas.drawFastVLine(innerX + 5, baseY + 5, fill - 9, t.accent);
    }
  }

  // Tick marks in the frame, independent of the actual water level.
  for (int mark = 1; mark <= 3; ++mark) {
    int yy = innerTop + (innerH * mark) / 4;
    canvas.drawFastHLine(x + w - 12, yy, 6, t.fg);
  }
  canvas.fillRoundRect(x + w / 2 - 12, bottom, 24, 5, 2, t.accent);
}
const char* pumpLabel() {
  if (manualOverrideActive) return "MANUAL RUN";
  if (autoPumpState == AUTO_FILLING)
    return alarmFillSessionActive() ? "SCHEDULE FILL" : "AUTO FILL";
  if (autoPumpState == AUTO_RETRY_WAIT) return "RETRY WAIT";
  if (autoPumpState == AUTO_WAIT_SENSOR) return "WAIT SENSOR";
  if (manualPumpState) return "PUMP ON";
  return managementAllowsAuto() ? "AUTO READY" : "PUMP OFF";
}
uint16_t pumpTint(const Theme& t) {
  if (pumpRetryWarningActive()) return t.err;
  if (autoPumpState == AUTO_RETRY_WAIT || manualOverrideActive) return t.warn;
  if (fillingNow()) return t.ok;
  return t.accent;
}
String retryLabel(bool compact) {
  const uint32_t seconds = pumpRetryRemainingSeconds();
  char buf[48];
  if (pumpRetryManualTimeEnabled()) {
    if (compact)
      snprintf(buf, sizeof(buf), "AT %02u:%02u  %02lu:%02lu",
          pumpRetryManualHour(), pumpRetryManualMinute(),
          (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
    else
      snprintf(buf, sizeof(buf), "SCHEDULE %02u:%02u  IN %02lu:%02lu",
          pumpRetryManualHour(), pumpRetryManualMinute(),
          (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
  } else
    snprintf(buf, sizeof(buf), compact ? "RETRY IN %02lu:%02lu" : "AUTO RETRY IN %02lu:%02lu",
        (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
  return String(buf);
}
void footer(const Theme& t) {
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.drawFastHLine(0, H - 20, W, t.selBg);
  text(8, H - 13, "PRESS MENU   TURN BRIGHTNESS", 1, t.fg, t.footerBg);
}
void drawPortrait(const Theme& t, const WeatherSnapshot& weather) {
  card(8, 46, 224, 42, t);
  text(17, 53, "LOCAL TIME", 1, t.muted, t.panel);
  text(16, 65, timeFormatHHMM(), 2, t.accent, t.panel);
  canvas.drawFastVLine(109, 53, 28, t.selBg);
  text(121, 52, shorten(String(weather.city[0] ? weather.city : "SET CITY"), 15),
      1, t.muted, t.panel);
  if (weather.valid) {
    char temp[20]; snprintf(temp, sizeof(temp), "%.0f C", weather.temperatureC);
    text(121, 66, temp, 2, t.fg, t.panel);
    text(177, 76, shorten(String(weatherConditionLabel(weather.weatherCode)), 8),
        1, t.accent, t.panel);
  } else text(121, 68, "WEATHER --", 1, t.warn, t.panel);

  card(8, 95, 224, 142, t);
  text(18, 104, "LIVE WATER LEVEL", 1, t.muted, t.panel);
  drawWaterTank(23, 120, 75, 103, t);
  text(111, 128, "WATER LEVEL", 1, t.muted, t.panel);
  if (sensorOnline()) {
    char percent[16]; snprintf(percent, sizeof(percent), "%.0f%%", waterLevelPercent);
    text(108, 147, percent, 3, t.fg, t.panel);
    char litres[30];
    snprintf(litres, sizeof(litres), "%.0f/%.0f L",
        waterVolumeLitres, tankCapacityLitres);
    text(111, 185, litres, 1, t.muted, t.panel);
  } else {
    text(108, 148, "--%", 3, t.warn, t.panel);
    text(111, 186, "SENSOR WAIT", 1, t.warn, t.panel);
  }
  text(111, 207, remoteConnected ? "LoRa CONNECTED" : "LoRa DISCONNECTED",
      1, remoteConnected ? t.ok : t.err, t.panel);

  const bool warn = pumpRetryWarningActive();
  const uint16_t bg = warn ? t.err : t.panel;
  canvas.fillRoundRect(8, 244, 224, 55, 9, bg);
  text(17, 250, warn ? "WATER SUPPLY ALERT" : "PUMP STATUS",
      1, warn ? TFT_WHITE : t.muted, bg);
  text(17, 266, warn ? "NO WATER RISE" : pumpLabel(), 2,
      warn ? TFT_WHITE : pumpTint(t), bg);
  text(17, 287, warn ? shorten(retryLabel(false), 33) :
      (fillingNow() ? "PUMP ACTIVE" : "PUMP STANDBY"), 1,
      warn ? TFT_WHITE : t.muted, bg);
}
void drawLandscape(const Theme& t, const WeatherSnapshot& weather) {
  card(8, 46, 154, 134, t);
  text(18, 55, "LIVE WATER LEVEL", 1, t.muted, t.panel);
  drawWaterTank(20, 68, 66, 101, t);
  if (sensorOnline()) {
    char pct[16]; snprintf(pct, sizeof(pct), "%.0f%%", waterLevelPercent);
    text(94, 85, pct, 2, t.fg, t.panel);
    char litres[20]; snprintf(litres, sizeof(litres), "%.0f L", waterVolumeLitres);
    text(94, 112, litres, 1, t.muted, t.panel);
  } else text(94, 86, "--%", 2, t.warn, t.panel);
  text(94, 145, remoteConnected ? "LoRa OK" : "LoRa X", 1,
      remoteConnected ? t.ok : t.err, t.panel);

  card(170, 46, 142, 61, t);
  text(181, 54, "LOCAL TIME", 1, t.muted, t.panel);
  text(180, 70, timeFormatHHMM(), 2, t.accent, t.panel);
  text(263, 81, timeNtpSynced() ? "NTP" : "WAIT", 1, t.muted, t.panel);

  card(170, 114, 142, 66, t);
  text(181, 123, shorten(String(weather.city[0] ? weather.city : "SET CITY"), 17),
      1, t.muted, t.panel);
  if (weather.valid) {
    char temperature[20];
    snprintf(temperature, sizeof(temperature), "%.0f C", weather.temperatureC);
    text(181, 139, temperature, 2, t.fg, t.panel);
    text(181, 163, weatherConditionLabel(weather.weatherCode), 1, t.accent, t.panel);
  } else text(181, 144, "WEATHER OFFLINE", 1, t.warn, t.panel);

  const bool warn = pumpRetryWarningActive();
  const uint16_t bg = warn ? t.err : t.panel;
  canvas.fillRoundRect(8, 188, 304, 30, 8, bg);
  text(17, 196, warn ? "NO WATER RISE" : pumpLabel(), 1,
      warn ? TFT_WHITE : pumpTint(t), bg);
  text(155, 196, warn ? retryLabel(true) :
      (fillingNow() ? "PUMP ACTIVE" : "PUMP STANDBY"), 1,
      warn ? TFT_WHITE : t.muted, bg);
}
} // namespace

void drawDashboard() {
  if (!displayReady || W <= 0 || H <= 0) return;
  const Theme& t = themeGet();
  const WeatherSnapshot weather = weatherServiceSnapshot();
  canvas.fillSprite(t.bg);
  drawHeader();
  if (W > H) drawLandscape(t, weather);
  else drawPortrait(t, weather);
  footer(t);
  canvas.pushSprite(0, 0);
}
