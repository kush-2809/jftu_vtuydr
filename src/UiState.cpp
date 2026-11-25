#include "UiState.h"
#include <Arduino.h>
#include <Preferences.h>
#include "PortalAuth.h"

static Page g_stack[16];
static uint8_t g_depth = 1;
static int g_menuIndex = 0;
static int g_pendingRot = 0;
static int g_scrollOffset = 0;
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
  g_scrollOffset = 0;
  g_pendingPress = false;
  g_pendingLong = false;
  g_dirCount = 0;
  g_adminGestureArmed = false;
  g_pressCount = 0;
  g_pressWindowStart = 0;

  // Administrative unlock now requires a portal password session.
  g_admin = false;
}

Page uiPage() { return g_stack[g_depth - 1]; }
void uiPush(Page p) {
  if (g_depth < (sizeof(g_stack) / sizeof(g_stack[0]))) g_stack[g_depth++] = p;
  g_menuIndex = 0;
  g_scrollOffset = 0;
}
void uiPop() {
  if (g_depth > 1) --g_depth;
  g_menuIndex = 0;
  g_scrollOffset = 0;
}
void uiGoRoot() {
  g_depth = 1;
  g_menuIndex = 0;
  g_scrollOffset = 0;
  g_pendingRot = 0;
  g_pendingPress = false;
  g_pendingLong = false;
}
int uiScrollOffset() { return g_scrollOffset; }
void uiScrollBy(int direction, int maxOffset) {
  if (maxOffset < 0) maxOffset = 0;
  if (direction > 0) g_scrollOffset += 18;
  else if (direction < 0) g_scrollOffset -= 18;
  if (g_scrollOffset < 0) g_scrollOffset = 0;
  if (g_scrollOffset > maxOffset) g_scrollOffset = maxOffset;
}
void uiScrollReset() { g_scrollOffset = 0; }
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

// TFT configuration is authorized by a live ESP32-verified admin session.
// The previous undocumented rotary gesture and persistent unlock flag are
// intentionally disabled: neither can bypass the admin password.
bool adminUnlocked() { return portalAuthLocalAdminOpen(); }
void adminLock() { portalAuthLogout(); }
void adminGestureFeed(int /*dir*/) {}
void uiAdminPress() {}
