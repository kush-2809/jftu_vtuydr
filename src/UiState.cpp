#include "UiState.h"
#include <Arduino.h>
#include <Preferences.h>

static Page g_stack[16];
static uint8_t g_depth = 1;
static int g_menuIndex = 0;
static int g_pendingRot = 0;
static bool g_pendingPress = false;
static bool g_pendingLong = false;

static int g_lastDirs[4] = {0, 0, 0, 0};
static uint8_t g_dirCount = 0;
static bool g_adminGestureArmed = false;
static uint8_t g_pressCount = 0;
static unsigned long g_pressWindowStart = 0;
static bool g_admin = false;

void uiInit() {
  g_stack[0] = Page::Dashboard;
  g_depth = 1;
  g_menuIndex = 0;
  g_pendingRot = 0;
  g_pendingPress = false;
  g_pendingLong = false;
  g_dirCount = 0;
  g_adminGestureArmed = false;
  g_pressCount = 0;
  g_pressWindowStart = 0;

  Preferences p;
  p.begin("ui", true);
  g_admin = p.getBool("admin", false);
  p.end();
}

Page uiPage() { return g_stack[g_depth - 1]; }
void uiPush(Page p) {
  if (g_depth < (sizeof(g_stack) / sizeof(g_stack[0]))) {
    g_stack[g_depth++] = p;
  }
  g_menuIndex = 0;
}
void uiPop() {
  if (g_depth > 1) --g_depth;
  g_menuIndex = 0;
}
void uiGoRoot() { g_depth = 1; g_menuIndex = 0; }
int uiMenuIndex() { return g_menuIndex; }
void uiSetMenuIndex(int i) { g_menuIndex = i < 0 ? 0 : i; }

bool uiInMenu() {
  switch (uiPage()) {
    case Page::MainMenu:
    case Page::Alarm:
    case Page::Display:
    case Page::Layout:
    case Page::Management:
    case Page::Network:
    case Page::SystemClean:
    case Page::AdminConfig:
      return true;
    default:
      return false;
  }
}

int uiConsumeRotation() {
  int v = g_pendingRot;
  g_pendingRot = 0;
  return v;
}
bool uiConsumePress() {
  bool v = g_pendingPress;
  g_pendingPress = false;
  return v;
}
bool uiConsumeLongPress() {
  bool v = g_pendingLong;
  g_pendingLong = false;
  return v;
}
void uiInjectRotation(int d) { g_pendingRot += d; }
void uiInjectPress() { g_pendingPress = true; }
void uiInjectLongPress() { g_pendingLong = true; }

static void setAdmin(bool enabled) {
  g_admin = enabled;
  Preferences p;
  p.begin("ui", false);
  p.putBool("admin", enabled);
  p.end();
}

bool adminUnlocked() { return g_admin; }
void adminLock() {
  setAdmin(false);
  g_adminGestureArmed = false;
  g_pressCount = 0;
}

void adminGestureFeed(int dir) {
  if (uiPage() != Page::SystemInfo || g_admin) return;
  if (dir == 0) return;
  if (g_dirCount < 4) {
    g_lastDirs[g_dirCount++] = dir;
  } else {
    for (int i = 1; i < 4; ++i) g_lastDirs[i - 1] = g_lastDirs[i];
    g_lastDirs[3] = dir;
  }

  if (g_dirCount == 4 &&
      g_lastDirs[0] == -g_lastDirs[1] &&
      g_lastDirs[0] == g_lastDirs[2] &&
      g_lastDirs[1] == g_lastDirs[3]) {
    g_adminGestureArmed = true;
    g_pressCount = 0;
    g_pressWindowStart = millis();
  }
}

void uiAdminPress() {
  if (uiPage() != Page::SystemInfo || g_admin || !g_adminGestureArmed) return;
  const unsigned long now = millis();
  if (g_pressCount == 0 || now - g_pressWindowStart > 5000) {
    g_pressWindowStart = now;
    g_pressCount = 0;
  }
  if (++g_pressCount >= 5) {
    setAdmin(true);
    g_adminGestureArmed = false;
    g_pressCount = 0;
  }
}
