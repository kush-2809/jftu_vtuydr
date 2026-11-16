#pragma once
#include <stdint.h>

struct Theme {
  const char* name;
  uint16_t bg, panel, fg, muted, accent, accentText;
  uint16_t tankWater, tankEmpty, ok, warn, err;
  uint16_t headerBg, footerBg, selBg, selFg;
};

const Theme& themeGet();
void themeSet(uint8_t index);
void themeLoad(uint8_t index);
uint8_t themeCount();
uint8_t themeIndex();