#include "SmartTankState.h"
#include "Theme.h"
#include <WiFi.h>
#include <math.h>
#include "WiFiManager.h"

int getSignalLevel(int rssi) {
  if (rssi >= -70) return 4;
  if (rssi >= -85) return 3;
  if (rssi >= -100) return 2;
  if (rssi >= -115) return 1;
  return 0;
}
uint16_t getSignalColor(int level) {
  if (level >= 3) return TFT_GREEN;
  if (level == 2) return TFT_YELLOW;
  return TFT_RED;
}
void drawSignalBars(int x, int y, int level, uint16_t activeColor) {
  const int barWidth = 3, gap = 2;
  const int heights[] = {5, 8, 11, 14};
  for (int i = 0; i < 4; i++) {
    int barHeight = heights[i];
    uint16_t color = i < level ? activeColor : TFT_DARKGREY;
    canvas.fillRect(x + i * (barWidth + gap), y + 15 - barHeight,
                    barWidth, barHeight, color);
  }
}
void drawWifiArc(int cx, int cy, int radius, uint16_t color) {
  float previousX = cx + radius * cos(225.0f * PI / 180.0f);
  float previousY = cy + radius * sin(225.0f * PI / 180.0f);
  for (int angle = 230; angle <= 315; angle += 5) {
    float rad = angle * PI / 180.0f;
    float currentX = cx + radius * cos(rad);
    float currentY = cy + radius * sin(rad);
    canvas.drawLine((int)previousX, (int)previousY,
                    (int)currentX, (int)currentY, color);
    previousX = currentX; previousY = currentY;
  }
}
void drawWifiIcon(int cx, int cy, int level) {
  uint16_t active = level >= 3 ? TFT_GREEN : level >= 2 ? TFT_YELLOW : TFT_RED;
  uint16_t inactive = TFT_DARKGREY;
  drawWifiArc(cx, cy, 12, level >= 1 ? active : inactive);
  drawWifiArc(cx, cy, 8,  level >= 2 ? active : inactive);
  drawWifiArc(cx, cy, 4,  level >= 3 ? active : inactive);
  canvas.fillCircle(cx, cy + 1, 2, level > 0 ? active : inactive);
}
void drawHeader() {
  canvas.fillRect(0, 0, W, 30, TFT_NAVY);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_WHITE, TFT_NAVY);
  canvas.setCursor(7, 8);
  canvas.print("SMART TANK");
  int loraLevel = remoteConnected ? getSignalLevel(lastRSSI) : 0;
  drawSignalBars(W - 58, 7, loraLevel,
                 remoteConnected ? getSignalColor(loraLevel) : TFT_DARKGREY);
  int wifiLevel = 0;
  if (WiFi.status() == WL_CONNECTED) wifiLevel = getSignalLevel(WiFi.RSSI());
  drawWifiIcon(W - 17, 14, wifiLevel);
}
void drawTankGraphic() {
  const int tankX = 14, tankY = 58, tankW = 88, tankH = 140;
  const int innerX = tankX + 9, innerY = tankY + 9;
  const int innerW = tankW - 18, innerH = tankH - 18;
  canvas.fillRoundRect(innerX, innerY, innerW, innerH, 6, TFT_DARKGREY);
  int fillHeight = 0;
  if (sensorReadingValid && waterLevelValid) {
    float pct = constrain(waterLevelPercent, 0.0f, 100.0f);
    fillHeight = (int)(innerH * pct / 100.0f);
  }
  if (fillHeight > 0) {
    int waterY = innerY + innerH - fillHeight;
    canvas.fillRoundRect(innerX + 2, waterY, innerW - 4, fillHeight, 5, TFT_CYAN);
    if (fillHeight > 8)
      canvas.drawFastHLine(innerX + 8, waterY + 3, innerW - 16, TFT_WHITE);
  }
  canvas.drawRoundRect(tankX, tankY, tankW, tankH, 10, TFT_LIGHTGREY);
  canvas.drawRoundRect(tankX + 24, tankY - 5, tankW - 48, 9, 3, TFT_LIGHTGREY);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(tankX + 29, tankY + tankH + 7);
  canvas.print("TANK");
}