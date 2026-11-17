#include "ClockHeader.h"
#include "SmartTankState.h"
#include "TimeService.h"
#include <WiFi.h>

void drawClockInHeader() {
  if (W == 0) return;
  // Header is 30 px tall; put clock left of the LoRa/Wi-Fi icons
  // Icons occupy the rightmost ~60 px, so clock lives at W-130 .. W-70
  const int baseX = W - 122;
  const int baseY = 5;

  // Clear just the clock zone (so old digits disappear when time changes)
  canvas.fillRect(baseX - 2, baseY, 78, 20, TFT_NAVY);

  canvas.setTextSize(2);
  canvas.setTextColor(TFT_WHITE, TFT_NAVY);
  canvas.setCursor(baseX, baseY);
  canvas.print(timeFormatHHMM());

  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_NAVY);
  canvas.setCursor(baseX + 2, baseY + 14);
  canvas.print(dateFormatDMY().substring(0, 5));   // "DD/MM"
}