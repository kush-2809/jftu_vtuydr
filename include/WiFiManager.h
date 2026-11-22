
#pragma once

#include <Arduino.h>
String wifiManagerGetActiveAPIP();
void wifiManagerBegin();
void wifiManagerUpdate();
void wifiManagerHandlePortal();

void wifiManagerSaveCredentials(
    const String& ssid,
    const String& password
);

void wifiManagerStartProvisioning();

bool wifiManagerIsConnected();
bool wifiManagerIsAPActive();
bool wifiManagerUsesDHCP();

int wifiManagerGetRSSI();

String wifiManagerGetIP();
String wifiManagerGetHostname();
String wifiManagerGetSSID();

String wifiManagerGetStaticIP();
String wifiManagerGetGateway();
String wifiManagerGetSubnet();
String wifiManagerGetDNS();
String wifiManagerGetAPIP();
String wifiManagerGetAPSubnet();

// Save and validate network settings.
// Settings are applied after the next reboot.
bool wifiManagerSaveNetworkConfig(
    bool useDhcp,
    const String& ip,
    const String& gateway,
    const String& subnet,
    const String& dns,
    const String& hostname,
    const String& apIp,
    const String& apSubnet,
    String& error
);
