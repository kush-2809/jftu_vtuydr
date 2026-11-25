#include "UiPages.h"
#include "UiState.h"
#include "SmartTankState.h"
#include "Theme.h"
#include "Management.h"
#include "FeatureControl.h"
#include "PumpController.h"
#include "Buzzer.h"
#include "WiFiManager.h"
#include "TimeService.h"
#include "TankSettings.h"
#include "Scheduler.h"
#include "DisplayOrientation.h"
#include <Preferences.h>
#include <math.h>
#include <time.h>

static void simpleHeader(const char* title) {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 34, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 9);
  canvas.print(title);
  canvas.fillRect(0, 34, W, 2, t.accent);
}

static void simpleFooter(const char* hint) {
  const Theme& t = themeGet();
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(7, H - 13);
  canvas.print(hint);
}

static int clampValue(int value, int lo, int hi) {
  return value < lo ? lo : (value > hi ? hi : value);
}

static void labelValue(int y, const char* label, const String& value,
                       uint16_t valueColor = 0) {
  const int drawY=y-uiScrollOffset();
  if(drawY<38||drawY>H-28)return;
  const Theme& t=themeGet();
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,drawY);canvas.print(label);
  canvas.setTextColor(valueColor?valueColor:t.fg,t.bg);
  canvas.setCursor(128,drawY);canvas.print(value);
}

// ---------------------------------------------------------------------------
// MANAGEMENT: CONTROL MODE
// ---------------------------------------------------------------------------
static ControlMode modeDraft = CONTROL_ALARM_ONLY;
static bool modeLoaded = false;

void uiManagementModeTick() {
  if (!modeLoaded) {
    modeDraft = managementMode();
    modeLoaded = true;
  }
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) {
    modeLoaded = false;
    uiPop();
    return;
  }
  if (dir) {
    int value = static_cast<int>(modeDraft) + (dir > 0 ? 1 : -1);
    if (value < CONTROL_ALARM_ONLY) value = CONTROL_ALL;
    if (value > CONTROL_ALL) value = CONTROL_ALARM_ONLY;
    modeDraft = static_cast<ControlMode>(value);
  }
  if (uiConsumePress()) {
    managementSetMode(modeDraft);
    buzzerBeep(60, buzzerVolume());
    modeLoaded = false;
    uiPop();
  }
}

void uiManagementModeDraw() {
  const Theme& t = themeGet();
  simpleHeader("OPERATING MODE");
  const char* names[] = {"Alarm only", "Automatic only", "Auto + manual", "All functions"};
  const char* desc[] = {
    "Timer and daily schedule",
    "Level-based automatic fill",
    "Automatic + touch override",
    "Alarm, automatic and manual"
  };
  const ControlMode shown = modeLoaded ? modeDraft : managementMode();
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 49);
  canvas.print("SELECT HOW THE PUMP IS CONTROLLED");
  const int modeTop = W > H ? 48 : 72;
  const int modeStep = W > H ? 36 : 39;
  for (int i = 0; i < 4; ++i) {
    const int y = modeTop + i * modeStep;
    const bool selected = i == static_cast<int>(shown);
    if (selected) {
      canvas.fillRoundRect(8, y, W - 16, 33, 6, t.selBg);
      canvas.fillRect(8, y, 3, 33, t.accent);
    }
    canvas.setTextSize(2);
    canvas.setTextColor(selected ? t.selFg : t.fg, selected ? t.selBg : t.bg);
    canvas.setCursor(18, y + 4);
    canvas.print(names[i]);
    canvas.setTextSize(1);
    canvas.setTextColor(selected ? t.selFg : t.muted, selected ? t.selBg : t.bg);
    canvas.setCursor(18, y + 22);
    canvas.print(desc[i]);
  }
  simpleFooter("ROTATE: SELECT  PRESS: SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// MANAGEMENT: BUZZER
// ---------------------------------------------------------------------------
static uint8_t buzzerDraft = 3;
static bool buzzerLoaded = false;

void uiManagementBuzzerTick() {
  if (!buzzerLoaded) {
    buzzerDraft = buzzerVolume();
    buzzerLoaded = true;
  }
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) {
    buzzerLoaded = false;
    uiPop();
    return;
  }
  if (dir) buzzerDraft = static_cast<uint8_t>(clampValue((int)buzzerDraft + dir, 0, 3));
  if (uiConsumePress()) {
    buzzerSetVolume(buzzerDraft);
    if (buzzerDraft) buzzerBeep(100, buzzerDraft);
    buzzerLoaded = false;
    uiPop();
  }
}

