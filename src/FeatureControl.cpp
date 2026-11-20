#include "FeatureControl.h"
#include <Preferences.h>
#include "PumpController.h"

static Preferences featurePrefs;
static bool loraEnabled = true;
static bool ssrEnabled = true;
static bool touchEnabled = true;

void featureControlInit() {
  featurePrefs.begin("feature-config", true);
  loraEnabled = featurePrefs.getBool("lora", true);
  ssrEnabled = featurePrefs.getBool("ssr", true);
  touchEnabled = featurePrefs.getBool("touch", true);
  featurePrefs.end();
}

bool featureLoraEnabled() { return loraEnabled; }
bool featureSsrEnabled() { return ssrEnabled; }
bool featureTouchEnabled() { return touchEnabled; }

void featureSetLoraEnabled(bool enabled) {
  loraEnabled = enabled;
  featurePrefs.begin("feature-config", false);
  featurePrefs.putBool("lora", enabled);
  featurePrefs.end();
}

void featureSetSsrEnabled(bool enabled) {
  ssrEnabled = enabled;
  featurePrefs.begin("feature-config", false);
  featurePrefs.putBool("ssr", enabled);
  featurePrefs.end();
  if (!enabled) drivePumpOutput(false);
}

void featureSetTouchEnabled(bool enabled) {
  touchEnabled = enabled;
  featurePrefs.begin("feature-config", false);
  featurePrefs.putBool("touch", enabled);
  featurePrefs.end();
}
