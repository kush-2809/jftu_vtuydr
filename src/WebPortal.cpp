#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <math.h>
#include <stdlib.h>

#include "WebPortal.h"
#include "WiFiManager.h"
#include "SmartTankState.h"
#include "PumpController.h"
#include "TankSettings.h"
#include "Management.h"
#include "PortalAuth.h"
#include "PortalHtml.h"
#include "DisplayEngine.h"
#include "DisplayOrientation.h"
#include "Theme.h"
#include "Buzzer.h"
#include "Scheduler.h"
#include "TimeService.h"
#include <Preferences.h>

namespace {
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
bool portalConfigured = false;
bool portalStarted = false;
bool dnsStarted = false;

void sendMessage(int code, const String& message) {
  server.send(code, "text/plain; charset=utf-8", message);
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", SMARTTANK_PORTAL_HTML);
}

void handleStatus() {
  if (!portalAuthCheck(server)) return;
  String json = "{";
  json += "\"levelValid\":" + String(waterLevelValid ? "true" : "false");
  json += ",\"level\":" + String(waterLevelPercent, 1);
  json += ",\"volume\":" + String(waterVolumeLitres, 1);
  json += ",\"capacity\":" + String(tankCapacityLitres, 1);
  json += ",\"height\":" + String(tankHeightCm, 1);
  json += ",\"pumpOn\":" + String(manualPumpState ? "true" : "false");
  json += ",\"override\":" + String(manualOverrideActive ? "true" : "false");
  json += ",\"overrideMinutes\":" + String(manualOverrideMinutes);
  json += ",\"autoEnabled\":" + String(managementAllowsAuto() ? "true" : "false");
  json += ",\"autoOn\":" + String(pumpAutoOnLevel, 1);
  json += ",\"autoOff\":" + String(pumpAutoOffLevel, 1);
  json += ",\"sensorConnected\":" + String(remoteConnected ? "true" : "false");
  json += ",\"sensorValid\":" + String(sensorReadingValid ? "true" : "false");
  json += ",\"distance\":" + String(distanceCm, 1);
  json += ",\"wifiConnected\":" + String(wifiManagerIsConnected() ? "true" : "false");
  json += ",\"apActive\":" + String(wifiManagerIsAPActive() ? "true" : "false");
  json += ",\"ip\":\"" + wifiManagerGetIP() + "\"";
  String host = wifiManagerGetHostname();
  if (host.endsWith(".local")) host.remove(host.length() - 6);
  json += ",\"hostname\":\"" + host + "\"";
  json += ",\"retryWarning\":" + String(pumpRetryWarningActive() ? "true" : "false");
  json += ",\"retrySeconds\":" + String(pumpRetryRemainingSeconds());
  json += ",\"retryManual\":" + String(pumpRetryManualTimeEnabled() ? "true" : "false");
  json += ",\"retryHour\":" + String(pumpRetryManualHour());
  json += ",\"retryMinute\":" + String(pumpRetryManualMinute());
  json += ",\"clock\":\"" + timeFormatFull() + "\"";
  const char* state = "IDLE";
  if (autoPumpState == AUTO_FILLING) state = "FILLING";
  else if (autoPumpState == AUTO_RETRY_WAIT) state = "RETRY WAIT";
  else if (autoPumpState == AUTO_WAIT_SENSOR) state = "WAIT SENSOR";
  json += ",\"autoState\":\"" + String(state) + "\"}";
  server.send(200, "application/json", json);
}

void handlePumpStart() {
  if (!portalAuthCheck(server)) return;
  if (!remoteConnected || !sensorReadingValid || !waterLevelValid) {
    sendMessage(409, "Cannot start: valid LoRa sensor data is required.");
    return;
  }
  if (autoPumpState == AUTO_FILLING || manualPumpState) {
    sendMessage(409, "Pump is already running under controller control.");
    return;
  }
  startManualOverride();
  if (manualOverrideActive) sendMessage(200, "Manual override started.");
  else sendMessage(409, "Manual override was not started. Check Management mode, sensor link and pump state.");
}

void handlePumpStop() {
  if (!portalAuthCheck(server)) return;
  if (!manualOverrideActive) {
    sendMessage(409, "No manual override is active. Automatic filling is not stopped by this button.");
    return;
  }
  stopManualOverride("web dashboard stop");
  sendMessage(200, "Manual override stopped.");
}

void handleAutoMode() {
  if (!portalAuthCheck(server)) return;
  if (!server.hasArg("enabled")) { sendMessage(400, "Missing enabled parameter."); return; }
  bool enabled = server.arg("enabled") == "1";
  if (enabled && !managementModeSupportsAuto()) {
    sendMessage(409, "Automatic operation is not included in the selected Management mode.");
    return;
  }
  if (enabled != autoModeEnabled) {
    autoModeEnabled = enabled;
    stopManualOverride("automatic mode changed from web dashboard");
    drivePumpOutput(false);
    setAutoPendingOn(false);
    autoPumpState = AUTO_IDLE;
    failedAttempts = 0;
    fillingConfirmed = false;
    bootResumePending = false;
    saveTankSettings();
  }
  sendMessage(200, enabled ? "Automatic mode enabled." : "Automatic mode disabled; pump output commanded OFF.");
}

void handleSettings() {
  if (!portalAuthCheck(server)) return;
  if (!server.hasArg("on") || !server.hasArg("off") || !server.hasArg("minutes") ||
      !server.hasArg("height") || !server.hasArg("capacity")) {
    sendMessage(400, "All settings are required."); return;
  }
  float onLevel = server.arg("on").toFloat();
  float offLevel = server.arg("off").toFloat();
  long minutes = server.arg("minutes").toInt();
  float height = server.arg("height").toFloat();
  float capacity = server.arg("capacity").toFloat();
  if (!isfinite(onLevel) || !isfinite(offLevel) || onLevel < 0 || onLevel > 99 ||
      offLevel < 1 || offLevel > 100 || offLevel - onLevel < 1.0f) {
    sendMessage(400, "Invalid thresholds: OFF must be at least 1% higher than ON."); return;
  }
  if (minutes < (long)MIN_OVERRIDE_MINUTES || minutes > (long)MAX_OVERRIDE_MINUTES) {
    sendMessage(400, "Override timeout must be between 1 and 120 minutes."); return;
  }
  if (!isfinite(height) || height < MIN_TANK_HEIGHT_CM || height > MAX_TANK_HEIGHT_CM ||
      !isfinite(capacity) || capacity < 1 || capacity > 100000) {
    sendMessage(400, "Tank height or capacity is outside the allowed range."); return;
  }
  pumpAutoOnLevel = onLevel;
  pumpAutoOffLevel = offLevel;
  manualOverrideMinutes = (uint32_t)minutes;
  tankHeightCm = height;
  tankCapacityLitres = capacity;
  saveTankSettings();
  if (sensorReadingValid && remoteConnected) calculateWaterLevel(distanceCm);
  sendMessage(200, "Settings saved to ESP32 non-volatile storage.");
}

void handleNetworkConfigGet() {
  if (!portalAuthCheck(server)) return;
  String json = "{";

  json += "\"dhcp\":";
  json += wifiManagerUsesDHCP() ? "true" : "false";

  json += ",\"ip\":\"" + wifiManagerGetStaticIP() + "\"";
  json += ",\"gateway\":\"" + wifiManagerGetGateway() + "\"";
  json += ",\"subnet\":\"" + wifiManagerGetSubnet() + "\"";
  json += ",\"dns\":\"" + wifiManagerGetDNS() + "\"";
  String networkHost = wifiManagerGetHostname();
  if (networkHost.endsWith(".local")) networkHost.remove(networkHost.length() - 6);
  json += ",\"hostname\":\"" + networkHost + "\"";
  json += ",\"apip\":\"" + wifiManagerGetAPIP() + "\"";
  json += ",\"apsubnet\":\"" + wifiManagerGetAPSubnet() + "\"";

  json += "}";

  server.send(200, "application/json", json);
}

void handleNetworkConfigSave() {
  if (!portalAuthCheck(server)) return;
  const char* required[] = {
    "dhcp", "ip", "gateway", "subnet",
    "dns", "hostname", "apip", "apsubnet"
  };

  for (const char* key : required) {
    if (!server.hasArg(key)) {
      sendMessage(400, String("Missing field: ") + key);
      return;
    }
  }

  const bool useDhcp = server.arg("dhcp") == "1";

  String error;

  const bool saved = wifiManagerSaveNetworkConfig(
      useDhcp,
      server.arg("ip"),
      server.arg("gateway"),
      server.arg("subnet"),
      server.arg("dns"),
      server.arg("hostname"),
      server.arg("apip"),
      server.arg("apsubnet"),
      error
  );

  sendMessage(saved ? 200 : 400, error);
}


String escapeJson(const String& value) {
  String out; out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    unsigned char c = value[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if (c < 32) {
      char temp[7];
      snprintf(temp, sizeof(temp), "\\u%04x", c);
      out += temp;
    } else out += (char)c;
  }
  return out;
}
void handleAdminStatus() {
  const bool ok = portalAuthIsAuthenticated(server);
  String json = "{\"authenticated\":";
  json += ok ? "true" : "false";
  json += ",\"mustChange\":";
  json += (ok && portalAuthMustChange()) ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}
void handleAdminLogin() {
  if (!server.hasArg("password")) {
    sendMessage(400, "Password required"); return;
  }
  String token;
  if (!portalAuthLogin(server.arg("password"), token)) {
    sendMessage(401, "Invalid password or temporary login lockout"); return;
  }
  server.sendHeader("Set-Cookie",
      "stsession=" + token + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=1800");
  String response = "{\"mustChange\":";
  response += portalAuthMustChange() ? "true" : "false";
  response += "}";
  server.send(200, "application/json", response);
}
void handleAdminChange() {
  if (!portalAuthSessionValid(server)) return;
  if (!server.hasArg("old") || !server.hasArg("next")) {
    sendMessage(400, "Current and new passwords required"); return;
  }
  if (!portalAuthChange(server.arg("old"), server.arg("next"))) {
    sendMessage(400, "Incorrect current password, or new password invalid (10-64 characters)"); return;
  }
  sendMessage(200, "Administrator password updated");
}
void handleAdminLogout() {
  if (!portalAuthSessionValid(server)) return;
  portalAuthLogout();
  server.sendHeader("Set-Cookie", "stsession=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  sendMessage(200, "Logged out");
}
void handleWifiScanStart() {
  if (!portalAuthCheck(server)) return;
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    sendMessage(202, "Wi-Fi scan already running"); return;
  }
  WiFi.scanDelete();
  int scan = WiFi.scanNetworks(true, true, false, 120);
  if (scan == WIFI_SCAN_FAILED) {
    sendMessage(503, "Unable to start Wi-Fi scan"); return;
  }
  sendMessage(202, "Scanning");
}
void handleWifiScanResult() {
  if (!portalAuthCheck(server)) return;
  int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    server.send(200, "application/json", "{\"state\":\"scanning\",\"networks\":[]}"); return;
  }
  if (count == WIFI_SCAN_FAILED) {
    server.send(200, "application/json", "{\"state\":\"idle\",\"networks\":[]}"); return;
  }
  String json = "{\"state\":\"done\",\"networks\":[";
  const int limit = count < 25 ? count : 25;
  for (int i = 0; i < limit; ++i) {
    if (i) json += ",";
    json += "{\"ssid\":\"" + escapeJson(WiFi.SSID(i)) + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI(i));
    json += ",\"secured\":";
    json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true";
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
  WiFi.scanDelete();
}
bool parseClock(const String& value, uint8_t& h, uint8_t& m) {
  if (value.length() != 5 || value[2] != ':' ||
      !isDigit(value[0]) || !isDigit(value[1]) ||
      !isDigit(value[3]) || !isDigit(value[4])) return false;
  int hh = (value[0] - '0') * 10 + value[1] - '0';
  int mm = (value[3] - '0') * 10 + value[4] - '0';
  if (hh > 23 || mm > 59) return false;
  h = hh; m = mm; return true;
}
void handleDisplayGet() {
  if (!portalAuthCheck(server)) return;
  const SleepWindow& sw = sleepWindow();
  char start[6], end[6];
  snprintf(start, sizeof(start), "%02u:%02u", sw.startHour, sw.startMinute);
  snprintf(end, sizeof(end), "%02u:%02u", sw.endHour, sw.endMinute);
  String j = "{\"rotation\":" + String(displayRotation());
  j += ",\"theme\":" + String(themeIndex());
  j += ",\"timeout\":" + String(displayGetTimeoutSeconds());
  j += ",\"dim\":" + String(displayGetDimStep());
  j += ",\"brightness\":" + String(displayBrightnessPercent());
  j += ",\"buzzer\":" + String(buzzerVolume());
  j += ",\"sleepEnabled\":" + String(sw.enabled ? "true" : "false");
  j += ",\"sleepStart\":\"" + String(start) + "\"";
  j += ",\"sleepEnd\":\"" + String(end) + "\"}";
  server.send(200, "application/json", j);
}
void handleDisplaySave() {
  if (!portalAuthCheck(server)) return;
  const char* required[] = {
      "rotation","theme","timeout","dim","brightness","buzzer",
      "sleepStart","sleepEnd","sleepEnabled"};
  for (const char* key : required)
    if (!server.hasArg(key)) { sendMessage(400, String("Missing ") + key); return; }
  int rotation = server.arg("rotation").toInt();
  int theme = server.arg("theme").toInt();
  int timeout = server.arg("timeout").toInt();
  int dim = server.arg("dim").toInt();
  int brightness = server.arg("brightness").toInt();
  int buzz = server.arg("buzzer").toInt();
  uint8_t sh, sm, eh, em;
  if (rotation < 0 || rotation > 3 || theme < 0 || theme >= themeCount()
      || timeout < 0 || timeout > 600 || (dim != 10 && dim != 15 && dim != 20)
      || brightness < 40 || brightness > 100 || brightness % 10 != 0
      || buzz < 0 || buzz > 3
      || !parseClock(server.arg("sleepStart"), sh, sm)
      || !parseClock(server.arg("sleepEnd"), eh, em)) {
    sendMessage(400, "Invalid display settings"); return;
  }
  if (!displaySetRotation(rotation)) {
    sendMessage(500, "Unable to resize display sprite; no settings changed"); return;
  }
  themeSet(theme);
  displaySetTimeoutSeconds(timeout);
  displaySetDimStep(dim);
  // Do not force the backlight ON during a configured sleep window.
  if (!inSleepWindow()) {
    int delta = ((int)brightness - (int)displayBrightnessPercent()) / 10;
    for (int i = 0; i < abs(delta); ++i)
      displayAdjustBrightness(delta > 0 ? 1 : -1);
  }
  buzzerSetVolume(buzz);
  SleepWindow& sw = sleepWindow();
  sw.startHour = sh; sw.startMinute = sm;
  sw.endHour = eh; sw.endMinute = em;
  sw.enabled = server.arg("sleepEnabled") == "1";
  prefsSaveSleep();
  sendMessage(200, "Display settings saved");
}
void handleLocationGet() {
  if (!portalAuthCheck(server)) return;
  Preferences p;
  if (!p.begin("weather-loc", true)) { sendMessage(500, "Preferences unavailable"); return; }
  String city = p.getString("city", "");
  String country = p.getString("country", "");
  float lat = p.getFloat("lat", 0);
  float lon = p.getFloat("lon", 0);
  String tz = p.getString("tz", "");
  p.end();
  String j = "{\"city\":\"" + escapeJson(city) + "\"";
  j += ",\"country\":\"" + escapeJson(country) + "\"";
  j += ",\"lat\":" + String(lat, 5);
  j += ",\"lon\":" + String(lon, 5);
  j += ",\"tz\":\"" + escapeJson(tz) + "\"";
  j += ",\"offset\":" + String(timeGetTimezoneMinutes());
  j += ",\"ntp\":\"" + escapeJson(timeGetNtpServer()) + "\"}";
  server.send(200, "application/json", j);
}
void handleLocationSave() {
  if (!portalAuthCheck(server)) return;
  const char* required[] = {"city","country","lat","lon","tz","offset","ntp"};
  for (const char* key : required)
    if (!server.hasArg(key)) { sendMessage(400, String("Missing ") + key); return; }
  String city = server.arg("city"), country = server.arg("country");
  String tz = server.arg("tz"), ntp = server.arg("ntp");
  float lat = server.arg("lat").toFloat(), lon = server.arg("lon").toFloat();
  int offset = server.arg("offset").toInt();
  if (city.length() == 0 || city.length() > 64 ||
      country.length() == 0 || country.length() > 64 ||
      tz.length() == 0 || tz.length() > 64 ||
      ntp.length() == 0 || ntp.length() > 100 ||
      !isfinite(lat) || !isfinite(lon) || lat < -90 || lat > 90 ||
      lon < -180 || lon > 180 || offset < -720 || offset > 840) {
    sendMessage(400, "Invalid city or time settings"); return;
  }
  Preferences p;
  if (!p.begin("weather-loc", false)) { sendMessage(500, "Preferences unavailable"); return; }
  p.putString("city", city);
  p.putString("country", country);
  p.putFloat("lat", lat);
  p.putFloat("lon", lon);
  p.putString("tz", tz);
  p.end();
  timeSetTimezoneMinutes(offset);
  timeSetNtpServer(ntp);
  sendMessage(200, "Location and NTP settings saved");
}

void handleSaveWiFi() {
  if (!portalAuthCheck(server)) return;
  if (!server.hasArg("ssid")) { sendMessage(400, "Wi-Fi SSID is required."); return; }
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  ssid.trim();
  if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 63) {
    sendMessage(400, "Invalid Wi-Fi credentials."); return;
  }
  sendMessage(200, "Credentials saved. SmartTank is connecting; this page may move to your router network.");
  wifiManagerSaveCredentials(ssid, password);
}
}