// ---------------------------------------------------------------------------
// MANAGEMENT: SCREEN ORIENTATION
// ---------------------------------------------------------------------------
static uint8_t rotationDraft=0;
static bool rotationLoaded=false;
void uiManagementRotationTick(){
  if(!rotationLoaded){rotationDraft=displayRotation();rotationLoaded=true;}
  const int dir=uiConsumeRotation();
  if(uiConsumeLongPress()){rotationLoaded=false;uiPop();return;}
  if(dir)rotationDraft=(uint8_t)((rotationDraft+(dir>0?1:3))&0x03);
  if(uiConsumePress()){
    if(displaySetRotation(rotationDraft)){
      buzzerBeep(70,buzzerVolume());rotationLoaded=false;uiPop();
    }else{rotationDraft=displayRotation();buzzerBeep(180,3);}
  }
}
void uiManagementRotationDraw(){
  const Theme& t=themeGet();simpleHeader("SCREEN ROTATION");
  const uint8_t shown=rotationLoaded?rotationDraft:displayRotation();
  const char* names[]={"Portrait 0 deg","Landscape 90 deg","Portrait 180 deg","Landscape 270 deg"};
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,50);canvas.print("CHOOSE DISPLAY ORIENTATION");
  const bool landscape=W>H;
  const int rotationTop=landscape?48:70;
  const int rotationStep=landscape?34:37;
  const int rotationRowH=landscape?28:31;
  for(int i=0;i<4;++i){
    const int y=rotationTop+i*rotationStep;const bool selected=i==shown;
    if(selected){canvas.fillRoundRect(8,y,W-16,rotationRowH,5,t.selBg);canvas.fillRect(8,y,3,rotationRowH,t.accent);}
    canvas.setTextSize(2);canvas.setTextColor(selected?t.selFg:t.fg,selected?t.selBg:t.bg);
    canvas.setCursor(18,y+8);canvas.print(names[i]);
    if(i==displayRotation()){
      canvas.setTextSize(1);canvas.setTextColor(selected?t.selFg:t.ok,selected?t.selBg:t.bg);
      canvas.setCursor(W-52,y+11);canvas.print("ACTIVE");
    }
  }
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,landscape?H-38:222);canvas.print("Landscape gives the dashboard more width.");
  simpleFooter("ROTATE: CHOOSE  PRESS: APPLY  HOLD: CANCEL");
  canvas.pushSprite(0,0);
}

