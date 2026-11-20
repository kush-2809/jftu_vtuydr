#pragma once
#include <Arduino.h>

void featureControlInit();
bool featureLoraEnabled();
bool featureSsrEnabled();
bool featureTouchEnabled();
void featureSetLoraEnabled(bool enabled);
void featureSetSsrEnabled(bool enabled);
void featureSetTouchEnabled(bool enabled);
