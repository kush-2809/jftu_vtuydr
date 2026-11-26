#pragma once
#include <stdint.h>

enum class Page : uint8_t {
  Dashboard, MainMenu,
  Telemetry,
  Alarm, AlarmTimer, AlarmScheduler, AlarmSleep,
  Display, DisplayTimeout, DisplayDim, DisplayDateTime, DisplayNTP,
  Layout, SystemInfo, Management, Network, SystemClean,
  SystemRetry, AdminConfig,
  ManagementMode, ManagementBuzzer, ManagementRotation,
  ManagementThresholds, ManagementOverride,
  NetworkStatus, NetworkWiFi,
  CleanReboot, CleanClearCache, CleanFactoryReset,
  AdminTankHeight, AdminTankCapacity, AdminThresholds,
  AdminToggleLoRa, AdminToggleSSR, AdminToggleTouch, AdminLock
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
int uiScrollOffset();
void uiScrollBy(int direction, int maxOffset);
void uiScrollReset();
void uiDrawScrollIndicator(int maxOffset);

// Admin gate
void adminGestureFeed(int dir);
void uiAdminPress();
bool adminUnlocked();
void adminLock();