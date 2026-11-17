#include "TimeService.h"
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>

static Preferences prefs;
static String   ntpServer = "pool.ntp.org";
static int32_t  tzOffsetMin = 330;
static bool     timeValid = false;
static bool     ntpSyncOk = false;
static unsigned long lastNtpAttempt = 0;
static const unsigned long NTP_RETRY_MS = 60UL * 1000UL;

static void applyTz() {
  char tzbuf[32];
  int hours = tzOffsetMin / 60;
  int mins  = abs(tzOffsetMin % 60);
  snprintf(tzbuf, sizeof(tzbuf), "UTC%+d:%02d", -hours, mins);
  setenv("TZ", tzbuf, 1);
  tzset();
}

static void startSntp() {
  configTime(tzOffsetMin * 60, 0, ntpServer.c_str(), "time.nist.gov");
  lastNtpAttempt = millis();
}

void timeServiceInit() {
  prefs.begin("alarm-config", false);
  ntpServer   = prefs.getString("ntp", "pool.ntp.org");
  tzOffsetMin = (int32_t)prefs.getInt("tzmin", 330);
  prefs.end();
  applyTz();
  time_t t = time(nullptr);
  timeValid = (t > 1600000000);
  if (!timeValid) startSntp();
}

void timeServiceTick() {
  time_t now = time(nullptr);
  if (now > 1600000000) {
    if (!timeValid) {
      timeValid = true; ntpSyncOk = true;
      Serial.printf("TimeService: time acquired (%ld)\n", (long)now);
    }
  } else timeValid = false;
  if (!timeValid && millis() - lastNtpAttempt > NTP_RETRY_MS) startSntp();
}

bool   timeIsValid()     { return timeValid; }
time_t nowLocal()        { return time(nullptr); }
bool   timeNtpSynced()   { return ntpSyncOk; }
String timeGetNtpServer(){ return ntpServer; }
int32_t timeGetTimezoneMinutes() { return tzOffsetMin; }

String timeFormatHHMM() {
  if (!timeValid) return "--:--";
  struct tm tmv; time_t t = nowLocal(); localtime_r(&t, &tmv);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d", tmv.tm_hour, tmv.tm_min);
  return String(buf);
}
String dateFormatDMY() {
  if (!timeValid) return "--/--/----";
  struct tm tmv; time_t t = nowLocal(); localtime_r(&t, &tmv);
  char buf[32];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d",
           tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_year + 1900);
  return String(buf);
}

String timeFormatFull() {
  if (!timeValid) return "waiting for time...";
  struct tm tmv; time_t t = nowLocal(); localtime_r(&t, &tmv);
  char buf[40];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d",
           tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_year + 1900,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  return String(buf);
}
void timeSetManual(int year, int month, int day, int hour, int minute, int second) {
  struct tm tmv = {};
  tmv.tm_year = year - 1900; tmv.tm_mon = month - 1; tmv.tm_mday = day;
  tmv.tm_hour = hour; tmv.tm_min = minute; tmv.tm_sec = second;
  time_t t = mktime(&tmv);
  struct timeval tv = { t, 0 };
  settimeofday(&tv, nullptr);
  timeValid = true; ntpSyncOk = false;
}

void timeSetNtpServer(const String& server) {
  ntpServer = server;
  prefs.begin("alarm-config", false); prefs.putString("ntp", ntpServer); prefs.end();
  timeRequestNtpSync();
}
void timeRequestNtpSync() { startSntp(); }

void timeSetTimezoneMinutes(int32_t minutes) {
  tzOffsetMin = minutes;
  prefs.begin("alarm-config", false); prefs.putInt("tzmin", minutes); prefs.end();
  applyTz();
  configTime(tzOffsetMin * 60, 0, ntpServer.c_str(), "time.nist.gov");
}