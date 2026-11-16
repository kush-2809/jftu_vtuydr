#include "MenuModel.h"
#include "Theme.h"

static const MenuItem kMain[] = {
  {"Telemetry",     Page::Telemetry},
  {"Alarm",         Page::Alarm},
  {"Display",       Page::Display},
  {"Layout",        Page::Layout},
  {"System Info",   Page::SystemInfo},
  {"Management",    Page::Management},
  {"Network",       Page::Network},
  {"System Clean",  Page::SystemClean},
  {"System Retry",  Page::SystemRetry},
  {"Configuration", Page::AdminConfig},
};
static const MenuItem kAlarm[] = {
  {"Timer",     Page::AlarmTimer},
  {"Scheduler", Page::AlarmScheduler},
  {"Sleep",     Page::AlarmSleep},
};
static const MenuItem kDisplay[] = {
  {"Timeout",     Page::DisplayTimeout},
  {"Dim %",       Page::DisplayDim},
  {"Date & Time", Page::DisplayDateTime},
};
static const MenuItem kLayout[] = {
  {"Navy",  Page::Layout}, {"Slate", Page::Layout},
  {"Amber", Page::Layout}, {"Term",  Page::Layout}, {"Light", Page::Layout},
};
static const MenuItem kMgmt[] = {
  {"Mode",   Page::Management}, {"Buzzer", Page::Management},
};
static const MenuItem kNet[] = {
  {"LoRa", Page::Network}, {"Ultrasonic", Page::Network},
  {"Wi-Fi", Page::Network}, {"Change Wi-Fi", Page::Network},
};
static const MenuItem kClean[] = {
  {"Reboot", Page::SystemClean}, {"Clear Cache", Page::SystemClean},
  {"Factory Reset (keep serial)", Page::SystemClean},
};
static const MenuItem kRetry[] = { {"Retry Policy", Page::SystemRetry} };

int menuItemCount(Page p) {
  switch (p) {
    case Page::MainMenu:    return sizeof(kMain)/sizeof(MenuItem);
    case Page::Alarm:       return sizeof(kAlarm)/sizeof(MenuItem);
    case Page::Display:     return sizeof(kDisplay)/sizeof(MenuItem);
    case Page::Layout:      return sizeof(kLayout)/sizeof(MenuItem);
    case Page::Management:  return sizeof(kMgmt)/sizeof(MenuItem);
    case Page::Network:     return sizeof(kNet)/sizeof(MenuItem);
    case Page::SystemClean: return sizeof(kClean)/sizeof(MenuItem);
    case Page::SystemRetry: return sizeof(kRetry)/sizeof(MenuItem);
    default: return 0;
  }
}

const MenuItem& menuItemAt(Page p, int i) {
  switch (p) {
    case Page::MainMenu:    return kMain[i];
    case Page::Alarm:       return kAlarm[i];
    case Page::Display:     return kDisplay[i];
    case Page::Layout:      return kLayout[i];
    case Page::Management:  return kMgmt[i];
    case Page::Network:     return kNet[i];
    case Page::SystemClean: return kClean[i];
    default:                return kRetry[i];
  }
}

void menuActivate(Page p, int i) {
  if (p == Page::Layout) { themeSet((uint8_t)i); return; }
  Page t = menuItemAt(p, i).target;
  if (t == Page::AdminConfig && !adminUnlocked()) return;
  uiPush(t);
}