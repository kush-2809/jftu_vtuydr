#include "SmartTankState.h"
#include "Theme.h"

void drawDashboard() {
  canvas.fillSprite(TFT_BLACK);
  drawHeader();
  drawTankGraphic();

  canvas.setTextSize(1);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(118, 53); canvas.print("WATER LEVEL");

  canvas.setTextSize(3);
  canvas.setTextColor((sensorReadingValid && waterLevelValid) ? TFT_WHITE : TFT_YELLOW, TFT_BLACK);
  canvas.setCursor(118, 70);
  if (sensorReadingValid && waterLevelValid) canvas.printf("%.1f%%", waterLevelPercent);
  else canvas.print("--.-%");

  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(120, 108);
  if (sensorReadingValid && waterLevelValid)
    canvas.printf("%.0f / %.0f Litres", waterVolumeLitres, tankCapacityLitres);
  else canvas.print("Waiting for sensor...");

  const int barX = 120, barY = 124, barW = W - 132, barH = 9;
  canvas.drawRoundRect(barX, barY, barW, barH, 4, TFT_DARKGREY);
  if (sensorReadingValid && waterLevelValid) {
    float pct = constrain(waterLevelPercent, 0.0f, 100.0f);
    int fillW = (int)((barW - 4) * pct / 100.0f);
    if (fillW > 0) canvas.fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 3, TFT_CYAN);
  }

  canvas.setTextSize(1);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(120, 143); canvas.print("SENSOR DISTANCE");
  canvas.setTextSize(2);
  canvas.setTextColor(sensorReadingValid ? TFT_WHITE : TFT_YELLOW, TFT_BLACK);
  canvas.setCursor(120, 157);
  if (sensorReadingValid) canvas.printf("%.1f cm", distanceCm);
  else canvas.print("--.- cm");

  canvas.fillRoundRect(116, 183, W - 128, 32, 7, TFT_DARKGREY);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
  canvas.setCursor(126, 188); canvas.print("PUMP STATUS");
  canvas.setCursor(126, 201);
  if (manualOverrideActive) { canvas.setTextColor(TFT_YELLOW, TFT_DARKGREY); canvas.print("MANUAL OVERRIDE"); }
  else if (autoPumpState == AUTO_FILLING) { canvas.setTextColor(TFT_GREEN, TFT_DARKGREY); canvas.print("AUTO FILLING"); }
  else if (autoModeEnabled) { canvas.setTextColor(TFT_CYAN, TFT_DARKGREY); canvas.print("AUTO STANDBY"); }
  else if (manualPumpState) { canvas.setTextColor(TFT_GREEN, TFT_DARKGREY); canvas.print("MANUAL PUMP ON"); }
  else { canvas.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY); canvas.print("PUMP OFF"); }

  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13);
  canvas.print("TOUCH: OVERRIDE  |  PRESS: MENU");
  canvas.pushSprite(0, 0);
}