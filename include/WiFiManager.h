#pragma once

#include <Arduino.h>

void wifiManagerBegin();
void wifiManagerUpdate();
void wifiManagerHandlePortal();
void wifiManagerSaveCredentials(const String& ssid, const String& password);

bool wifiManagerIsConnected();
bool wifiManagerIsAPActive();
int wifiManagerGetRSSI();
String wifiManagerGetIP();
String wifiManagerGetHostname();
