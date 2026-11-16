#pragma once

#include <Arduino.h>
#include <IPAddress.h>

void webPortalConfigure();
void webPortalStart(const IPAddress& apAddress);
void webPortalStop();
void webPortalHandleClient();
