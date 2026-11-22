#include "ClockHeader.h"
#include "SmartTankState.h"
#include "Theme.h"
#include "TimeService.h"

void drawDashboardClock() {
  if (W <= 0 || H <= 0) return;

  const Theme& t = themeGet();
  const bool landscape = W > H;

  const int x = landscape ? 100 : 91;
  const int y = landscape ? 43 : 46;
  const int w = W - x - 8;
  const int h = landscape ? 51 : 66;

  // Clock background card
  canvas.fillRoundRect(x, y, w, h, 7, t.panel);
  canvas.drawRoundRect(x, y, w, h, 7, t.muted);

  // Large clock
  canvas.setTextSize(landscape ? 2 : 3);
  canvas.setTextColor(t.accent, t.panel);
  canvas.setCursor(x + 9, y + 5);
  canvas.print(timeFormatHHMM());

  // Date
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.panel);
  canvas.setCursor(x + 9, y + (landscape ? 29 : 40));
  canvas.print(dateFormatDMY());

  // Clock synchronization status
  const char* clockStatus =
      timeNtpSynced() ? "NTP SYNCED" :
      timeIsValid()   ? "CLOCK READY" :
                        "SYNC WAIT";

  const uint16_t statusColor =
      timeNtpSynced() ? t.ok :
      timeIsValid()   ? t.accent :
                        t.warn;

  canvas.setTextColor(statusColor, t.panel);

  if (landscape) {
    canvas.setCursor(x + 125, y + 31);
  } else {
    canvas.setCursor(x + 9, y + 54);
  }

  canvas.print(clockStatus);
}