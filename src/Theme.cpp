#include "Theme.h"
#include "DisplayConfig.h"
#include <LGFX_TFT_eSPI.hpp>
#include <Preferences.h>

static const Theme kThemes[] = {
  // Name, background, card, foreground, secondary text, accent, accent text,
  // water, empty tank, success, warning, error, header, footer, selected, selected text.
  {"Midnight", 0x0841, 0x10E4, 0xF7DE, 0x9C71, 0x36FA, 0x0021,
   0x2DB9, 0x18C6, 0x45CF, 0xFDE0, 0xF9A8, 0x0841, 0x0841, 0x1A49, 0xFFFF},
  {"Slate", 0x1082, 0x2145, 0xFFFF, 0xA514, 0x67FF, 0x0000,
   0x4DDF, 0x3148, 0x4E6B, 0xFE40, 0xF9A8, 0x1082, 0x1082, 0x31A8, 0xFFFF},
  {"Amber", 0x1040, 0x20C1, 0xFFBA, 0xC5A9, 0xFD20, 0x0800,
   0xFC80, 0x3163, 0x5EAC, 0xFDE0, 0xF987, 0x1040, 0x1040, 0x39A3, 0xFFFF},
  {"Terminal", 0x0000, 0x0200, 0x07E0, 0x04C0, 0x07E0, 0x0000,
   0x07E0, 0x0140, 0x07E0, 0xFFE0, 0xF800, 0x0000, 0x0000, 0x03E0, 0x0000},
  {"Daylight", 0xF7DE, 0xFFFF, 0x18A4, 0x6B6D, 0x05D5, 0xFFFF,
   0x055B, 0xDEFB, 0x04C8, 0xBC80, 0xD9A5, 0xE71C, 0xE71C, 0xC69B, 0x0841}
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