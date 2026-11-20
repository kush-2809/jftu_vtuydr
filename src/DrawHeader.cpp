#include "SmartTankState.h"
#include "Theme.h"
#include <WiFi.h>
#include <math.h>
#include "WiFiManager.h"
#include "ClockHeader.h"
#include "PumpController.h"

int getSignalLevel(int rssi) {
  if (rssi >= -70) return 4;
  if (rssi >= -85) return 3;
  if (rssi >= -100) return 2;
  if (rssi >= -115) return 1;
  return 0;
}

uint16_t getSignalColor(int level) {
  const Theme& t = themeGet();
  if (level >= 3) return t.ok;
  if (level == 2) return t.warn;
  return t.err;
}

void drawSignalBars(int x, int y, int level, uint16_t activeColor) {
  const Theme& t = themeGet();
  const int barWidth = 3, gap = 2;
  const int heights[] = {5, 8, 11, 14};
  for (int i = 0; i < 4; ++i) {
    const int h = heights[i];
    const uint16_t color = i < level ? activeColor : t.muted;
    canvas.fillRect(x + i * (barWidth + gap), y + 15 - h, barWidth, h, color);
  }
}

void drawWifiArc(int cx, int cy, int radius, uint16_t color) {
  float px = cx + radius * cosf(225.0f * PI / 180.0f);
  float py = cy + radius * sinf(225.0f * PI / 180.0f);
  for (int angle = 230; angle <= 315; angle += 5) {
    const float rad = angle * PI / 180.0f;
    const float x = cx + radius * cosf(rad);
    const float y = cy + radius * sinf(rad);
    canvas.drawLine((int)px, (int)py, (int)x, (int)y, color);
    px = x;
    py = y;
  }
}

void drawWifiIcon(int cx, int cy, int level) {
  const Theme& t = themeGet();
  const uint16_t active = level >= 3 ? t.ok : level >= 2 ? t.warn : t.err;
  drawWifiArc(cx, cy, 11, active);
  drawWifiArc(cx, cy, 7, active);
  drawWifiArc(cx, cy, 3, active);
  canvas.fillCircle(cx, cy + 1, 2, active);
}

static uint16_t headerStateColor() {
  const Theme& t = themeGet();
  if (pumpRetryWarningActive()) return t.err;
  if (!sensorReadingValid || !waterLevelValid) return t.warn;
  if (manualOverrideActive) return t.warn;
  if (autoPumpState == AUTO_FILLING || manualPumpState) return t.ok;
  if (autoModeEnabled) return t.accent;
  return t.muted;
}

void drawHeader() {
  const Theme& t = themeGet();
  canvas.fillRect(0, 0, W, 36, t.headerBg);

  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(7, 10);
  canvas.print("SMART TANK");

  drawClockInHeader();

  // LoRa remains visible even when disconnected; muted bars indicate no link.
  const int loraLevel = remoteConnected ? getSignalLevel(lastRSSI) : 0;
  drawSignalBars(W - 43, 9, loraLevel,
                 remoteConnected ? getSignalColor(loraLevel) : t.muted);

  // Hide Wi-Fi entirely when not connected so the status area stays clean.
  if (wifiManagerIsConnected()) {
    drawWifiIcon(W - 10, 17, getSignalLevel(wifiManagerGetRSSI()));
  }
  canvas.fillRect(0, 36, W, 2, headerStateColor());
}

void drawTankGraphic() {
  const Theme& t = themeGet();
  const int tankX = 14, tankY = 62, tankW = 88, tankH = 142;
  const int innerX = tankX + 8, innerY = tankY + 8;
  const int innerW = tankW - 16, innerH = tankH - 16;

  canvas.fillRoundRect(innerX, innerY, innerW, innerH, 6, t.tankEmpty);

  int fillHeight = 0;
  if (sensorReadingValid && waterLevelValid) {
    const float pct = constrain(waterLevelPercent, 0.0f, 100.0f);
    fillHeight = (int)(innerH * pct / 100.0f);
  }
  if (fillHeight > 0) {
    const int waterY = innerY + innerH - fillHeight;
    canvas.fillRect(innerX + 2, waterY + 3, innerW - 4, fillHeight - 2, t.tankWater);

    // Animate the surface only while the pump is actually running. When
    // idle, keep a clean static waterline so animation communicates state.
    const bool filling = manualPumpState || manualOverrideActive ||
                         autoPumpState == AUTO_FILLING;
    if (fillHeight >= 8 && filling) {
      const float phase = millis() / 420.0f;
      for (int x = innerX + 3; x < innerX + innerW - 3; x += 2) {
        const int waveY = waterY + 2 + (int)(2.0f * sinf(phase + x * 0.16f));
        canvas.drawPixel(x, waveY, t.fg);
      }
    } else {
      canvas.drawFastHLine(innerX + 3, waterY + 2, innerW - 6, t.fg);
    }
  }

  canvas.drawRoundRect(tankX, tankY, tankW, tankH, 10, t.fg);
  canvas.drawRoundRect(tankX + 24, tankY - 5, tankW - 48, 9, 3, t.fg);

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(tankX + 31, tankY + tankH + 8);
  canvas.print("TANK");

}
