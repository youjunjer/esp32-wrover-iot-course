#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_system.h>
#include <math.h>
#include <time.h>

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif

#include "thingspeak_root_ca.h"

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int DHT_PIN = 14;
constexpr int LIGHT_PIN = 33;
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long SENSOR_INTERVAL_MS = 2500UL;
constexpr unsigned long UPLOAD_INTERVAL_MS = 30000UL;
constexpr unsigned long UPLOAD_JITTER_MS = 5000UL;
constexpr unsigned long CLOUD_RETRY_MIN_MS = 30000UL;
constexpr unsigned long CLOUD_RETRY_MAX_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long HTTP_TIMEOUT_MS = 8000UL;
constexpr unsigned long BODY_IDLE_TIMEOUT_MS = 3000UL;
constexpr size_t MAX_BODY_BYTES = 32;
constexpr char TIME_ZONE[] = "CST-8";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";
constexpr char THINGSPEAK_URL[] = "https://api.thingspeak.com/update";

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

bool oledReady = false;
bool wifiAttemptActive = false;
bool clockReady = false;
bool ntpAttemptActive = false;
bool sensorValid = false;
unsigned long wifiStartedAt = 0;
unsigned long nextWifiAt = 0;
unsigned long ntpStartedAt = 0;
unsigned long nextNtpAt = 0;
unsigned long nextSensorAt = 0;
unsigned long nextUploadAt = 0;
unsigned long cloudBackoffMs = CLOUD_RETRY_MIN_MS;
unsigned long uploadSequence = 0;
float temperatureC = NAN;
float humidityPct = NAN;
int lightRaw = 0;

bool due(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

unsigned long secondsUntil(unsigned long now, unsigned long target) {
  return due(now, target) ? 0 : (target - now + 999UL) / 1000UL;
}

bool placeholder(const char *value) {
  return value == nullptr || value[0] == '\0' ||
         strncmp(value, "REPLACE_WITH_", 13) == 0;
}

bool configurationReady() {
  return !USING_EXAMPLE_SECRETS && !placeholder(WIFI_SSID) &&
         !placeholder(WIFI_PASSWORD) &&
         !placeholder(THINGSPEAK_WRITE_API_KEY) &&
         strlen(THINGSPEAK_WRITE_API_KEY) >= 12;
}

uint8_t findOledAddress() {
  constexpr uint8_t addresses[] = {0x3C, 0x3D};
  for (uint8_t address : addresses) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void drawStatus(const char *title, const char *state, const char *detail,
                const char *footer) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(strlen(state) > 10 ? 1 : 2);
  display.setCursor(0, strlen(state) > 10 ? 20 : 14);
  display.print(state);
  display.setTextSize(1);
  display.setCursor(0, 39);
  display.print(detail);
  display.setCursor(0, 52);
  display.print(footer);
  display.display();
}

void blinkError(uint8_t pulses) {
  for (uint8_t index = 0; index < pulses; ++index) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(160);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(160);
  }
  delay(1000);
}

bool serviceWifi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) {
    wifiAttemptActive = false;
    return true;
  }

  clockReady = false;
  ntpAttemptActive = false;
  if (wifiAttemptActive) {
    const unsigned long elapsed = now - wifiStartedAt;
    if (elapsed >= WIFI_TIMEOUT_MS) {
      WiFi.disconnect(false, false);
      wifiAttemptActive = false;
      nextWifiAt = now + WIFI_RETRY_MS;
      drawStatus("THINGSPEAK", "WIFI ERR", "SSID HIDDEN", "RETRY 10s");
    } else {
      char footer[20];
      snprintf(footer, sizeof(footer), "LEFT %lus",
               (WIFI_TIMEOUT_MS - elapsed + 999UL) / 1000UL);
      drawStatus("THINGSPEAK", "WIFI WAIT", "SSID HIDDEN", footer);
    }
    return false;
  }

  if (due(now, nextWifiAt)) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiStartedAt = now;
    wifiAttemptActive = true;
    drawStatus("THINGSPEAK", "WIFI WAIT", "SSID HIDDEN", "TIMEOUT 15s");
  } else {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus", secondsUntil(now, nextWifiAt));
    drawStatus("THINGSPEAK", "WIFI OFF", "SSID HIDDEN", footer);
  }
  return false;
}

