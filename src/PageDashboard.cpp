#include "SmartTankState.h"
#include "Theme.h"
#include "PumpController.h"
#include "Management.h"
#include "Buzzer.h"
#include "WiFiManager.h"

void drawDashboard() {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  drawHeader();
  drawTankGraphic();

  // Level and volume are the primary dashboard measurements.
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(116, 58);
  canvas.print("WATER LEVEL");

  canvas.setTextSize(3);
  canvas.setTextColor((sensorReadingValid && waterLevelValid) ? t.fg : t.warn, t.bg);
  canvas.setCursor(114, 76);
  if (sensorReadingValid && waterLevelValid) canvas.printf("%.1f%%", waterLevelPercent);
  else canvas.print("--.-%");

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(116, 116);
  if (sensorReadingValid && waterLevelValid)
    canvas.printf("%.0f / %.0f L", waterVolumeLitres, tankCapacityLitres);
  else
    canvas.print("Waiting for sensor");

  const int barX = 116, barY = 136, barW = W - 128, barH = 11;
  canvas.drawRoundRect(barX, barY, barW, barH, 4, t.muted);
  if (sensorReadingValid && waterLevelValid) {
    const float pct = constrain(waterLevelPercent, 0.0f, 100.0f);
    const int fillW = (int)((barW - 4) * pct / 100.0f);
    if (fillW > 0) canvas.fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 3, t.accent);
  }

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(116, 162);
  canvas.print("SENSOR LINK");
  canvas.setTextColor(remoteConnected && sensorReadingValid ? t.ok : t.err, t.bg);
  canvas.setCursor(116, 177);
  canvas.print(remoteConnected && sensorReadingValid ? "ONLINE" : "OFFLINE");
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(116, 194);
  if (remoteConnected) canvas.printf("LoRa %d dBm", lastRSSI);
  else canvas.print("LoRa waiting");

  // Pump status card.
  const int cardY = 214;
  canvas.fillRoundRect(8, cardY, W - 16, 72, 7,
                       pumpRetryWarningActive() ? t.err : t.panel);
  canvas.setTextSize(1);
  canvas.setTextColor(pumpRetryWarningActive() ? TFT_WHITE : t.muted, 
                      pumpRetryWarningActive() ? t.err : t.panel);
  canvas.setCursor(17, cardY + 8);
  canvas.print(pumpRetryWarningActive() ? "PUMP WARNING" : "PUMP STATUS");

  uint16_t statusColor = t.muted;
  const char* status = "OFF";
  if (manualOverrideActive) {
    status = "MANUAL OVERRIDE";
    statusColor = t.warn;
  } else if (autoPumpState == AUTO_FILLING) {
    status = alarmFillSessionActive() ? "SCHEDULE / TIMER FILL" : "AUTO FILLING";
    statusColor = t.ok;
  } else if (autoPumpState == AUTO_RETRY_WAIT) {
    status = pumpRetryWarningActive() ? "NO WATER - RETRY WAIT" : "RETRY WAIT";
    statusColor = pumpRetryWarningActive() ? t.err : t.warn;
  } else if (autoPumpState == AUTO_WAIT_SENSOR) {
    status = "WAITING FOR SENSOR";
    statusColor = t.warn;
  } else if (manualPumpState) {
    status = "OUTPUT ON";
    statusColor = t.ok;
  } else if (managementAllowsAuto()) {
    status = "AUTO STANDBY";
    statusColor = t.accent;
  }

  canvas.setTextSize(2);
  canvas.setTextColor(pumpRetryWarningActive() ? TFT_WHITE : statusColor,
                      pumpRetryWarningActive() ? t.err : t.panel);
  canvas.setCursor(17, cardY + 23);
  canvas.print(pumpRetryWarningActive() ? "NO WATER" : status);

  if (pumpRetryWarningActive()) {
    const uint32_t left = pumpRetryRemainingSeconds();
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE, t.err);
    canvas.setCursor(17, cardY + 49);
    if (autoPumpState == AUTO_FILLING) {
      canvas.print("Automatic retry active - monitoring level");
    } else if (autoPumpState == AUTO_RETRY_WAIT) {
      canvas.printf("No rise. Retry in %02lu:%02lu:%02lu",
                    (unsigned long)(left / 3600),
                    (unsigned long)((left / 60) % 60),
                    (unsigned long)(left % 60));
    } else {
      canvas.print("No water rise. Check sensor / pump");
    }
    canvas.fillCircle(W - 22, cardY + 14, 6, TFT_WHITE);
    canvas.setTextColor(t.err, TFT_WHITE);
    canvas.setCursor(W - 24, cardY + 10);
    canvas.print("!");
  }

  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13);
  canvas.print("PRESS: MENU   HOLD: MENU");
  canvas.pushSprite(0, 0);
}
