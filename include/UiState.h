#pragma once
#include <stdint.h>

enum class Page : uint8_t {
  Dashboard, MainMenu,
  Telemetry,
  Alarm, AlarmTimer, AlarmScheduler, AlarmSleep,
  Display, DisplayTimeout, DisplayDim, DisplayDateTime, DisplayNTP,
  Layout, SystemInfo, Management, Network, SystemClean,
  SystemRetry, AdminConfig
};

void uiInit();
Page uiPage();
bool uiInMenu();
int  uiMenuIndex();
void uiPush(Page p);
void uiPop();
void uiGoRoot();
void uiSetMenuIndex(int i);

// Rotation/press consumption used by per-page tickers
int  uiConsumeRotation();     // returns -1/0/+1
bool uiConsumePress();        // true once
bool uiConsumeLongPress();    // true once
void uiInjectRotation(int d); // called from encoder handler
void uiInjectPress();
void uiInjectLongPress();

// Admin gate
void adminGestureFeed(int dir);
void uiAdminPress();
bool adminUnlocked();
void adminLock();