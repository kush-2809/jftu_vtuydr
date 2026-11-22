
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ctype.h>

#include "WiFiManager.h"
#include "WebPortal.h"

namespace {

Preferences wifiPrefs;

String savedWiFiSSID = "";
String savedWiFiPassword = "";

// ------------------------------------------
// NETWORK CONFIGURATION
// ------------------------------------------

bool useDhcp = true;

String staticIPText = "192.168.1.210";
String gatewayText  = "192.168.1.1";
String subnetText   = "255.255.255.0";
String dnsText      = "8.8.8.8";

String hostnameText = "smarttank";

String apIPText     = "192.168.4.1";
String apSubnetText = "255.255.255.0";

// ------------------------------------------
// ACCESS POINT
// ------------------------------------------

const char* AP_SSID = "SmartTank";
const char* AP_PASSWORD = "Tank@1234";

const unsigned long WIFI_CONNECT_TIMEOUT = 15000;
const unsigned long WIFI_RETRY_INTERVAL = 10000;

bool wifiApActive = false;
bool provisioningRequested = false;
bool wifiConnecting = false;
bool wifiWasConnected = false;
bool mdnsStarted = false;

unsigned long wifiConnectStartedAt = 0;
unsigned long wifiRetryAt = 0;

// ------------------------------------------
// IP VALIDATION
// ------------------------------------------

uint32_t ipToNumber(const IPAddress& ip) {
  return
      ((uint32_t)ip[0] << 24) |
      ((uint32_t)ip[1] << 16) |
      ((uint32_t)ip[2] << 8) |
      ((uint32_t)ip[3]);
}

bool validSubnetMask(const IPAddress& mask) {
  uint32_t value = ipToNumber(mask);

  bool zeroSeen = false;
  int prefix = 0;

  for (int bit = 31; bit >= 0; --bit) {
    bool set = (value & ((uint32_t)1 << bit)) != 0;

    if (set) {
      if (zeroSeen) return false;
      prefix++;
    } else {
      zeroSeen = true;
    }
  }

  return prefix >= 8 && prefix <= 30;
}

bool usableHostAddress(
    const IPAddress& ip,
    const IPAddress& mask
) {
  uint32_t hostMask = ~ipToNumber(mask);
  uint32_t hostPart = ipToNumber(ip) & hostMask;

  return hostPart != 0 && hostPart != hostMask;
}

bool sameSubnet(
    const IPAddress& a,
    const IPAddress& b,
    const IPAddress& mask
) {
  uint32_t m = ipToNumber(mask);

  return (ipToNumber(a) & m) == (ipToNumber(b) & m);
}

bool validHostname(const String& hostname) {
  if (hostname.length() < 1 || hostname.length() > 24)
    return false;

  if (hostname[0] == '-' ||
      hostname[hostname.length() - 1] == '-')
    return false;

  for (size_t i = 0; i < hostname.length(); ++i) {
    char c = hostname[i];

    if (!isalnum((unsigned char)c) && c != '-')
      return false;
  }

  return true;
}

// ------------------------------------------
// LOAD SAVED NETWORK SETTINGS
// ------------------------------------------

void loadNetworkConfig() {
  useDhcp = wifiPrefs.getBool("dhcp", true);

  staticIPText = wifiPrefs.getString(
      "ip", "192.168.1.210"
  );

  gatewayText = wifiPrefs.getString(
      "gw", "192.168.1.1"
  );

  subnetText = wifiPrefs.getString(
      "mask", "255.255.255.0"
  );

  dnsText = wifiPrefs.getString(
      "dns", "8.8.8.8"
  );

  hostnameText = wifiPrefs.getString(
      "host", "smarttank"
  );

  apIPText = wifiPrefs.getString(
      "apip", "192.168.4.1"
  );

  apSubnetText = wifiPrefs.getString(
      "apmask", "255.255.255.0"
  );

  // Validate hostname.
  if (!validHostname(hostnameText))
    hostnameText = "smarttank";

  // Validate saved AP settings.
  IPAddress apIP, apMask;

  if (!apIP.fromString(apIPText) ||
      !apMask.fromString(apSubnetText) ||
      !validSubnetMask(apMask) ||
      !usableHostAddress(apIP, apMask)) {

    apIPText = "192.168.4.1";
    apSubnetText = "255.255.255.0";
  }

  // Validate saved static configuration.
  IPAddress ip, gateway, mask, dns;

  bool validStatic =
      ip.fromString(staticIPText) &&
      gateway.fromString(gatewayText) &&
      mask.fromString(subnetText) &&
      dns.fromString(dnsText) &&
      validSubnetMask(mask) &&
      usableHostAddress(ip, mask) &&
      usableHostAddress(gateway, mask) &&
      sameSubnet(ip, gateway, mask) &&
      ipToNumber(ip) != ipToNumber(gateway);

  if (!validStatic) {
    useDhcp = true;

    staticIPText = "192.168.1.210";
    gatewayText = "192.168.1.1";
    subnetText = "255.255.255.0";
    dnsText = "8.8.8.8";

    Serial.println(
        "Invalid saved static IP configuration; using DHCP"
    );
  }
}

// ------------------------------------------
// APPLY STATION IP CONFIGURATION
// ------------------------------------------

void applyStationIPConfig() {
  if (useDhcp) {
    Serial.println("Network mode: DHCP");
    return;
  }

  IPAddress ip, gateway, mask, dns;

  if (!ip.fromString(staticIPText) ||
      !gateway.fromString(gatewayText) ||
      !mask.fromString(subnetText) ||
      !dns.fromString(dnsText)) {

    Serial.println("Invalid static configuration; DHCP remains active");
    return;
  }

  if (WiFi.config(ip, gateway, mask, dns)) {
    Serial.println("Static IP configuration applied");
    Serial.print("Static IP: ");
    Serial.println(ip);
  } else {
    Serial.println("Static IP configuration failed");
  }
}

// ------------------------------------------
// ACCESS POINT START / STOP
// ------------------------------------------

void startSmartTankAP() {
  if (wifiApActive) return;

  Serial.println("Starting SmartTank Access Point...");

  IPAddress apIP, apMask;

  if (!apIP.fromString(apIPText) ||
      !apMask.fromString(apSubnetText)) {
    apIP = IPAddress(192, 168, 4, 1);
    apMask = IPAddress(255, 255, 255, 0);
  }

  WiFi.mode(WIFI_AP_STA);

  if (!WiFi.softAPConfig(apIP, apIP, apMask)) {
    Serial.println("WARNING: AP IP configuration failed");
  }

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

  webPortalStop();
  WiFi.softAPdisconnect(true);

  wifiApActive = false;

  Serial.println("SmartTank AP stopped");
}

} // namespace

