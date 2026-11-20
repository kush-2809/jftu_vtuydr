#include "SmartTankState.h"
#include "Theme.h"
#include "UiState.h"
#include "MenuModel.h"
#include "WiFiManager.h"
#include "FeatureControl.h"
#include "Management.h"
#include <WiFi.h>

static void drawSelectedRow(int x, int y, int w, int h) {
  const Theme& t = themeGet();
  canvas.fillRoundRect(x, y, w, h, 5, t.selBg);
  canvas.fillRect(x, y, 3, h, t.accent);
}

static String menuItemState(Page page, int index) {
  if (page == Page::Layout) return index == themeIndex() ? "ACTIVE" : "";
  if (page == Page::AdminConfig) {
    switch (menuItemAt(page, index).target) {
      case Page::AdminToggleLoRa: return featureLoraEnabled() ? "ON" : "OFF";
      case Page::AdminToggleSSR: return featureSsrEnabled() ? "ON" : "OFF";
      case Page::AdminToggleTouch: return featureTouchEnabled() ? "ON" : "OFF";
      default: return "";
    }
  }
  if (page == Page::Management && index == 0) return managementModeName(managementMode());
  return "";
}

void uiMenuDraw(const char* title, int /*unused*/) {
  const Theme& t = themeGet();
  const Page page = uiPage();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 36, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 10);
  canvas.print(title);
  canvas.fillRect(0, 36, W, 2, t.accent);

  const int n = menuItemCount(page);
  const int sel = uiMenuIndex();
  const int visible = 7;
  int first = sel >= visible ? sel - visible + 1 : 0;
  if (first + visible > n) first = n > visible ? n - visible : 0;

  for (int slot = 0; slot < visible && first + slot < n; ++slot) {
    const int index = first + slot;
    const int y = 45 + slot * 34;
    const bool selected = index == sel;
    if (selected) drawSelectedRow(8, y, W - 16, 29);

    const bool locked = page == Page::MainMenu &&
      menuItemAt(page, index).target == Page::AdminConfig && !adminUnlocked();
    canvas.setTextSize(2);
    canvas.setTextColor(selected ? t.selFg : (locked ? t.muted : t.fg),
                        selected ? t.selBg : t.bg);
    canvas.setCursor(selected ? 19 : 14, y + 7);
    canvas.print(menuItemAt(page, index).label);

    String state = menuItemState(page, index);
    if (locked) state = "LOCKED";
    if (state.length()) {
      canvas.setTextSize(1);
      canvas.setTextColor(selected ? t.selFg : t.muted, selected ? t.selBg : t.bg);
      canvas.setCursor(W - 66, y + 11);
      canvas.print(state);
    }
  }

  if (n > visible) {
    const int trackY = 46;
    const int trackH = visible * 34 - 8;
    canvas.drawFastVLine(W - 3, trackY, trackH, t.muted);
    int thumbH = (trackH * visible) / n;
    if (thumbH < 8) thumbH = 8;
    const int maxFirst = n - visible;
    const int thumbY = trackY + (maxFirst > 0 ? (trackH - thumbH) * first / maxFirst : 0);
    canvas.fillRect(W - 4, thumbY, 3, thumbH, t.accent);
  }

  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(7, H - 13);
  canvas.print("ROTATE: MOVE  PRESS: SELECT  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiPlaceholderDraw(const char* title, const char* line) {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 36, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 10);
  canvas.print(title);
  canvas.setTextSize(2);
  canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(12, 90);
  canvas.print(line);
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(7, H - 13);
  canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiSystemInfoDraw() {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 36, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 10);
  canvas.print("SYSTEM INFO");
  canvas.fillRect(0, 36, W, 2, t.accent);

  const uint64_t mac = ESP.getEfuseMac();
  char macStr[18];
  char serialStr[13];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           (uint8_t)(mac >> 40), (uint8_t)(mac >> 32),
           (uint8_t)(mac >> 24), (uint8_t)(mac >> 16),
           (uint8_t)(mac >> 8), (uint8_t)mac);
  snprintf(serialStr, sizeof(serialStr), "%012llX", (unsigned long long)(mac & 0xFFFFFFFFFFFFULL));
  const String serial(serialStr); // eFuse-derived; stable across firmware resets.

  int y = 47;
  auto line = [&](const char* key, const String& value) {
    canvas.setTextSize(1);
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(8, y);
    canvas.print(key);
    canvas.setTextColor(t.fg, t.bg);
    canvas.setCursor(93, y);
    canvas.print(value);
    y += 18;
  };

  line("Device", "SmartTank S3");
  line("Firmware", "v1.1.1");
  line("MAC", macStr);
  line("Serial", serial);
  line("IP", wifiManagerGetIP());
  line("Display", String(W) + " x " + String(H));
  line("Color depth", "RGB565 / 16-bit");
  line("CPU", String(ESP.getCpuFreqMHz()) + " MHz");
  line("Heap free", String(ESP.getFreeHeap() / 1024) + " KB");

  canvas.setTextSize(1);
  canvas.setTextColor(adminUnlocked() ? t.ok : t.warn, t.bg);
  canvas.setCursor(8, 226);
  canvas.print(adminUnlocked() ? "ADMIN ACCESS: UNLOCKED" : "ADMIN ACCESS: LOCKED");
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(8, 244);
  canvas.print("Unlock: rotate L-R-L-R, then press 5 times");

  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(7, H - 13);
  canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}