void uiManagementBuzzerDraw() {
  const Theme& t = themeGet();
  simpleHeader("BUZZER VOLUME");
  const uint8_t shown = buzzerLoaded ? buzzerDraft : buzzerVolume();
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 55);
  canvas.print("ALERT SOUND LEVEL");
  canvas.setTextSize(5);
  canvas.setTextColor(shown ? t.accent : t.muted, t.bg);
  canvas.setCursor(18, 83);
  canvas.printf("%u", shown);
  canvas.setTextSize(2);
  canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(76, 106);
  canvas.print(shown == 0 ? "MUTED" : shown == 1 ? "LOW" : shown == 2 ? "MEDIUM" : "HIGH");
  for (int i = 0; i < 3; ++i) {
    int x = 18 + i * 38;
    canvas.fillRoundRect(x, 160 - i * 15, 25, 15 + i * 15, 3,
                         shown > i ? t.accent : t.muted);
  }
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 213);
  canvas.print("0 = silent     3 = maximum output");
  simpleFooter("ROTATE: ADJUST  PRESS: SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// NETWORK: LIVE LINK DIAGNOSTICS
// ---------------------------------------------------------------------------
void uiNetworkStatusTick() {
  const int dir=uiConsumeRotation();uiConsumePress();
  if(uiConsumeLongPress()){uiPop();return;}
  if(dir)uiScrollBy(dir,227-(H-26));
}

void uiNetworkStatusDraw() {
  const Theme& t = themeGet();
  simpleHeader("NETWORK STATUS");
  labelValue(48, "LoRa link", remoteConnected ? "CONNECTED" : "OFFLINE",
             remoteConnected ? t.ok : t.err);
  labelValue(67, "LoRa RSSI", remoteConnected ? String(lastRSSI) + " dBm" : "--", t.accent);
  labelValue(86, "LoRa SNR", remoteConnected ? String(lastSNR, 1) + " dB" : "--", t.accent);
  labelValue(105, "Ultrasonic", sensorReadingValid ? String(distanceCm, 1) + " cm" : "NO DATA",
             sensorReadingValid ? t.ok : t.err);
  labelValue(124, "Water level", waterLevelValid ? String(waterLevelPercent, 1) + "%" : "INVALID",
             waterLevelValid ? t.ok : t.err);
  labelValue(143, "Wi-Fi", wifiManagerIsConnected() ? "CONNECTED" : "NOT CONNECTED",
             wifiManagerIsConnected() ? t.ok : t.warn);
  labelValue(162, "Wi-Fi RSSI", wifiManagerIsConnected() ? String(wifiManagerGetRSSI()) + " dBm" : "--",
             t.accent);
  labelValue(181, "IP address", wifiManagerGetIP());
  labelValue(200, "SSID", wifiManagerGetSSID());
  labelValue(219, "Packets", String((unsigned long)packetsReceived));
  uiDrawScrollIndicator(227-(H-26));
  simpleFooter("ROTATE: SCROLL  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// NETWORK: WI-FI PROVISIONING
// ---------------------------------------------------------------------------
void uiNetworkWiFiTick() {
  uiConsumeRotation();
  if (uiConsumeLongPress()) { uiPop(); return; }
  if (uiConsumePress()) {
    wifiManagerStartProvisioning();
    buzzerBeep(80, buzzerVolume());
  }
}

void uiNetworkWiFiDraw() {
  const Theme& t=themeGet();
  const bool landscape=W>H;
  simpleHeader("CHANGE WI-FI");
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,landscape?42:52);canvas.print("CURRENT CONNECTION");
  canvas.setTextSize(landscape?1:2);canvas.setTextColor(t.fg,t.bg);
  canvas.setCursor(10,landscape?58:72);
  canvas.print(wifiManagerIsConnected()?wifiManagerGetSSID():"Offline");
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,landscape?77:105);
  canvas.print(wifiManagerIsConnected()?wifiManagerGetIP():"Setup network is available");
  canvas.drawFastHLine(10,landscape?95:132,W-20,t.muted);
  canvas.setTextColor(t.accent,t.bg);canvas.setCursor(10,landscape?109:148);
  canvas.print("SETUP ACCESS POINT");
  canvas.setTextSize(landscape?1:2);canvas.setTextColor(t.fg,t.bg);
  canvas.setCursor(10,landscape?126:168);canvas.print("SmartTank");
  canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
  canvas.setCursor(10,landscape?146:194);canvas.print("Password: Tank@1234");
 canvas.setCursor(10,landscape?163:228);
canvas.print("Open http://");
canvas.print(wifiManagerGetActiveAPIP());
  canvas.setCursor(10,landscape?180:228);
  canvas.print(wifiManagerIsAPActive()?"Setup AP: ACTIVE":"Press to enable setup AP");
  simpleFooter("PRESS: ENABLE SETUP AP  HOLD: BACK");
  canvas.pushSprite(0,0);
}

// ---------------------------------------------------------------------------
// SYSTEM CLEAN: EXPLICIT CONFIRMATION
// ---------------------------------------------------------------------------
static int cleanChoice = 0; // 0 = cancel, 1 = confirm

static const char* cleanTitle(Page p) {
  switch (p) {
    case Page::CleanReboot: return "REBOOT DEVICE";
    case Page::CleanClearCache: return "CLEAR DIAGNOSTICS";
    case Page::CleanFactoryReset: return "FACTORY RESET";
    default: return "SYSTEM CLEAN";
  }
}

void uiSystemCleanTick() {
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { cleanChoice = 0; uiPop(); return; }
  if (dir) cleanChoice = cleanChoice ? 0 : 1;
  if (!uiConsumePress()) return;
  if (!cleanChoice) { uiPop(); return; }

  const Page action = uiPage();
  if (action == Page::CleanReboot) {
    buzzerBeep(80, buzzerVolume());
    delay(80);
    ESP.restart();
  } else if (action == Page::CleanClearCache) {
    packetsReceived = 0;
    sensorErrors = 0;
    invalidPackets = 0;
    lastPacketMillis = 0;
    lastRSSI = 0;
    lastSNR = 0;
    buzzerBeep(80, buzzerVolume());
    cleanChoice = 0;
    uiPop();
  } else if (action == Page::CleanFactoryReset) {
    const char* namespaces[] = {
      "tank-config", "alarm-config", "ui", "display-config",
      "wifi-config", "control-config", "feature-config", "retry-config",
      "portal-auth", "weather-loc"
    };
    for (const char* ns : namespaces) {
      Preferences p;
      if (p.begin(ns, false)) {
        p.clear();
        p.end();
      }
    }
    // The serial number is derived from the ESP32 eFuse MAC and is not erased.
    ESP.restart();
  }
}

void uiSystemCleanDraw() {
  const Theme& t = themeGet();
  simpleHeader(cleanTitle(uiPage()));
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 59);
  if (uiPage() == Page::CleanReboot) canvas.print("Restart Smart Tank controller?");
  else if (uiPage() == Page::CleanClearCache) canvas.print("Clear packet and sensor error counters?");
  else canvas.print("Erase saved settings and Wi-Fi credentials?");

  if (uiPage() == Page::CleanFactoryReset) {
    canvas.setCursor(10, 82);
    canvas.print("Device serial number will remain.");
    canvas.setCursor(10, 101);
    canvas.print("Wi-Fi setup will be required again.");
  }

  const char* options[] = {"CANCEL", "CONFIRM"};
  for (int i = 0; i < 2; ++i) {
    int y = 155 + i * 42;
    if (cleanChoice == i) canvas.fillRoundRect(16, y, W - 32, 32, 6, t.selBg);
    canvas.setTextSize(2);
    canvas.setTextColor(cleanChoice == i ? t.selFg : t.fg,
                        cleanChoice == i ? t.selBg : t.bg);
    canvas.setCursor(70, y + 8);
    canvas.print(options[i]);
  }
  simpleFooter("ROTATE: CHOOSE  PRESS: CONFIRM  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// SYSTEM RETRY: MONITOR AND SET A MANUAL RETRY TIME
// ---------------------------------------------------------------------------
static bool retryUiLoaded = false;
static uint8_t retryHourDraft = 18;
static uint8_t retryMinuteDraft = 0;
static uint8_t retryEditStep = 0; // hour, minute, save, clear

void uiSystemRetryTick() {
  if (!retryUiLoaded) {
    retryHourDraft = pumpRetryManualTimeEnabled() ? pumpRetryManualHour() : 18;
    retryMinuteDraft = pumpRetryManualTimeEnabled() ? pumpRetryManualMinute() : 0;
    retryEditStep = 0;
    retryUiLoaded = true;
  }
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) {
    retryUiLoaded = false;
    uiPop();
    return;
  }
  if (dir) {
    if (retryEditStep == 0) retryHourDraft = (uint8_t)((retryHourDraft + 24 + dir) % 24);
    else if (retryEditStep == 1) retryMinuteDraft = (uint8_t)((retryMinuteDraft + 60 + dir) % 60);
    else retryEditStep = (retryEditStep + (dir > 0 ? 1 : 3)) % 4;
  }
  if (!uiConsumePress()) return;
  if (retryEditStep < 2) {
    retryEditStep = (retryEditStep + 1) % 4;
    return;
  }
  if (retryEditStep == 2) {
    if (timeIsValid()) {
      pumpRetrySetManualTime(retryHourDraft, retryMinuteDraft);
      buzzerBeep(80, buzzerVolume());
    }
  } else {
    pumpRetryClearManualTime();
    buzzerBeep(50, buzzerVolume());
  }
  retryEditStep = 0;
}

void uiSystemRetryDraw() {
  const Theme& t = themeGet();
  simpleHeader("SYSTEM RETRY");
  const bool warning = pumpRetryWarningActive();
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 48);
  canvas.print("PUMP RECOVERY STATUS");
  canvas.setTextSize(2);
  canvas.setTextColor(warning ? t.err : (autoPumpState == AUTO_RETRY_WAIT ? t.warn : t.ok), t.bg);
  canvas.setCursor(10, 65);
  if (warning && autoPumpState == AUTO_FILLING) canvas.print("RETRY IN PROGRESS");
  else canvas.print(warning ? "NO WATER DETECTED" : autoPumpState == AUTO_RETRY_WAIT ? "RETRY WAIT" : "NO ACTIVE ALERT");

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 99);
  canvas.printf("Failed attempts: %u", pumpFailedAttemptCount());
  const uint32_t left = pumpRetryRemainingSeconds();
  canvas.setCursor(10, 116);
  if (autoPumpState == AUTO_FILLING && warning) canvas.print("Retry attempt: RUNNING");
  else canvas.printf("Next retry: %02lu:%02lu:%02lu",
                     (unsigned long)(left / 3600),
                     (unsigned long)((left / 60) % 60),
                     (unsigned long)(left % 60));
  canvas.setCursor(10, 133);
  canvas.print(pumpRetryManualTimeEnabled() ? "Manual retry time is active" : "Automatic retry sequence active");

  canvas.drawFastHLine(10, 153, W - 20, t.muted);
  canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 165);
  canvas.print("SET A RETRY TIME");
  const uint8_t hh = retryUiLoaded ? retryHourDraft : (pumpRetryManualTimeEnabled() ? pumpRetryManualHour() : 18);
  const uint8_t mm = retryUiLoaded ? retryMinuteDraft : (pumpRetryManualTimeEnabled() ? pumpRetryManualMinute() : 0);
  const char* fields[] = {"HOUR", "MINUTE", "SAVE TIME", "CLEAR TIME"};
  char value[12];
  snprintf(value, sizeof(value), "%02u:%02u", hh, mm);
  canvas.setTextSize(3);
  canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(10, 186);
  canvas.print(value);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 220);
  canvas.printf("Edit: %s", fields[retryUiLoaded ? retryEditStep : 0]);
  canvas.setCursor(10, 236);
  canvas.print(timeIsValid() ? "Clock: synchronized/valid" : "Clock required: sync NTP first");
  canvas.setCursor(10, 253);
  canvas.setTextColor(warning ? t.err : t.muted, t.bg);
  canvas.print(warning ? "No water rise. Set time or await retry." : "Manual retry time clears the alert.");
  simpleFooter("ROTATE: EDIT  PRESS: NEXT / ACTION  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// ADMIN: TANK HEIGHT
// ---------------------------------------------------------------------------
void uiAdminTankHeightTick() {
  if (!calibrationEditing) startTankHeightEdit();
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { cancelTankHeightEdit(); uiPop(); return; }
  if (dir) {
    calibrationDraftHeightCm += dir;
    calibrationDraftHeightCm = constrain(calibrationDraftHeightCm, MIN_TANK_HEIGHT_CM, MAX_TANK_HEIGHT_CM);
  }
  if (uiConsumePress()) { saveTankHeightEdit(); uiPop(); }
}

void uiAdminTankHeightDraw() {
  const Theme& t = themeGet();
  simpleHeader("TANK HEIGHT");
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 62);
  canvas.print("SENSOR TO TANK BASE DISTANCE");
  canvas.setTextSize(4);
  canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 92);
  canvas.printf("%.0f cm", calibrationEditing ? calibrationDraftHeightCm : tankHeightCm);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 159);
  canvas.printf("Allowed: %.0f - %.0f cm", MIN_TANK_HEIGHT_CM, MAX_TANK_HEIGHT_CM);
  simpleFooter("ROTATE: ADJUST  PRESS: SAVE  HOLD: CANCEL");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// ADMIN: TANK CAPACITY
