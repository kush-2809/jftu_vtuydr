#include "Theme.h"
#include "DisplayConfig.h"
#include <LGFX_TFT_eSPI.hpp>
#include <Preferences.h>

static const Theme kThemes[] = {
  {"Navy",   TFT_BLACK, TFT_NAVY,   TFT_WHITE, TFT_LIGHTGREY, TFT_CYAN,    TFT_BLACK,
   TFT_CYAN,  TFT_DARKGREY, TFT_GREEN, TFT_YELLOW, TFT_RED,  TFT_NAVY, TFT_DARKGREY, TFT_DARKCYAN, TFT_WHITE},
  {"Slate",  0x10A2, 0x2124, TFT_WHITE, 0x8C71, 0x07FF, TFT_BLACK,
   0x07FF, 0x39E7, 0x07E0, 0xFFE0, 0xF800, 0x18E3, 0x2124, 0x2945, TFT_WHITE},
  {"Amber",  TFT_BLACK, 0x2100, 0xFD20, 0x9A60, 0xFD20, TFT_BLACK,
   0xFD20, 0x4200, 0x07E0, 0xFFE0, 0xF800, 0x2100, 0x2100, 0x4200, TFT_BLACK},
  {"Term",   TFT_BLACK, 0x0200, 0x07E0, 0x03E0, 0x07E0, TFT_BLACK,
   0x07E0, 0x01A0, 0x07E0, 0xFFE0, 0xF800, 0x0200, 0x0200, 0x0320, TFT_BLACK},
  {"Light",  TFT_WHITE, 0xC618, TFT_BLACK, 0x7BEF, TFT_BLUE, TFT_WHITE,
   0x04BF, 0xC618, 0x0400, 0xFC00, 0xF800, 0x001F, 0xAD55, 0xBDF7, TFT_WHITE},
};
static const uint8_t kThemeCount = sizeof(kThemes) / sizeof(kThemes[0]);
static uint8_t g_theme = 0;
static Preferences g_prefs;

const Theme& themeGet() { return kThemes[g_theme]; }
uint8_t themeCount()    { return kThemeCount; }
uint8_t themeIndex()    { return g_theme; }

void themeLoad(uint8_t i) {
  if (i < kThemeCount) g_theme = i;
}

void themeSet(uint8_t i) {
  if (i >= kThemeCount || i == g_theme) return;
  g_theme = i;
  g_prefs.begin("ui", false);
  g_prefs.putUChar("theme", i);
  g_prefs.end();
}