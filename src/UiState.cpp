#include "UiState.h"
#include <Arduino.h>
#include <Preferences.h>

static Page g_stack[16];
static uint8_t g_depth = 1;
static int  g_menuIndex = 0;

static int  g_pendingRot = 0;
static bool g_pendingPress = false;
static bool g_pendingLong  = false;

static int  g_lastDirs[8]; static uint8_t g_dirCount = 0;
static uint8_t g_pressCount = 0; static unsigned long g_pressWindowStart = 0;
static bool g_admin = false;

void uiInit() {
  g_stack[0] = Page::Dashboard;
  g_depth = 1;
  g_menuIndex = 0;
  g_pendingRot = 0; g_pendingPress = false; g_pendingLong = false;
  g_dirCount = 0; g_pressCount = 0; g_pressWindowStart = 0;
  Preferences p; p.begin("ui", true);
  g_admin = p.getBool("admin", false);
  p.end();
}

Page uiPage() { return g_stack[g_depth - 1]; }
void uiPush(Page p) { if (g_depth < 16) g_stack[g_depth++] = p; g_menuIndex = 0; }
void uiPop()  { if (g_depth > 1) g_depth--; g_menuIndex = 0; }
void uiGoRoot() { g_depth = 1; g_menuIndex = 0; }
int  uiMenuIndex() { return g_menuIndex; }
void uiSetMenuIndex(int i) { g_menuIndex = i; }

bool uiInMenu() {
  switch (uiPage()) {
    case Page::MainMenu:
    case Page::Alarm:
    case Page::Display:
    case Page::Layout:
    case Page::Management:
    case Page::Network:
    case Page::SystemClean:
    case Page::SystemRetry:
    case Page::AdminConfig:
      return true;
    default: return false;
  }
}

int  uiConsumeRotation() { int v = g_pendingRot; g_pendingRot = 0; return v; }
bool uiConsumePress()    { bool v = g_pendingPress; g_pendingPress = false; return v; }
bool uiConsumeLongPress(){ bool v = g_pendingLong;  g_pendingLong  = false; return v; }
void uiInjectRotation(int d) { g_pendingRot += d; }
void uiInjectPress()      { g_pendingPress = true; }
void uiInjectLongPress()  { g_pendingLong  = true; }

static void setAdmin(bool v) {
  g_admin = v;
  Preferences p; p.begin("ui", false); p.putBool("admin", v); p.end();
}
bool adminUnlocked() { return g_admin; }
void adminLock()     { setAdmin(false); }

void adminGestureFeed(int dir) {
  if (g_dirCount < 8) g_lastDirs[g_dirCount++] = dir;
  else { for (int i = 1; i < 8; i++) g_lastDirs[i-1] = g_lastDirs[i]; g_lastDirs[7] = dir; }
  if (g_dirCount >= 4) {
    int a = g_lastDirs[g_dirCount-4], b = g_lastDirs[g_dirCount-3],
        c = g_lastDirs[g_dirCount-2], d = g_lastDirs[g_dirCount-1];
    if (a != 0 && b == -a && c == a && d == -a) g_pressCount = 0;
  }
}

void uiAdminPress() {
  if (uiPage() != Page::SystemInfo) return;
  unsigned long now = millis();
  if (g_pressCount == 0 || now - g_pressWindowStart > 3000) {
    g_pressWindowStart = now; g_pressCount = 0;
  }
  if (++g_pressCount >= 5) { setAdmin(true); g_pressCount = 0; }
}