bool serviceClock(unsigned long now) {
  if (clockReady) return true;
  if (!ntpAttemptActive && !due(now, nextNtpAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus",
             secondsUntil(now, nextNtpAt));
    drawStatus("THINGSPEAK", "TIMEOUT", "TLS NOT STARTED", footer);
    return false;
  }
  if (!ntpAttemptActive && due(now, nextNtpAt)) {
    configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
    ntpStartedAt = now;
    ntpAttemptActive = true;
    drawStatus("THINGSPEAK", "TIME WAIT", "TLS NOT STARTED", "TIMEOUT 12s");
  }

  if (ntpAttemptActive) {
    const time_t current = time(nullptr);
    if (current > 1700000000) {
      clockReady = true;
      ntpAttemptActive = false;
      drawStatus("THINGSPEAK", "TIME OK", "TLS READY", "UPLOAD NEXT");
      return true;
    }
    const unsigned long elapsed = now - ntpStartedAt;
    if (elapsed >= NTP_TIMEOUT_MS) {
      ntpAttemptActive = false;
      nextNtpAt = now + NTP_RETRY_MS;
      drawStatus("THINGSPEAK", "TIMEOUT", "TLS NOT STARTED", "RETRY 30s");
    } else {
      char footer[20];
      snprintf(footer, sizeof(footer), "LEFT %lus",
               (NTP_TIMEOUT_MS - elapsed + 999UL) / 1000UL);
      drawStatus("THINGSPEAK", "TIME WAIT", "TLS NOT STARTED", footer);
    }
  }
  return false;
}

void readSensors(unsigned long now) {
  if (!due(now, nextSensorAt)) return;
  nextSensorAt = now + SENSOR_INTERVAL_MS;
  const float newHumidity = dht.readHumidity();
  const float newTemperature = dht.readTemperature();
  const int newLight = analogRead(LIGHT_PIN);
  sensorValid = isfinite(newTemperature) && isfinite(newHumidity) &&
                newTemperature >= -20.0f && newTemperature <= 80.0f &&
                newHumidity >= 0.0f && newHumidity <= 100.0f &&
                newLight >= 0 && newLight <= 4095;
  if (!sensorValid) {
    drawStatus("THINGSPEAK", "NO DATA", "DHT/LIGHT INVALID", "UPLOAD BLOCKED");
    return;
  }
  temperatureC = newTemperature;
  humidityPct = newHumidity;
  lightRaw = newLight;
}

bool readSmallBody(HTTPClient &http, char *buffer, size_t capacity) {
  const int declared = http.getSize();
  if (declared > static_cast<int>(capacity - 1)) return false;
  NetworkClient *stream = http.getStreamPtr();
  size_t used = 0;
  unsigned long lastProgress = millis();
  while (http.connected() || stream->available()) {
    while (stream->available()) {
      if (used >= capacity - 1) return false;
      buffer[used++] = static_cast<char>(stream->read());
      lastProgress = millis();
    }
    if (declared >= 0 && used >= static_cast<size_t>(declared)) break;
    if (millis() - lastProgress >= BODY_IDLE_TIMEOUT_MS) return false;
    delay(1);
  }
  buffer[used] = '\0';
  return used > 0 && (declared < 0 || used == static_cast<size_t>(declared));
}

void scheduleCloudRetry(unsigned long now, bool rateLimited) {
  if (rateLimited) cloudBackoffMs = CLOUD_RETRY_MAX_MS;
  nextUploadAt = now + cloudBackoffMs + esp_random() % UPLOAD_JITTER_MS;
  cloudBackoffMs = min(cloudBackoffMs * 2UL, CLOUD_RETRY_MAX_MS);
}

