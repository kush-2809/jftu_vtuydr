#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include "WiFiManager.h"
#include "WebPortal.h"

namespace {
Preferences wifiPrefs;
String savedWiFiSSID = "";
String savedWiFiPassword = "";

const char* AP_SSID = "SmartTank";
const char* AP_PASSWORD = "Tank@1234";
const unsigned long WIFI_CONNECT_TIMEOUT = 15000;
const unsigned long WIFI_RETRY_INTERVAL = 10000;

bool wifiApActive = false;
bool provisioningRequested = false;
bool wifiConnecting = false;
bool wifiWasConnected = false;
bool mdnsStarted = false;
const char* MDNS_HOSTNAME = "smarttank";
unsigned long wifiConnectStartedAt = 0;
unsigned long wifiRetryAt = 0;

void startSmartTankAP() {
  if (wifiApActive) return;

  Serial.println("Starting SmartTank Access Point...");
  WiFi.mode(WIFI_AP_STA);
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    wifiApActive = true;
    Serial.println("SmartTank AP started");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    webPortalStart(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start SmartTank AP");
  }
}

void stopSmartTankAP() {
  if (!wifiApActive) return;
  Serial.println("Stopping SmartTank Access Point...");
  // Stop only captive DNS and the AP. Keep HTTP server alive on STA.
  webPortalStop();
  WiFi.softAPdisconnect(true);
  wifiApActive = false;
  Serial.println("SmartTank AP stopped");
}
} // namespace

void wifiManagerBegin() {
  Serial.println("Initializing Wi-Fi manager...");
  wifiPrefs.begin("wifi-config", false);
  webPortalConfigure();

  savedWiFiSSID = wifiPrefs.getString("ssid", "");
  savedWiFiPassword = wifiPrefs.getString("password", "");
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);

  // Keep the setup AP available while attempting the saved home network.
  startSmartTankAP();

  if (savedWiFiSSID.length() == 0) {
    Serial.println("No saved Wi-Fi credentials; setup AP is ready");
    return;
  }

  Serial.print("Connecting to saved Wi-Fi: ");
  Serial.println(savedWiFiSSID);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(savedWiFiSSID.c_str(), savedWiFiPassword.c_str());
  wifiConnecting = true;
  wifiConnectStartedAt = millis();
}

void wifiManagerUpdate() {
  const unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      Serial.println("Home Wi-Fi connected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      if (mdnsStarted) MDNS.end();
      mdnsStarted = MDNS.begin(MDNS_HOSTNAME);
      if (mdnsStarted) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS: http://smarttank.local/");
      } else {
        Serial.println("mDNS startup failed; use the IP address above");
      }
      wifiWasConnected = true;
    }
    wifiConnecting = false;
    if (wifiApActive && !provisioningRequested) {
      stopSmartTankAP();
      WiFi.mode(WIFI_STA);
    }
    return;
  }

  if (wifiWasConnected) {
    Serial.println("Home Wi-Fi disconnected");
    if (mdnsStarted) { MDNS.end(); mdnsStarted = false; }
    wifiWasConnected = false;
    startSmartTankAP();
    wifiConnecting = false;
    wifiRetryAt = now;
  }

  if (wifiConnecting && now - wifiConnectStartedAt >= WIFI_CONNECT_TIMEOUT) {
    Serial.println("Wi-Fi connection timeout");
    wifiConnecting = false;
    wifiRetryAt = now + WIFI_RETRY_INTERVAL;
    startSmartTankAP();
  }

  if (savedWiFiSSID.length() > 0 && !wifiConnecting &&
      (long)(now - wifiRetryAt) >= 0) {
    Serial.println("Retrying home Wi-Fi...");
    WiFi.mode(wifiApActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(savedWiFiSSID.c_str(), savedWiFiPassword.c_str());
    wifiConnecting = true;
    wifiConnectStartedAt = now;
  }
}

void wifiManagerHandlePortal() {
  // HTTP dashboard must be served on the home Wi-Fi as well as the setup AP.
  webPortalHandleClient();
}

void wifiManagerStartProvisioning() {
  provisioningRequested = true;
  startSmartTankAP();
}

void wifiManagerSaveCredentials(const String& ssid, const String& password) {
  provisioningRequested = false;
  savedWiFiSSID = ssid;
  savedWiFiPassword = password;
  wifiPrefs.putString("ssid", savedWiFiSSID);
  wifiPrefs.putString("password", savedWiFiPassword);

  // Keep a recovery path available while switching to the new network.
  if (mdnsStarted) { MDNS.end(); mdnsStarted = false; }
  wifiWasConnected = false;
  if (!wifiApActive) startSmartTankAP();
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(savedWiFiSSID.c_str(), savedWiFiPassword.c_str());
  wifiConnecting = true;
  wifiConnectStartedAt = millis();
  Serial.println("New Wi-Fi credentials saved; connection started");
}

bool wifiManagerIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool wifiManagerIsAPActive() {
  return wifiApActive;
}

int wifiManagerGetRSSI() {
  return wifiManagerIsConnected() ? WiFi.RSSI() : -127;
}

String wifiManagerGetHostname() {
  return String(MDNS_HOSTNAME) + ".local";
}

String wifiManagerGetSSID() {
  return wifiManagerIsConnected() ? WiFi.SSID() : String("Not connected");
}

String wifiManagerGetIP() {
  if (wifiManagerIsConnected()) return WiFi.localIP().toString();
  if (wifiApActive) return WiFi.softAPIP().toString();
  return String("0.0.0.0");
}
