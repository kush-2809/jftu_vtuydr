#include "WeatherService.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <math.h>

namespace {
portMUX_TYPE weatherMux = portMUX_INITIALIZER_UNLOCKED;
WeatherSnapshot shared;
bool started = false;

// Weather is informational only and must not interrupt pump/sensor tasks.
// Network work runs on a separate, low priority FreeRTOS task.
constexpr uint32_t WEATHER_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t LOCATION_CHECK_MS = 30000UL;

void loadLocation(float& latitude, float& longitude, char* city, size_t size) {
  latitude = NAN; longitude = NAN;
  if (size) city[0] = '\0';
  Preferences prefs;
  if (!prefs.begin("weather-loc", true)) return;
  String name = prefs.getString("city", "");
  if (name.length()) {
    latitude = prefs.getFloat("lat", NAN);
    longitude = prefs.getFloat("lon", NAN);
    name.toCharArray(city, size);
  }
  prefs.end();
}

// Never call from the main loop. HTTPS may take seconds while connecting.
bool fetchWeather(float latitude, float longitude, float& temp, int& code) {
  WiFiClientSecure tls;
  // Public noncritical weather only. CA verification should be added
  // for production use; never use this connection for pump commands.
  tls.setInsecure();
  tls.setTimeout(4000);

  HTTPClient http;
  const String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
      + String(latitude, 5) + "&longitude=" + String(longitude, 5)
      + "&current=temperature_2m,weather_code";
  if (!http.begin(tls, url)) return false;
  http.setTimeout(4500);
  http.setReuse(false);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    http.end(); return false;
  }
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, http.getStream());
  http.end();
  if (error) return false;
  JsonVariant current = doc["current"];
  if (current.isNull()) return false;
  const float value = current["temperature_2m"].as<float>();
  if (!isfinite(value) || value < -100 || value > 80) return false;
  temp = value;
  code = current["weather_code"].as<int>();
  return true;
}

void worker(void*) {
  float lastLat = NAN, lastLon = NAN;
  char lastCity[40] = {};
  uint32_t lastAttempt = 0;
  bool attempted = false;
  for (;;) {
    float lat, lon;
    char city[40] = {};
    loadLocation(lat, lon, city, sizeof(city));
    const bool configured = isfinite(lat) && isfinite(lon)
        && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180
        && city[0] != '\0';
    const bool locationChanged = strcmp(city, lastCity) != 0 ||
        !(isfinite(lastLat) && isfinite(lastLon)) ||
        fabs(lat - lastLat) > 0.00001f || fabs(lon - lastLon) > 0.00001f;

    if (locationChanged) {
      lastLat = lat; lastLon = lon;
      strlcpy(lastCity, city, sizeof(lastCity));
      attempted = false;
      portENTER_CRITICAL(&weatherMux);
      shared.valid = false;
      strlcpy(shared.city, city, sizeof(shared.city));
      portEXIT_CRITICAL(&weatherMux);
    }
    if (!configured && !locationChanged) {
      portENTER_CRITICAL(&weatherMux);
      shared.valid = false;
      portEXIT_CRITICAL(&weatherMux);
    }
    if (configured && WiFi.status() == WL_CONNECTED &&
        (!attempted || millis() - lastAttempt >= WEATHER_INTERVAL_MS)) {
      attempted = true;
      lastAttempt = millis();
      float temp = 0;
      int code = -1;
      if (fetchWeather(lat, lon, temp, code)) {
        portENTER_CRITICAL(&weatherMux);
        shared.valid = true;
        shared.temperatureC = temp;
        shared.weatherCode = code;
        shared.updatedAtMs = millis();
        strlcpy(shared.city, city, sizeof(shared.city));
        portEXIT_CRITICAL(&weatherMux);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_MS));
  }
}
} // namespace

void weatherServiceBegin() {
  if (started) return;
  started = true;
  BaseType_t result = xTaskCreatePinnedToCore(
      worker, "weather-service", 9216, nullptr, 1, nullptr, 0);
  if (result != pdPASS) {
    started = false;
    Serial.println("Weather task creation failed; dashboard remains available");
  }
}

WeatherSnapshot weatherServiceSnapshot() {
  WeatherSnapshot value;
  portENTER_CRITICAL(&weatherMux);
  value = shared;
  portEXIT_CRITICAL(&weatherMux);
  // Hide an outdated report if the device has been offline too long.
  if (value.valid && (uint32_t)(millis() - value.updatedAtMs) > 45UL * 60UL * 1000UL)
    value.valid = false;
  return value;
}

const char* weatherConditionLabel(int code) {
  if (code == 0) return "CLEAR";
  if (code >= 1 && code <= 3) return "CLOUDY";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "SHOWERS";
  if (code >= 85 && code <= 86) return "SNOW";
  if (code >= 95) return "STORM";
  return "WEATHER";
}
