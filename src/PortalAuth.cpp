#include "PortalAuth.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

namespace {
constexpr uint32_t SESSION_TTL_MS = 30UL * 60UL * 1000UL;
constexpr unsigned int KDF_ROUNDS = 25000;
uint8_t salt[16] = {};
uint8_t verifier[32] = {};
bool configured = false;
String sessionToken;
unsigned long lastSessionAt = 0;
uint8_t badLogins = 0;
unsigned long blockedUntil = 0;

void randomBytes(uint8_t* out, size_t count) {
  for (size_t i = 0; i < count; ++i) out[i] = (uint8_t)esp_random();
}
String hexEncode(const uint8_t* input, size_t n) {
  const char* digits = "0123456789abcdef";
  String out; out.reserve(n * 2);
  for (size_t i = 0; i < n; ++i) {
    out += digits[input[i] >> 4];
    out += digits[input[i] & 15];
  }
  return out;
}
bool kdf(const String& password, uint8_t out[32]) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md || mbedtls_md_setup(&ctx, md, 1) != 0) {
    mbedtls_md_free(&ctx); return false;
  }
  const int rc = mbedtls_pkcs5_pbkdf2_hmac(
      &ctx, (const unsigned char*)password.c_str(), password.length(),
      salt, sizeof(salt), KDF_ROUNDS, 32, out);
  mbedtls_md_free(&ctx);
  return rc == 0;
}
bool matches(const String& supplied) {
  uint8_t actual[32] = {};
  if (!kdf(supplied, actual)) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < sizeof(actual); ++i) diff |= actual[i] ^ verifier[i];
  return diff == 0;
}
void storeVerifier(const String& password) {
  randomBytes(salt, sizeof(salt));
  if (!kdf(password, verifier)) return;
  Preferences prefs;
  if (!prefs.begin("portal-auth", false)) return;
  prefs.putBytes("salt", salt, sizeof(salt));
  prefs.putBytes("hash", verifier, sizeof(verifier));
  prefs.putBool("set", true);
  prefs.end();
  configured = true;
}
bool sessionMatches(WebServer& server) {
  if (!sessionToken.length()) return false;
  if ((unsigned long)(millis() - lastSessionAt) >= SESSION_TTL_MS) {
    sessionToken = ""; return false;
  }
  if (!server.hasHeader("Cookie")) return false;
  String cookie = server.header("Cookie");
  int at = cookie.indexOf("stsession=");
  if (at < 0) return false;
  int begin = at + 10;
  int end = cookie.indexOf(';', begin);
  String supplied = cookie.substring(begin, end < 0 ? cookie.length() : end);
  if (supplied.length() != sessionToken.length()) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < sessionToken.length(); ++i)
    diff |= (uint8_t)(supplied[i] ^ sessionToken[i]);
  if (diff) return false;
  lastSessionAt = millis();
  return true;
}
} // namespace

void portalAuthBegin() {
  Preferences prefs;
  if (prefs.begin("portal-auth", true)) {
    configured = prefs.getBool("set", false)
        && prefs.getBytesLength("salt") == sizeof(salt)
        && prefs.getBytesLength("hash") == sizeof(verifier);
    if (configured) {
      prefs.getBytes("salt", salt, sizeof(salt));
      prefs.getBytes("hash", verifier, sizeof(verifier));
    }
    prefs.end();
  }
  if (!configured) {
    // Factory-bootstrap credential exists in firmware, never in HTML/JS.
    // First successful login must replace it.
    storeVerifier("Admin123@");
    Preferences p;
    if (p.begin("portal-auth", false)) { p.putBool("changed", false); p.end(); }
  }
}
bool portalAuthReady() { return configured; }
bool portalAuthMustChange() {
  Preferences p;
  if (!p.begin("portal-auth", true)) return true;
  bool must = !p.getBool("changed", false);
  p.end();
  return must;
}
bool portalAuthLogin(const String& password, String& session) {
  if (!configured) return false;
  if ((long)(millis() - blockedUntil) < 0) return false;
  if (!matches(password)) {
    if (++badLogins >= 5) {
      blockedUntil = millis() + 60000UL;
      badLogins = 0;
    }
    return false;
  }
  badLogins = 0;
  uint8_t token[24]; randomBytes(token, sizeof(token));
  sessionToken = hexEncode(token, sizeof(token));
  lastSessionAt = millis();
  session = sessionToken;
  return true;
}
bool portalAuthChange(const String& previous, const String& next) {
  if (next.length() < 10 || next.length() > 64 || next == "Admin123@" ||
      !matches(previous)) return false;
  storeVerifier(next);
  Preferences p;
  if (p.begin("portal-auth", false)) { p.putBool("changed", true); p.end(); }
  return true;
}
bool portalAuthCheck(WebServer& server) {
  if (!sessionMatches(server)) {
    server.send(401, "text/plain", "Admin login required");
    return false;
  }
  if (portalAuthMustChange()) {
    server.send(403, "text/plain", "Change default admin password first");
    return false;
  }
  return true;
}
void portalAuthLogout() { sessionToken = ""; }