void uploadThingSpeak(unsigned long now) {
  if (!sensorValid) {
    drawStatus("THINGSPEAK", "NO DATA", "DHT/LIGHT INVALID", "UPLOAD BLOCKED");
    nextUploadAt = now + SENSOR_INTERVAL_MS;
    return;
  }

  drawStatus("THINGSPEAK", "SEND", "TLS VERIFY", "KEY HIDDEN");
  NetworkClientSecure tls;
  tls.setCACert(THINGSPEAK_ROOT_CA);
  tls.setHandshakeTimeout(8);
  HTTPClient http;
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(tls, THINGSPEAK_URL)) {
    drawStatus("THINGSPEAK", "TLS/NET ERR", "BEGIN FAILED", "RETRY LATER");
    scheduleCloudRetry(now, false);
    return;
  }

  String form;
  form.reserve(150);
  form += F("api_key=");
  form += THINGSPEAK_WRITE_API_KEY;
  form += F("&field1=");
  form += String(temperatureC, 1);
  form += F("&field2=");
  form += String(humidityPct, 1);
  form += F("&field3=");
  form += String(lightRaw);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const int code = http.POST(form);
  char body[MAX_BODY_BYTES + 1] = {};
  const bool bodyOk = code > 0 && readSmallBody(http, body, sizeof(body));
  http.end();

  char *end = nullptr;
  const long entryId = bodyOk ? strtol(body, &end, 10) : 0;
  const bool entryOk = code == HTTP_CODE_OK && entryId > 0 && end != body &&
                       *end == '\0';
  if (entryOk) {
    ++uploadSequence;
    char detail[22];
    char footer[22];
    snprintf(detail, sizeof(detail), "ENTRY %ld", entryId);
    snprintf(footer, sizeof(footer), "SEQ %lu NEXT 30s", uploadSequence);
    drawStatus("THINGSPEAK", "HTTP 200", detail, footer);
    cloudBackoffMs = CLOUD_RETRY_MIN_MS;
    nextUploadAt = now + UPLOAD_INTERVAL_MS + esp_random() % UPLOAD_JITTER_MS;
    return;
  }

  char detail[22];
  snprintf(detail, sizeof(detail), "HTTP %d %s", code,
           bodyOk ? "ENTRY INVALID" : "BODY ERR");
  drawStatus("THINGSPEAK", code == 429 ? "RATE LIMIT" : "UPLOAD ERR",
             detail, "OLD DATA NOT OK");
  scheduleCloudRetry(now, code == 429);
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  const uint8_t address = findOledAddress();
  if (address == 0) return;
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, address);
  if (!oledReady) return;
  display.setRotation(OLED_ROTATION);
  drawStatus("THINGSPEAK", "BOOT", "OLED READY", "CHECK CONFIG");
  dht.begin();
  analogReadResolution(12);
  if (!configurationReady()) {
    drawStatus("THINGSPEAK", "CONFIG ERR", "COPY secrets.h", "KEY NOT SHOWN");
    return;
  }
  nextWifiAt = millis();
  nextSensorAt = millis();
  nextUploadAt = millis() + 5000UL + esp_random() % UPLOAD_JITTER_MS;
}

void loop() {
  if (!oledReady) {
    blinkError(findOledAddress() == 0 ? 2 : 3);
    return;
  }
  if (!configurationReady()) return;
  const unsigned long now = millis();
  readSensors(now);
  if (!serviceWifi(now) || !serviceClock(now)) return;
  if (due(now, nextUploadAt)) {
    uploadThingSpeak(now);
  } else if (sensorValid && now % 2000UL < 40UL) {
    char detail[22];
    char footer[22];
    snprintf(detail, sizeof(detail), "T %.1fC H %.0f%%", temperatureC, humidityPct);
    snprintf(footer, sizeof(footer), "LIGHT %d NEXT %lus", lightRaw,
             secondsUntil(now, nextUploadAt));
    drawStatus("THINGSPEAK", "READY", detail, footer);
  }
}
