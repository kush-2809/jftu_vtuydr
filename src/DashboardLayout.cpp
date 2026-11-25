#include "DashboardLayout.h"
#include <Preferences.h>
namespace {
uint8_t active = 0; // 0 = Aqua (phone-inspired), 1 = Compact dashboard
}
void dashboardLayoutInit() {
  Preferences p;
  if (p.begin("display-config", true)) {
    active = p.getUChar("layout", 0);
    p.end();
  }
  if (active > 1) active = 0;
}
uint8_t dashboardLayoutIndex() { return active; }
uint8_t dashboardLayoutCount() { return 2; }
void dashboardLayoutSet(uint8_t next) {
  if (next > 1 || active == next) return;
  active = next;
  Preferences p;
  if (p.begin("display-config", false)) {
    p.putUChar("layout", active);
    p.end();
  }
}
