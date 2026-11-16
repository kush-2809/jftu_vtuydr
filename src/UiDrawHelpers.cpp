#include "SmartTankState.h"
#include "Theme.h"
#include "UiState.h"
#include "MenuModel.h"
#include "WiFiManager.h"
#include <WiFi.h>

void uiMenuDraw(const char* title, int /*pageId unused; use uiPage()*/) {
  const Theme& t = themeGet();
  Page p = uiPage();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 30, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 8); canvas.print(title);

  int n = menuItemCount(p), sel = uiMenuIndex();
  int first = sel >= 6 ? sel - 5 : 0;
  if (first + 6 > n) first = n > 6 ? n - 6 : 0;

  for (int s = 0; s < 6 && first + s < n; s++) {
    int y = 40 + s * 30;
    bool selected = (first + s == sel);
    if (selected) canvas.fillRoundRect(8, y, W - 16, 26, 6, t.selBg);
    canvas.setTextSize(2);
    canvas.setTextColor(selected ? t.selFg : t.fg,
                        selected ? t.selBg : t.bg);
    canvas.setCursor(16, y + 6);
    bool locked = (p == Page::MainMenu &&
                   menuItemAt(p, first + s).target == Page::AdminConfig &&
                   !adminUnlocked());
    if (locked) canvas.print("[LOCKED] ");
    canvas.print(menuItemAt(p, first + s).label);
  }

  canvas.fillRect(0, H - 19, W, 19, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13);
  canvas.print("ROTATE: MOVE  PRESS: SELECT  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiPlaceholderDraw(const char* title, const char* line) {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 30, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 8); canvas.print(title);
  canvas.setTextSize(2);
  canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(12, 90); canvas.print(line);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(12, 130); canvas.print("Coming in next build");
  canvas.fillRect(0, H - 19, W, 19, t.footerBg);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13); canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiSystemInfoDraw() {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 30, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 8); canvas.print("SYSTEM INFO");
  canvas.setTextSize(1);

  int y = 44;
  auto line = [&](const char* k, const String& v) {
    canvas.setTextColor(t.muted, t.bg); canvas.setCursor(10, y); canvas.print(k);
    canvas.setTextColor(t.fg, t.bg);    canvas.setCursor(120, y); canvas.print(v);
    y += 18;
  };
  uint64_t mac = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%04X%04X%04X",
           (uint16_t)(mac >> 32), (uint16_t)(mac >> 16), (uint16_t)mac);
  line("Version",   "v1.0.0");
  line("Device",    "SmartTank-S3");
  line("MAC",       macStr);
  line("Serial",    String(macStr));
  line("IP",        wifiManagerGetIP());
  line("Display",   String(W) + "x" + String(H));
  line("Heap free", String(ESP.getFreeHeap() / 1024) + " KB");

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, H - 40);
  canvas.print(adminUnlocked() ? "ADMIN: UNLOCKED" : "ADMIN: LOCKED");

  canvas.fillRect(0, H - 19, W, 19, t.footerBg);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13); canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}