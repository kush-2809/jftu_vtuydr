#include "MenuModel.h"
#include "Theme.h"
#include "FeatureControl.h"
#include "PumpController.h"

void setLoraReceiverEnabled(bool enabled);

static const MenuItem kMain[] = {
  {"Telemetry", Page::Telemetry},
  {"Alarm", Page::Alarm},
  {"Display", Page::Display},
  {"Layout", Page::Layout},
  {"System Info", Page::SystemInfo},
  {"Management", Page::Management},
  {"Network", Page::Network},
  {"System Clean", Page::SystemClean},
  {"System Retry", Page::SystemRetry},
  {"Configuration", Page::AdminConfig},
};
static const MenuItem kAlarm[] = {
  {"Pump Timer", Page::AlarmTimer},
  {"Daily Scheduler", Page::AlarmScheduler},
  {"Sleep Window", Page::AlarmSleep},
};
static const MenuItem kDisplay[] = {
  {"Screen Timeout", Page::DisplayTimeout},
  {"Dim Step", Page::DisplayDim},
  {"Date & Time", Page::DisplayDateTime},
};
static const MenuItem kLayout[] = {
  {"Navy", Page::Layout},
  {"Slate", Page::Layout},
  {"Amber", Page::Layout},
  {"Terminal", Page::Layout},
  {"Light", Page::Layout},
};
static const MenuItem kManagement[] = {
  {"Operating Mode", Page::ManagementMode},
  {"Buzzer Volume", Page::ManagementBuzzer},
  {"Screen Rotation", Page::ManagementRotation},
};
static const MenuItem kNetwork[] = {
  {"Link Diagnostics", Page::NetworkStatus},
 {"Wi-Fi / IP Setup", Page::NetworkWiFi},
};
static const MenuItem kClean[] = {
  {"Reboot Device", Page::CleanReboot},
  {"Clear Diagnostics", Page::CleanClearCache},
  {"Factory Reset", Page::CleanFactoryReset},
};
static const MenuItem kAdmin[] = {
  {"Tank Height", Page::AdminTankHeight},
  {"Tank Capacity", Page::AdminTankCapacity},
  {"Auto Thresholds", Page::AdminThresholds},
  {"LoRa Receiver", Page::AdminToggleLoRa},
  {"SSR Output", Page::AdminToggleSSR},
  {"Touch Input", Page::AdminToggleTouch},
  {"Lock Admin", Page::AdminLock},
};

int menuItemCount(Page p) {
  switch (p) {
    case Page::MainMenu: return sizeof(kMain) / sizeof(kMain[0]);
    case Page::Alarm: return sizeof(kAlarm) / sizeof(kAlarm[0]);
    case Page::Display: return sizeof(kDisplay) / sizeof(kDisplay[0]);
    case Page::Layout: return sizeof(kLayout) / sizeof(kLayout[0]);
    case Page::Management: return sizeof(kManagement) / sizeof(kManagement[0]);
    case Page::Network: return sizeof(kNetwork) / sizeof(kNetwork[0]);
    case Page::SystemClean: return sizeof(kClean) / sizeof(kClean[0]);
    case Page::AdminConfig: return sizeof(kAdmin) / sizeof(kAdmin[0]);
    default: return 0;
  }
}

const MenuItem& menuItemAt(Page p, int i) {
  static const MenuItem invalid = {"Unavailable", Page::Dashboard};
  const MenuItem* items = nullptr;
  int count = 0;
  switch (p) {
    case Page::MainMenu: items = kMain; count = sizeof(kMain) / sizeof(kMain[0]); break;
    case Page::Alarm: items = kAlarm; count = sizeof(kAlarm) / sizeof(kAlarm[0]); break;
    case Page::Display: items = kDisplay; count = sizeof(kDisplay) / sizeof(kDisplay[0]); break;
    case Page::Layout: items = kLayout; count = sizeof(kLayout) / sizeof(kLayout[0]); break;
    case Page::Management: items = kManagement; count = sizeof(kManagement) / sizeof(kManagement[0]); break;
    case Page::Network: items = kNetwork; count = sizeof(kNetwork) / sizeof(kNetwork[0]); break;
    case Page::SystemClean: items = kClean; count = sizeof(kClean) / sizeof(kClean[0]); break;
    case Page::AdminConfig: items = kAdmin; count = sizeof(kAdmin) / sizeof(kAdmin[0]); break;
    default: return invalid;
  }
  if (i < 0 || i >= count) return invalid;
  return items[i];
}

void menuActivate(Page p, int i) {
  const int count = menuItemCount(p);
  if (i < 0 || i >= count) return;
  if (p == Page::Layout) {
    themeSet(static_cast<uint8_t>(i));
    return;
  }
  if (p == Page::AdminConfig && !adminUnlocked()) return;

  const Page target = menuItemAt(p, i).target;
  if (target == Page::AdminConfig && !adminUnlocked()) return;
  switch (target) {
    case Page::AdminToggleLoRa:
      setLoraReceiverEnabled(!featureLoraEnabled());
      return;
    case Page::AdminToggleSSR:
      featureSetSsrEnabled(!featureSsrEnabled());
      return;
    case Page::AdminToggleTouch:
      featureSetTouchEnabled(!featureTouchEnabled());
      return;
    case Page::AdminLock:
      adminLock();
      uiPop();
      return;
    default:
      uiPush(target);
      return;
  }
}