// ==========================================
// WIFI INITIALIZATION
// ==========================================

void wifiManagerBegin() {
  Serial.println("Initializing Wi-Fi manager...");

  wifiPrefs.begin("wifi-config", false);

  webPortalConfigure();

  savedWiFiSSID = wifiPrefs.getString("ssid", "");
  savedWiFiPassword = wifiPrefs.getString("password", "");

  loadNetworkConfig();

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);

  // Set hostname before starting the station connection.
  WiFi.setHostname(hostnameText.c_str());

  // Keep setup AP available while connecting.
  startSmartTankAP();

  if (savedWiFiSSID.length() == 0) {
    Serial.println("No saved Wi-Fi credentials; setup AP is ready");
    return;
  }

  Serial.print("Connecting to saved Wi-Fi: ");
  Serial.println(savedWiFiSSID);

  WiFi.mode(WIFI_AP_STA);

  applyStationIPConfig();

  WiFi.begin(
      savedWiFiSSID.c_str(),
      savedWiFiPassword.c_str()
  );

  wifiConnecting = true;
  wifiConnectStartedAt = millis();
}

// ==========================================
// WIFI UPDATE
// ==========================================

void wifiManagerUpdate() {
  const unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      Serial.println("Home Wi-Fi connected");

      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      if (mdnsStarted)
        MDNS.end();

      mdnsStarted = MDNS.begin(hostnameText.c_str());

      if (mdnsStarted) {
        MDNS.addService("http", "tcp", 80);

        Serial.print("mDNS: http://");
        Serial.print(hostnameText);
        Serial.println(".local/");
      } else {
        Serial.println("mDNS startup failed; use the IP address");
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

    if (mdnsStarted) {
      MDNS.end();
      mdnsStarted = false;
    }

    wifiWasConnected = false;

    startSmartTankAP();

    wifiConnecting = false;
    wifiRetryAt = now;
  }

  if (wifiConnecting &&
      now - wifiConnectStartedAt >= WIFI_CONNECT_TIMEOUT) {

    Serial.println("Wi-Fi connection timeout");

    wifiConnecting = false;
    wifiRetryAt = now + WIFI_RETRY_INTERVAL;

    startSmartTankAP();
  }

  if (savedWiFiSSID.length() > 0 &&
      !wifiConnecting &&
      (long)(now - wifiRetryAt) >= 0) {

    Serial.println("Retrying home Wi-Fi...");

    WiFi.mode(wifiApActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.setHostname(hostnameText.c_str());

    applyStationIPConfig();

    WiFi.begin(
        savedWiFiSSID.c_str(),
        savedWiFiPassword.c_str()
    );

    wifiConnecting = true;
    wifiConnectStartedAt = now;
  }
}

// ==========================================
// WEB PORTAL
// ==========================================

void wifiManagerHandlePortal() {
  webPortalHandleClient();
}

void wifiManagerStartProvisioning() {
  provisioningRequested = true;
  startSmartTankAP();
}

// ==========================================
// SAVE WIFI CREDENTIALS
// ==========================================

void wifiManagerSaveCredentials(
    const String& ssid,
    const String& password
) {
  provisioningRequested = false;

  savedWiFiSSID = ssid;
  savedWiFiPassword = password;

  wifiPrefs.putString("ssid", savedWiFiSSID);
  wifiPrefs.putString("password", savedWiFiPassword);

  if (mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }

  wifiWasConnected = false;

  if (!wifiApActive)
    startSmartTankAP();

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(hostnameText.c_str());

  applyStationIPConfig();

  WiFi.begin(
      savedWiFiSSID.c_str(),
      savedWiFiPassword.c_str()
  );

  wifiConnecting = true;
  wifiConnectStartedAt = millis();

  Serial.println("New Wi-Fi credentials saved; connection started");
}

// ==========================================
// SAVE NETWORK CONFIGURATION
// ==========================================

bool wifiManagerSaveNetworkConfig(
    bool dhcp,
    const String& ipText,
    const String& gatewayTextNew,
    const String& subnetTextNew,
    const String& dnsTextNew,
    const String& hostnameNew,
    const String& apIPTextNew,
    const String& apSubnetTextNew,
    String& error
) {
  IPAddress apIP, apMask;

  if (!apIP.fromString(apIPTextNew) ||
      !apMask.fromString(apSubnetTextNew) ||
      !validSubnetMask(apMask) ||
      !usableHostAddress(apIP, apMask)) {

    error = "Invalid setup AP IP or subnet mask.";
    return false;
  }

  if (!validHostname(hostnameNew)) {
    error = "Hostname must contain 1-24 letters, numbers or hyphens.";
    return false;
  }

  IPAddress ip, gateway, mask, dns;

  if (!dhcp) {
    if (!ip.fromString(ipText) ||
        !gateway.fromString(gatewayTextNew) ||
        !mask.fromString(subnetTextNew) ||
        !dns.fromString(dnsTextNew)) {

      error = "One or more static IP fields are invalid.";
      return false;
    }

    if (!validSubnetMask(mask)) {
      error = "Subnet mask is invalid.";
      return false;
    }

    if (!usableHostAddress(ip, mask) ||
        !usableHostAddress(gateway, mask)) {

      error = "IP and gateway must be usable host addresses.";
      return false;
    }

    if (!sameSubnet(ip, gateway, mask)) {
      error = "IP address and gateway must be in the same subnet.";
      return false;
    }

    if (ipToNumber(ip) == ipToNumber(gateway)) {
      error = "IP address and gateway cannot be identical.";
      return false;
    }

    if (ipToNumber(dns) == 0) {
      error = "DNS address cannot be 0.0.0.0.";
      return false;
    }
  }

  // Save all validated settings.
  useDhcp = dhcp;
  hostnameText = hostnameNew;
  apIPText = apIPTextNew;
  apSubnetText = apSubnetTextNew;

  if (!dhcp) {
    staticIPText = ipText;
    gatewayText = gatewayTextNew;
    subnetText = subnetTextNew;
    dnsText = dnsTextNew;
  }

  wifiPrefs.putBool("dhcp", useDhcp);
  wifiPrefs.putString("ip", staticIPText);
  wifiPrefs.putString("gw", gatewayText);
  wifiPrefs.putString("mask", subnetText);
  wifiPrefs.putString("dns", dnsText);
  wifiPrefs.putString("host", hostnameText);
  wifiPrefs.putString("apip", apIPText);
  wifiPrefs.putString("apmask", apSubnetText);

  error = "Network settings saved. Restart SmartTank to apply them.";
  return true;
}

// ==========================================
// NETWORK INFORMATION
// ==========================================

bool wifiManagerIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool wifiManagerIsAPActive() {
  return wifiApActive;
}

bool wifiManagerUsesDHCP() {
  return useDhcp;
}

int wifiManagerGetRSSI() {
  return wifiManagerIsConnected() ? WiFi.RSSI() : -127;
}

String wifiManagerGetHostname() {
  return hostnameText + ".local";
}

String wifiManagerGetSSID() {
  return wifiManagerIsConnected()
      ? WiFi.SSID()
      : String("Not connected");
}

String wifiManagerGetIP() {
  if (wifiManagerIsConnected())
    return WiFi.localIP().toString();

  if (wifiApActive)
    return WiFi.softAPIP().toString();

  return String("0.0.0.0");
}

String wifiManagerGetStaticIP() {
  return staticIPText;
}

String wifiManagerGetGateway() {
  return gatewayText;
}

String wifiManagerGetSubnet() {
  return subnetText;
}

String wifiManagerGetDNS() {
  return dnsText;
}

String wifiManagerGetAPIP() {
  return apIPText;
}

String wifiManagerGetAPSubnet() {
  return apSubnetText;
}
String wifiManagerGetActiveAPIP() {
  if (wifiApActive) {
    return WiFi.softAPIP().toString();
  }

  return apIPText;
}