#include "SmartTankState.h"
#include "Theme.h"
#include "PumpController.h"
#include "Management.h"
#include "Buzzer.h"
#include "WiFiManager.h"
#include "ClockHeader.h"


void drawDashboard() {
  const Theme& t = themeGet();
  const bool landscape = W > H;

  canvas.fillSprite(t.bg);

  // Header contains only the name and connection indicators.
  drawHeader();

  // Main dashboard components.
  drawTankGraphic();
  drawDashboardClock();

  // ---------------------------------------
  // COMMON PUMP STATUS
  // ---------------------------------------
  const bool warning = pumpRetryWarningActive();

  uint16_t statusColor = t.muted;
  const char* status = "PUMP OFF";

  if (manualOverrideActive) {
    status = "MANUAL OVERRIDE";
    statusColor = t.warn;
  } else if (autoPumpState == AUTO_FILLING) {
    status = alarmFillSessionActive() ? "SCHEDULE FILL" : "AUTO FILL";
    statusColor = t.ok;
  } else if (autoPumpState == AUTO_RETRY_WAIT) {
    status = "RETRY WAIT";
    statusColor = t.warn;
  } else if (autoPumpState == AUTO_WAIT_SENSOR) {
    status = "WAITING SENSOR";
    statusColor = t.warn;
  } else if (manualPumpState) {
    status = "PUMP ON";
    statusColor = t.ok;
  } else if (managementAllowsAuto()) {
    status = "AUTO READY";
    statusColor = t.accent;
  }

  // ---------------------------------------
  // LANDSCAPE
  // ---------------------------------------
  if (landscape) {
    const int x = 100;

    canvas.setTextSize(1);
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(x, 101);
    canvas.print("WATER LEVEL");

    canvas.setTextSize(2);
    canvas.setTextColor(
        (sensorReadingValid && waterLevelValid) ? t.fg : t.warn,
        t.bg
    );

    canvas.setCursor(x, 115);

    if (sensorReadingValid && waterLevelValid)
      canvas.printf("%.1f%%", waterLevelPercent);
    else
      canvas.print("--.-%");

    canvas.setTextSize(1);
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(x, 139);

    if (sensorReadingValid && waterLevelValid)
      canvas.printf("%.0f / %.0f L",
                    waterVolumeLitres, tankCapacityLitres);
    else
      canvas.print("Waiting for sensor");

    const int barX = x;
    const int barY = 155;
    const int barW = W - x - 12;
    const int barH = 9;

    canvas.drawRoundRect(barX, barY, barW, barH, 4, t.muted);

    if (sensorReadingValid && waterLevelValid) {
      const int fillW = (int)(
          (barW - 4) *
          constrain(waterLevelPercent, 0.0f, 100.0f) / 100.0f
      );

      if (fillW > 0)
        canvas.fillRoundRect(
            barX + 2, barY + 2, fillW, barH - 4, 3, t.accent
        );
    }

    canvas.setTextSize(1);
    canvas.setTextColor(t.muted, t.bg);
    canvas.setCursor(x, 172);
    canvas.print("SENSOR");

    canvas.setTextColor(
        remoteConnected && sensorReadingValid ? t.ok : t.err,
        t.bg
    );
    canvas.setCursor(x + 52, 172);

    if (!remoteConnected)
      canvas.print("OFFLINE");
    else if (!sensorReadingValid)
      canvas.print("NO DATA");
    else
      canvas.print("ONLINE");

    // Compact pump card
    canvas.fillRoundRect(
        x, 190, W - x - 8, 26, 5,
        warning ? t.err : t.panel
    );

    canvas.setTextSize(1);
    canvas.setTextColor(
        warning ? TFT_WHITE : statusColor,
        warning ? t.err : t.panel
    );

    canvas.setCursor(x + 8, 199);
    canvas.print(warning ? "NO WATER - RETRY" : status);

    canvas.fillRect(0, H - 20, W, 20, t.footerBg);
    canvas.setTextColor(t.fg, t.footerBg);
    canvas.setCursor(8, H - 13);
    canvas.print("PRESS: MENU   HOLD: MENU");

    canvas.pushSprite(0, 0);
    return;
  }

  // ---------------------------------------
  // PORTRAIT WATER LEVEL
  // ---------------------------------------
  const int x = 92;

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(x, 121);
  canvas.print("WATER LEVEL");

  canvas.setTextSize(2);
  canvas.setTextColor(
      (sensorReadingValid && waterLevelValid) ? t.fg : t.warn,
      t.bg
  );

  canvas.setCursor(x, 136);

  if (sensorReadingValid && waterLevelValid)
    canvas.printf("%.1f%%", waterLevelPercent);
  else
    canvas.print("--.-%");

  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(x, 163);

  if (sensorReadingValid && waterLevelValid)
    canvas.printf("%.0f / %.0f L",
                  waterVolumeLitres, tankCapacityLitres);
  else
    canvas.print("Waiting for sensor");

  // Progress bar
  const int barX = x;
  const int barY = 179;
  const int barW = W - x - 10;
  const int barH = 9;

  canvas.drawRoundRect(barX, barY, barW, barH, 4, t.muted);

  if (sensorReadingValid && waterLevelValid) {
    const int fillW = (int)(
        (barW - 4) *
        constrain(waterLevelPercent, 0.0f, 100.0f) / 100.0f
    );

    if (fillW > 0)
      canvas.fillRoundRect(
          barX + 2, barY + 2, fillW, barH - 4, 3, t.accent
      );
  }

  // Sensor status
  canvas.setTextSize(1);
  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(x, 199);
  canvas.print("SENSOR LINK");

  canvas.setTextColor(
      remoteConnected && sensorReadingValid ? t.ok : t.err,
      t.bg
  );

  canvas.setCursor(x, 212);

  if (!remoteConnected)
    canvas.print("OFFLINE");
  else if (!sensorReadingValid)
    canvas.print("NO DATA");
  else
    canvas.print("ONLINE");

  canvas.setTextColor(t.muted, t.bg);
  canvas.setCursor(x, 224);

  if (remoteConnected)
    canvas.printf("LoRa %d dBm", lastRSSI);
  else
    canvas.print("LoRa waiting");

  // ---------------------------------------
  // PORTRAIT PUMP CARD
  // ---------------------------------------
  const int cardY = 237;

  canvas.fillRoundRect(
      8, cardY, W - 16, 55, 7,
      warning ? t.err : t.panel
  );

  canvas.setTextSize(1);
  canvas.setTextColor(
      warning ? TFT_WHITE : t.muted,
      warning ? t.err : t.panel
  );

  canvas.setCursor(17, cardY + 7);
  canvas.print(warning ? "PUMP WARNING" : "PUMP STATUS");

  canvas.setTextSize(2);
  canvas.setTextColor(
      warning ? TFT_WHITE : statusColor,
      warning ? t.err : t.panel
  );

  canvas.setCursor(17, cardY + 22);
  canvas.print(warning ? "NO WATER - RETRY" : status);

  if (warning) {
    const uint32_t left = pumpRetryRemainingSeconds();

    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE, t.err);
    canvas.setCursor(17, cardY + 43);

    canvas.printf(
        "Retry %02lu:%02lu",
        (unsigned long)(left / 60),
        (unsigned long)(left % 60)
    );
  }

  // Footer
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(8, H - 13);
  canvas.print("PRESS: MENU   HOLD: MENU");

  canvas.pushSprite(0, 0);
}

