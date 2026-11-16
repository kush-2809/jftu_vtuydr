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

namespace {
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
bool portalConfigured = false;
bool portalStarted = false;
bool dnsStarted = false;

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#0f172a"><title>SmartTank</title>
<style>
:root{color-scheme:dark;font-family:Arial,sans-serif}*{box-sizing:border-box}body{margin:0;background:#0b1220;color:#e5e7eb;padding:18px}
main{max-width:760px;margin:auto}.top{display:flex;justify-content:space-between;gap:12px;align-items:center;flex-wrap:wrap}
h1{font-size:25px;margin:8px 0}.muted{color:#94a3b8;font-size:13px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px;margin:16px 0}
.card{background:#172235;border:1px solid #29384f;border-radius:14px;padding:16px}.value{font-size:27px;font-weight:700;margin-top:8px}.wide{grid-column:1/-1}
button{border:0;border-radius:9px;padding:12px 15px;background:#0d9488;color:white;font-weight:700;cursor:pointer;margin:4px 4px 4px 0}button.danger{background:#b45309}button.secondary{background:#334155}
input{width:100%;padding:11px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:white;margin:6px 0 12px}label{font-size:13px;color:#cbd5e1}
.status{padding:8px 10px;border-radius:8px;background:#26364d;display:inline-block;font-size:13px}.row{display:grid;grid-template-columns:1fr 1fr;gap:12px}@media(max-width:480px){.row{grid-template-columns:1fr}}
</style></head><body><main>
<div class="top"><div><h1>SmartTank Control</h1><div class="muted">Local controller dashboard</div></div><span class="status" id="net">Loading network…</span></div>
<div class="grid">
<section class="card"><div class="muted">WATER LEVEL</div><div class="value" id="level">--%</div><div class="muted" id="volume">-- L</div></section>
<section class="card"><div class="muted">PUMP OUTPUT</div><div class="value" id="pump">--</div><div class="muted" id="pumpmode">--</div></section>
<section class="card"><div class="muted">SENSOR / LORA</div><div class="value" id="link">--</div><div class="muted" id="distance">Distance: --</div></section>
<section class="card"><div class="muted">AUTOMATIC MODE</div><div class="value" id="auto">--</div><div class="muted" id="thresholds">--</div></section>
<section class="card wide"><h3>Manual pump override</h3><p class="muted">Uses the controller's existing override timeout and sensor/automatic-mode safeguards.</p>
<button onclick="post('/api/pump/start')">Start manual override</button><button class="danger" onclick="post('/api/pump/stop')">Stop manual override</button><div class="muted" id="pumpmsg"></div></section>
<section class="card wide"><h3>Automatic control</h3><button onclick="setAuto(true)">Enable automatic</button><button class="secondary" onclick="setAuto(false)">Disable automatic</button></section>
<section class="card wide"><h3>Tank & pump settings</h3><form id="settings" onsubmit="saveSettings(event)">
<div class="row"><div><label>Auto ON threshold (%)</label><input id="on" type="number" min="0" max="99" step="0.5" required></div>
<div><label>Auto OFF threshold (%)</label><input id="off" type="number" min="1" max="100" step="0.5" required></div></div>
<div class="row"><div><label>Manual override timeout (minutes)</label><input id="mins" type="number" min="1" max="120" required></div>
<div><label>Tank height (cm)</label><input id="height" type="number" min="20" max="500" step="0.5" required></div></div>
<div><label>Tank capacity (litres)</label><input id="capacity" type="number" min="1" max="100000" step="1" required></div>
<button type="submit">Save settings</button><span class="muted" id="settingsmsg"></span></form></section>
<section class="card wide"><h3>Wi-Fi</h3><div class="muted" id="wifiinfo">--</div><p class="muted">If connected to your router, open this page using the IP shown above or http://smarttank.local when mDNS is supported on your device.</p><form action="/save" method="POST"><label>Home Wi-Fi network name (SSID)</label><input name="ssid" maxlength="32" placeholder="Enter Wi-Fi SSID" required><label>Wi-Fi password</label><input name="password" type="password" maxlength="63" placeholder="Enter Wi-Fi password"><button type="submit">Save Wi-Fi & connect</button></form></section>
</div><div class="muted">Refreshes automatically every 2 seconds. Keep pump power isolated during software testing.</div>
</main><script>
async function post(url,body={}){try{let r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(body)});let t=await r.text();document.getElementById('pumpmsg').textContent=t;if(!r.ok)throw Error(t);refresh()}catch(e){document.getElementById('pumpmsg').textContent='Error: '+e.message}}
function setAuto(enabled){post('/api/auto',{enabled:enabled?'1':'0'})}
async function saveSettings(e){e.preventDefault();let b={on:document.getElementById('on').value,off:document.getElementById('off').value,minutes:document.getElementById('mins').value,height:document.getElementById('height').value,capacity:document.getElementById('capacity').value};try{let r=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(b)});let t=await r.text();document.getElementById('settingsmsg').textContent=t;if(!r.ok)throw Error(t);refresh()}catch(e){document.getElementById('settingsmsg').textContent='Error: '+e.message}}
async function refresh(){try{let r=await fetch('/api/status',{cache:'no-store'});let d=await r.json();document.getElementById('level').textContent=d.levelValid?d.level.toFixed(1)+'%':'--';document.getElementById('volume').textContent=d.levelValid?d.volume.toFixed(0)+' / '+d.capacity.toFixed(0)+' L':'Level unavailable';document.getElementById('pump').textContent=d.pumpOn?'ON':'OFF';document.getElementById('pumpmode').textContent=d.override?'Manual override':(d.autoState||'Idle');document.getElementById('link').textContent=d.sensorConnected?'CONNECTED':'OFFLINE';document.getElementById('distance').textContent='Distance: '+(d.sensorValid?d.distance.toFixed(1)+' cm':'--');document.getElementById('auto').textContent=d.autoEnabled?'ENABLED':'DISABLED';document.getElementById('thresholds').textContent='ON '+d.autoOn+'% · OFF '+d.autoOff+'%';document.getElementById('net').textContent=d.wifiConnected?'Home Wi-Fi connected':(d.apActive?'SmartTank setup AP':'Wi-Fi offline');document.getElementById('wifiinfo').textContent='IP: '+d.ip+' · Host: '+d.hostname;['on','off','mins','height','capacity'].forEach((id,i)=>{let el=document.getElementById(id);if(!el.matches(':focus'))el.value=[d.autoOn,d.autoOff,d.overrideMinutes,d.height,d.capacity][i]})}catch(e){document.getElementById('net').textContent='Controller not reachable'}}
refresh();setInterval(refresh,2000);
</script></body></html>
)rawliteral";

void sendMessage(int code, const String& message) {
  server.send(code, "text/plain; charset=utf-8", message);
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}

void handleStatus() {
  String json = "{";
  json += "\"levelValid\":" + String(waterLevelValid ? "true" : "false");
  json += ",\"level\":" + String(waterLevelPercent, 1);
  json += ",\"volume\":" + String(waterVolumeLitres, 1);
  json += ",\"capacity\":" + String(tankCapacityLitres, 1);
  json += ",\"height\":" + String(tankHeightCm, 1);
  json += ",\"pumpOn\":" + String(manualPumpState ? "true" : "false");
  json += ",\"override\":" + String(manualOverrideActive ? "true" : "false");
  json += ",\"overrideMinutes\":" + String(manualOverrideMinutes);
  json += ",\"autoEnabled\":" + String(autoModeEnabled ? "true" : "false");
  json += ",\"autoOn\":" + String(pumpAutoOnLevel, 1);
  json += ",\"autoOff\":" + String(pumpAutoOffLevel, 1);
  json += ",\"sensorConnected\":" + String(remoteConnected ? "true" : "false");
  json += ",\"sensorValid\":" + String(sensorReadingValid ? "true" : "false");
  json += ",\"distance\":" + String(distanceCm, 1);
  json += ",\"wifiConnected\":" + String(wifiManagerIsConnected() ? "true" : "false");
  json += ",\"apActive\":" + String(wifiManagerIsAPActive() ? "true" : "false");
  json += ",\"ip\":\"" + wifiManagerGetIP() + "\"";
  json += ",\"hostname\":\"" + wifiManagerGetHostname() + "\"";
  const char* state = "IDLE";
  if (autoPumpState == AUTO_FILLING) state = "FILLING";
  else if (autoPumpState == AUTO_RETRY_WAIT) state = "RETRY WAIT";
  else if (autoPumpState == AUTO_WAIT_SENSOR) state = "WAIT SENSOR";
  json += ",\"autoState\":\"" + String(state) + "\"}";
  server.send(200, "application/json", json);
}

void handlePumpStart() {
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
  else sendMessage(409, "Manual override was not started. Check automatic mode and pump state.");
}

void handlePumpStop() {
  if (!manualOverrideActive) {
    sendMessage(409, "No manual override is active. Automatic filling is not stopped by this button.");
    return;
  }
  stopManualOverride("web dashboard stop");
  sendMessage(200, "Manual override stopped.");
}

void handleAutoMode() {
  if (!server.hasArg("enabled")) { sendMessage(400, "Missing enabled parameter."); return; }
  bool enabled = server.arg("enabled") == "1";
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

void handleSaveWiFi() {
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
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/pump/start", HTTP_POST, handlePumpStart);
  server.on("/api/pump/stop", HTTP_POST, handlePumpStop);
  server.on("/api/auto", HTTP_POST, handleAutoMode);
  server.on("/api/settings", HTTP_POST, handleSettings);
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
