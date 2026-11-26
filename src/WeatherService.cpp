#include "WeatherService.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>

namespace {
portMUX_TYPE weatherMux = portMUX_INITIALIZER_UNLOCKED;
WeatherSnapshot shared;
bool started = false;

// Weather is informational only and must not interrupt pump/sensor tasks.
// Network work runs on a separate, low priority FreeRTOS task.
constexpr uint32_t WEATHER_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t WEATHER_RETRY_MS = 60UL * 1000UL;
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
void statusMessage(const char* status) {
  portENTER_CRITICAL(&weatherMux);
  strlcpy(shared.status, status, sizeof(shared.status));
  portEXIT_CRITICAL(&weatherMux);
}
bool fetchWeather(float latitude, float longitude, float& temp, int& code) {
  WiFiClientSecure tls;
  // Only informational weather data, never pump controls.
  // TODO: validate the Open-Meteo certificate chain in production.
  tls.setInsecure();
  tls.setTimeout(5000);

  HTTPClient http;
  const String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
      + String(latitude, 5) + "&longitude=" + String(longitude, 5)
      + "&current=temperature_2m,weather_code";
  // HTTP/1.0 avoids chunked stream parsing problems in some ESP32 cores.
  http.useHTTP10(true);
  if (!http.begin(tls, url)) {
    statusMessage("HTTPS setup failed");
    Serial.println("[Weather] HTTPS setup failed");
    return false;
  }
  http.setTimeout(6500);
  http.setReuse(false);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    http.end();
    statusMessage("Weather HTTP error");
    Serial.printf("[Weather] HTTP error %d\n", status);
    return false;
  }
  const int length = http.getSize();
  if (length > 4096) {
    http.end();
    statusMessage("Weather response too large");
    return false;
  }
  const String body = http.getString();
  http.end();
  if (body.length() == 0 || body.length() > 4096) {
    statusMessage("Weather body missing");
    return false;
  }
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, body);
  if (error) {
    statusMessage("Weather JSON parse failed");
    Serial.printf("[Weather] JSON error: %s\n", error.c_str());
    return false;
  }
  JsonVariant current = doc["current"];
  if (current.isNull() || current["temperature_2m"].isNull()
      || current["weather_code"].isNull()) {
    statusMessage("Weather fields missing");
    return false;
  }
  const float value = current["temperature_2m"].as<float>();
  const int weather = current["weather_code"].as<int>();
  if (!isfinite(value) || value < -100 || value > 80 || weather < 0 || weather > 99) {
    statusMessage("Weather values invalid");
    return false;
  }
  temp = value;
  code = weather;
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
      // Do not discard a just-synced portal forecast for the same city.
      const bool keepPortalForecast =
          configured && shared.valid && strcmp(shared.city, city) == 0;
      if (!keepPortalForecast) {
        shared.valid = false;
        strlcpy(shared.status, configured ? "New location" : "Set city in portal",
                sizeof(shared.status));
      }
      strlcpy(shared.city, city, sizeof(shared.city));
      portEXIT_CRITICAL(&weatherMux);
    }
    if (!configured && !locationChanged) {
      portENTER_CRITICAL(&weatherMux);
      shared.valid = false;
      strlcpy(shared.status, "Set city in portal", sizeof(shared.status));
      portEXIT_CRITICAL(&weatherMux);
    }
    if (configured && WiFi.status() != WL_CONNECTED && !attempted) {
      statusMessage("ESP Wi-Fi offline");
    }
    WeatherSnapshot cached;
    portENTER_CRITICAL(&weatherMux);
    cached = shared;
    portEXIT_CRITICAL(&weatherMux);
    const uint32_t interval = cached.valid ? WEATHER_INTERVAL_MS : WEATHER_RETRY_MS;
    if (configured && WiFi.status() == WL_CONNECTED &&
        (!attempted || millis() - lastAttempt >= interval)) {
      attempted = true;
      lastAttempt = millis();
      float temp = 0;
      int code = -1;
      if (fetchWeather(lat, lon, temp, code)) {
        portENTER_CRITICAL(&weatherMux);
        shared.valid = true;
        shared.temperatureC = temp;
        shared.weatherCode = code;
        strlcpy(shared.status, "ESP weather updated", sizeof(shared.status));
        shared.updatedAtMs = millis();
        strlcpy(shared.city, city, sizeof(shared.city));
        portEXIT_CRITICAL(&weatherMux);
        Serial.printf("[Weather] %s %.1f C code=%d\n", city, temp, code);
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

bool weatherServiceAcceptPortal(float latitude, float longitude,
                                float temperatureC, int weatherCode) {
  if (!isfinite(latitude) || !isfinite(longitude) ||
      !isfinite(temperatureC) || temperatureC < -100 || temperatureC > 80 ||
      weatherCode < 0 || weatherCode > 99) return false;
  float configuredLat, configuredLon;
  char city[40] = {};
  loadLocation(configuredLat, configuredLon, city, sizeof(city));
  if (!isfinite(configuredLat) || !isfinite(configuredLon) || !city[0] ||
      fabsf(configuredLat - latitude) > 0.005f ||
      fabsf(configuredLon - longitude) > 0.005f) return false;
  portENTER_CRITICAL(&weatherMux);
  // Location may have changed while reading Preferences. Next background
  // pass invalidates this cache if a different location is configured.
  shared.temperatureC = temperatureC;
  shared.weatherCode = weatherCode;
  shared.updatedAtMs = millis();
  shared.valid = true;
  strlcpy(shared.city, city, sizeof(shared.city));
  strlcpy(shared.status, "Portal weather synced", sizeof(shared.status));
  portEXIT_CRITICAL(&weatherMux);
  return true;
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
