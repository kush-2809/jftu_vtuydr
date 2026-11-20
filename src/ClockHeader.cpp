#include "ClockHeader.h"
#include "SmartTankState.h"
#include "Theme.h"
#include "TimeService.h"

void drawClockInHeader() {
  if (W == 0) return;
  const Theme& t = themeGet();
  const int x = 126;
  canvas.fillRect(x - 2, 3, 62, 28, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.fg, t.headerBg);
  canvas.setCursor(x, 4);
  canvas.print(timeFormatHHMM());
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.headerBg);
  canvas.setCursor(x, 23);
  canvas.print(dateFormatDMY());
}