void webPortalConfigure() {
  if (portalConfigured) return;
  portalAuthBegin();
  const char* headers[] = {"Cookie"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/admin/status", HTTP_GET, handleAdminStatus);
  server.on("/api/admin/login", HTTP_POST, handleAdminLogin);
  server.on("/api/admin/change", HTTP_POST, handleAdminChange);
  server.on("/api/admin/logout", HTTP_POST, handleAdminLogout);
  server.on("/api/wifi/scan", HTTP_POST, handleWifiScanStart);
  server.on("/api/wifi/scan", HTTP_GET, handleWifiScanResult);
  server.on("/api/display", HTTP_GET, handleDisplayGet);
  server.on("/api/display", HTTP_POST, handleDisplaySave);
  server.on("/api/location", HTTP_GET, handleLocationGet);
  server.on("/api/location", HTTP_POST, handleLocationSave);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/pump/start", HTTP_POST, handlePumpStart);
  server.on("/api/pump/stop", HTTP_POST, handlePumpStop);
  server.on("/api/auto", HTTP_POST, handleAutoMode);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on(
    "/api/network-config",
    HTTP_GET,
    handleNetworkConfigGet
);

server.on(
    "/api/network-config",
    HTTP_POST,
    handleNetworkConfigSave
);
  server.on("/save", HTTP_POST, handleSaveWiFi);
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/gen_204", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
  server.on("/connecttest.txt", HTTP_GET, handleRoot);
  server.on("/ncsi.txt", HTTP_GET, handleRoot);
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
  portalConfigured = true;
}

void webPortalStart(const IPAddress& apAddress) {
  if (!portalStarted) {
    server.begin();
    portalStarted = true;
  }
  if (!dnsStarted) {
    dnsServer.start(DNS_PORT, "*", apAddress);
    dnsStarted = true;
  }
  Serial.println("SmartTank web dashboard ready");
}

void webPortalStop() {
  // Only stop captive DNS. Do NOT stop HTTP: the dashboard remains
  // reachable through the home-router IP after the setup AP is closed.
  if (dnsStarted) {
    dnsServer.stop();
    dnsStarted = false;
  }
}

void webPortalHandleClient() {
  if (!portalStarted) return;
  if (dnsStarted) dnsServer.processNextRequest();
  server.handleClient();
}