// ---------------------------------------------------------------------------
static float capacityDraft = 1000.0f;
static bool capacityLoaded = false;

void uiAdminTankCapacityTick() {
  if (!capacityLoaded) { capacityDraft = tankCapacityLitres; capacityLoaded = true; }
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { capacityLoaded = false; uiPop(); return; }
  if (dir) {
    float step = capacityDraft < 1000.0f ? 10.0f : 50.0f;
    capacityDraft += dir * step;
    capacityDraft = constrain(capacityDraft, 1.0f, 100000.0f);
  }
  if (uiConsumePress()) {
    tankCapacityLitres = capacityDraft;
    saveTankSettings();
    if (sensorReadingValid && remoteConnected) calculateWaterLevel(distanceCm);
    capacityLoaded = false;
    buzzerBeep(60, buzzerVolume());
    uiPop();
  }
}

void uiAdminTankCapacityDraw() {
  const Theme& t = themeGet();
  simpleHeader("TANK CAPACITY");
  const float shown = capacityLoaded ? capacityDraft : tankCapacityLitres;
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 62);
  canvas.print("USABLE TANK VOLUME");
  canvas.setTextSize(4);
  canvas.setTextColor(t.accent, t.bg);
  canvas.setCursor(10, 92);
  canvas.printf("%.0f L", shown);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 159);
  canvas.print("Rotate to change in 10 / 50 L steps");
  simpleFooter("ROTATE: ADJUST  PRESS: SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// ADMIN: AUTOMATIC WATER-LEVEL THRESHOLDS
// ---------------------------------------------------------------------------
static uint8_t thresholdStep = 0;
static bool thresholdPageLoaded = false;

void uiAdminThresholdsTick() {
  if (!thresholdPageLoaded) {
    draftAutoOnLevel = pumpAutoOnLevel;
    draftAutoOffLevel = pumpAutoOffLevel;
    thresholdStep = 0;
    thresholdPageLoaded = true;
  }
  const int dir = uiConsumeRotation();
  if (uiConsumeLongPress()) { thresholdPageLoaded = false; uiPop(); return; }
  if (dir) {
    if (thresholdStep == 0) draftAutoOnLevel = constrain(draftAutoOnLevel + dir, 0.0f, 98.0f);
    else if (thresholdStep == 1) draftAutoOffLevel = constrain(draftAutoOffLevel + dir, 1.0f, 100.0f);
    else thresholdStep = (thresholdStep + (dir > 0 ? 1 : 2)) % 3;
  }
  if (!uiConsumePress()) return;
  if (thresholdStep < 2) { ++thresholdStep; return; }
  if (draftAutoOffLevel >= draftAutoOnLevel + 1.0f) {
    pumpAutoOnLevel = draftAutoOnLevel;
    pumpAutoOffLevel = draftAutoOffLevel;
    saveTankSettings();
    thresholdPageLoaded = false;
    buzzerBeep(60, buzzerVolume());
    uiPop();
  }
}

void uiAdminThresholdsDraw() {
  const Theme& t = themeGet();
  simpleHeader("AUTO THRESHOLDS");
  const bool loaded = thresholdPageLoaded;
  const float on = loaded ? draftAutoOnLevel : pumpAutoOnLevel;
  const float off = loaded ? draftAutoOffLevel : pumpAutoOffLevel;
  const uint8_t step = loaded ? thresholdStep : 0;
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 55);
  canvas.print("START FILL WHEN LEVEL IS AT OR BELOW");
  canvas.setTextSize(3);
  canvas.setTextColor(step == 0 && loaded ? t.accent : t.fg, t.bg);
  canvas.setCursor(10, 74);
  canvas.printf("%.0f%%", on);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 128);
  canvas.print("STOP FILL WHEN LEVEL REACHES");
  canvas.setTextSize(3);
  canvas.setTextColor(step == 1 && loaded ? t.accent : t.fg, t.bg);
  canvas.setCursor(10, 147);
  canvas.printf("%.0f%%", off);
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(10, 207);
  canvas.print("OFF must be at least 1% above ON");
  simpleFooter("ROTATE: EDIT  PRESS: NEXT / SAVE  HOLD: BACK");
  canvas.pushSprite(0, 0);
}
