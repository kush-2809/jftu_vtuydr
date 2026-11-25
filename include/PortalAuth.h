#pragma once
#include <Arduino.h>
class WebServer;

// Server-side credentials, session and forced first-login change.
void portalAuthBegin();
bool portalAuthCheck(WebServer& server);
bool portalAuthReady();
bool portalAuthMustChange();
bool portalAuthLogin(const String& password, String& session);
bool portalAuthChange(const String& oldPassword, const String& newPassword);
void portalAuthLogout();
