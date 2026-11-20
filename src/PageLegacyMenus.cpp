#include "SmartTankState.h"
#include "Theme.h"
#include "TankSettings.h"
#include "PumpController.h"
#include "WiFiManager.h"

void drawLinkPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("LINK STATUS");
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(8, 80); canvas.print("REMOTE:");
  canvas.setCursor(125, 80);
  canvas.setTextColor(remoteConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
  canvas.print(remoteConnected ? "CONNECTED" : "DISCONNECTED");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(8, 110); canvas.print("LoRa RSSI:");
  canvas.setCursor(125, 110);
  if (remoteConnected) canvas.printf("%d dBm", lastRSSI); else canvas.print("--");
  canvas.setCursor(8, 135); canvas.print("LoRa SNR:");
  canvas.setCursor(125, 135);
  if (remoteConnected) canvas.printf("%.1f dB", lastSNR); else canvas.print("--");
  canvas.setCursor(8, 160); canvas.print("Wi-Fi:");
  canvas.setCursor(125, 160);
  if (wifiManagerIsConnected()) {
    canvas.setTextColor(TFT_GREEN, TFT_BLACK); canvas.print("CONNECTED");
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setCursor(8, 185); canvas.printf("Wi-Fi RSSI: %d dBm", wifiManagerGetRSSI());
  } else canvas.print("NOT CONNECTED");
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawDiagnosticsPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("DIAGNOSTICS");
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(8, 82);  canvas.printf("Packets received: %u", (unsigned)packetsReceived);
  canvas.setCursor(8, 108); canvas.printf("Sensor errors: %u",   (unsigned)sensorErrors);
  canvas.setCursor(8, 134); canvas.printf("Invalid packets: %u", (unsigned)invalidPackets);
  canvas.setCursor(8, 160); canvas.printf("Distance: %.1f cm", distanceCm);
  canvas.setCursor(8, 186); canvas.printf("Auto-pend: %s", autoPendingOn ? "YES" : "NO");
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawWaterLevelPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("WATER LEVEL");
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 78); canvas.print("CURRENT LEVEL");
  canvas.setTextSize(4);
  canvas.setTextColor((sensorReadingValid && waterLevelValid) ? TFT_GREEN : TFT_YELLOW, TFT_BLACK);
  canvas.setCursor(10, 96);
  if (sensorReadingValid && waterLevelValid) canvas.printf("%.1f%%", waterLevelPercent);
  else canvas.print("--.-%");
  canvas.setTextSize(2); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 145);
  if (sensorReadingValid && waterLevelValid) canvas.printf("%.0f Litres", waterVolumeLitres);
  else canvas.print("-- Litres");
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 178); canvas.printf("Water height: %.1f cm", waterHeightCm);
  canvas.setCursor(10, 196); canvas.printf("Tank height: %.1f cm", tankHeightCm);
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawTankCalibrationPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("TANK SETUP");
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 82); canvas.print("TANK HEIGHT");
  float shownHeight = calibrationEditing ? calibrationDraftHeightCm : tankHeightCm;
  canvas.setTextSize(4);
  canvas.setTextColor(calibrationEditing ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 103); canvas.printf("%.0f cm", shownHeight);
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 157); canvas.printf("Capacity: %.0f Litres", tankCapacityLitres);
  canvas.setCursor(10, 177); canvas.print("Shape: Cylindrical");
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13);
  if (calibrationEditing) canvas.print("PRESS: SAVE  |  HOLD: CANCEL");
  else                    canvas.print("PRESS: EDIT  |  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawManualPumpPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("MANUAL PUMP");
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 70); canvas.print("TTP223 TOUCH SWITCH");
 canvas.drawRect(10, 92, W - 20, 65, themeGet().muted);
  if (manualOverrideActive) {
    unsigned long elapsedSec = (millis() - manualOverrideStartedAt) / 1000UL;
    unsigned long totalSec   = (unsigned long)manualOverrideMinutes * 60UL;
    unsigned long remainSec  = totalSec > elapsedSec ? totalSec - elapsedSec : 0;
    canvas.setTextSize(2); canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.setCursor(20, 100); canvas.print("OVERRIDE ON");
    canvas.setTextSize(3); canvas.setCursor(20, 122);
    canvas.printf("%02lu:%02lu", remainSec / 60, remainSec % 60);
  } else if (autoPumpState == AUTO_FILLING) {
    canvas.setTextSize(2); canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    canvas.setCursor(20, 108); canvas.print("AUTO FILLING");
    canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas.setCursor(20, 136); canvas.print("Touch disabled while auto runs");
  } else {
    canvas.setTextSize(3);
    canvas.setTextColor(manualPumpState ? TFT_GREEN : TFT_RED, TFT_BLACK);
    canvas.setCursor(25, 110); canvas.print(manualPumpState ? "OUTPUT ON" : "OUTPUT OFF");
  }
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 170); canvas.printf("Override timeout: %lu min", (unsigned long)manualOverrideMinutes);
  canvas.setCursor(10, 188); canvas.print("Touch: start / cancel override");
  canvas.setCursor(10, 204);
  canvas.print(autoModeEnabled ? "Auto mode: ENABLED" : "Auto mode: DISABLED");
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("TOUCH: TOGGLE  |  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawAutoThresholdsPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("AUTO THRESHOLDS");
  float onV  = thresholdEditing ? draftAutoOnLevel  : pumpAutoOnLevel;
  float offV = thresholdEditing ? draftAutoOffLevel : pumpAutoOffLevel;
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 85); canvas.print("PUMP AUTO ON AT");
  canvas.setTextSize(3);
  canvas.setTextColor(thresholdEditing && thresholdEditStep == 0 ? TFT_YELLOW : TFT_GREEN, TFT_BLACK);
  canvas.setCursor(10, 103); canvas.printf("%.0f%%", onV);
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 153); canvas.print("PUMP AUTO OFF AT");
  canvas.setTextSize(3);
  canvas.setTextColor(thresholdEditing && thresholdEditStep == 1 ? TFT_YELLOW : TFT_GREEN, TFT_BLACK);
  canvas.setCursor(10, 171); canvas.printf("%.0f%%", offV);
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13);
  canvas.print(thresholdEditing ? "PRESS: NEXT/SAVE  HOLD: CANCEL"
                                : "PRESS: EDIT       HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawAutoControlPage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("AUTO CONTROL");
  canvas.setTextSize(1); canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(10, 85); canvas.print("AUTOMATIC MODE");
  canvas.setTextSize(3);
  canvas.setTextColor(autoModeEnabled ? TFT_GREEN : TFT_RED, TFT_BLACK);
  canvas.setCursor(10, 105); canvas.print(autoModeEnabled ? "ENABLED" : "DISABLED");
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 157); canvas.printf("AUTO ON: %.0f%%", pumpAutoOnLevel);
  canvas.setCursor(10, 177); canvas.printf("AUTO OFF: %.0f%%", pumpAutoOffLevel);
  canvas.setCursor(10, 202);
  if (manualOverrideActive) canvas.print("Status: MANUAL OVERRIDE");
  else switch (autoPumpState) {
    case AUTO_IDLE:        canvas.print("Status: STANDBY"); break;
    case AUTO_FILLING:     canvas.print("Status: FILLING"); break;
    case AUTO_RETRY_WAIT:  canvas.print("Status: RETRY WAIT"); break;
    case AUTO_WAIT_SENSOR: canvas.print("Status: SENSOR WAIT"); break;
  }
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("PRESS: TOGGLE  |  HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawOverrideTimePage() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 42); canvas.print("OVERRIDE TIME");
  uint32_t shown = overrideEditing ? draftOverrideMinutes : manualOverrideMinutes;
  canvas.setTextSize(4);
  canvas.setTextColor(overrideEditing ? TFT_YELLOW : TFT_GREEN, TFT_BLACK);
  canvas.setCursor(10, 110); canvas.printf("%lu min", (unsigned long)shown);
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(10, 170);
  canvas.printf("Range: %lu - %lu min",
                (unsigned long)MIN_OVERRIDE_MINUTES,
                (unsigned long)MAX_OVERRIDE_MINUTES);
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13);
  canvas.print(overrideEditing ? "PRESS: SAVE   HOLD: CANCEL"
                               : "PRESS: EDIT   HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void drawMenu() {
  canvas.fillSprite(TFT_BLACK); drawHeader();
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2); canvas.setCursor(8, 39); canvas.print("MAIN MENU");
  static const char* legacy[] = {
    "Dashboard","Link Status","Diagnostics","Water Level","Tank Calibration",
    "Manual Pump","Auto Thresholds","Auto Control","Override Time"
  };
  const int n = 9, VIS = 6;
  int first = menuIndex >= VIS ? menuIndex - VIS + 1 : 0;
  if (first + VIS > n) first = n > VIS ? n - VIS : 0;
  for (int s = 0; s < VIS && first + s < n; s++) {
    int y = 67 + s * 25;
    int idx = first + s;
  const Theme& t = themeGet();
if (idx == menuIndex) {
  canvas.fillRect(8, y, W - 16, 22, t.selBg);
  canvas.fillRect(8, y, 3, 22, t.accent);
  canvas.setTextColor(t.selFg, t.selBg);
  canvas.setTextSize(1);
  canvas.setCursor(18, y + 7);
  canvas.print(legacy[idx]);
  continue;  // skip the default draw below
} else canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas.setTextSize(1); canvas.setCursor(18, y + 7);
    canvas.print(legacy[idx]);
  }
  canvas.fillRect(0, H - 19, W, 19, TFT_DARKGREY);
  canvas.setTextSize(1); canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
  canvas.setCursor(8, H - 13); canvas.print("ROTATE: MOVE  |  PRESS: SELECT");
  canvas.pushSprite(0, 0);
}