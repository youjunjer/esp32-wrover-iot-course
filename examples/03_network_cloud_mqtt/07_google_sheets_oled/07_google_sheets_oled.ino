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

#include "google_root_ca.h"

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int DHT_PIN = 14;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long SENSOR_INTERVAL_MS = 2500UL;
constexpr unsigned long SEND_INTERVAL_MS = 60000UL;
constexpr unsigned long SEND_JITTER_MS = 10000UL;
constexpr unsigned long SEND_RETRY_MS = 60000UL;
constexpr unsigned long HTTP_TIMEOUT_MS = 8000UL;
constexpr char TIME_ZONE[] = "CST-8";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";
constexpr char GOOGLE_REDIRECT_PREFIX[] = "https://script.googleusercontent.com/";
constexpr char GOOGLE_SCRIPT_PREFIX[] = "https://script.google.com/macros/s/";

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT11);
bool oledReady = false;
bool wifiAttemptActive = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool sensorValid = false;
unsigned long wifiStartedAt = 0;
unsigned long nextWifiAt = 0;
unsigned long ntpStartedAt = 0;
unsigned long nextNtpAt = 0;
unsigned long nextSensorAt = 0;
unsigned long nextSendAt = 0;
unsigned long successfulSends = 0;
float temperatureC = NAN;
float humidityPct = NAN;

bool due(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

unsigned long secondsUntil(unsigned long now, unsigned long target) {
  return due(now, target) ? 0 : (target - now + 999UL) / 1000UL;
}

bool placeholder(const char *value) {
  return value == nullptr || value[0] == '\0' || strstr(value, "REPLACE_WITH_") != nullptr;
}

bool configurationReady() {
  return !USING_EXAMPLE_SECRETS && !placeholder(WIFI_SSID) &&
         !placeholder(WIFI_PASSWORD) && !placeholder(GOOGLE_SHEETS_GAS_URL) &&
         !placeholder(GOOGLE_SHEET_ID) && !placeholder(GOOGLE_SHEET_TAG) &&
         strncmp(GOOGLE_SHEETS_GAS_URL, GOOGLE_SCRIPT_PREFIX,
                 strlen(GOOGLE_SCRIPT_PREFIX)) == 0 &&
         strstr(GOOGLE_SHEETS_GAS_URL, "/exec") != nullptr;
}

uint8_t findOledAddress() {
  constexpr uint8_t addresses[] = {0x3C, 0x3D};
  for (uint8_t address : addresses) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void drawStatus(const char *state, const char *detail, const char *footer) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("GOOGLE SHEETS");
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
      drawStatus("WIFI ERR", "SSID HIDDEN", "RETRY 10s");
    } else {
      char footer[20];
      snprintf(footer, sizeof(footer), "LEFT %lus",
               (WIFI_TIMEOUT_MS - elapsed + 999UL) / 1000UL);
      drawStatus("WIFI WAIT", "SSID HIDDEN", footer);
    }
    return false;
  }
  if (due(now, nextWifiAt)) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiStartedAt = now;
    wifiAttemptActive = true;
    drawStatus("WIFI WAIT", "SSID HIDDEN", "TIMEOUT 15s");
  } else {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus", secondsUntil(now, nextWifiAt));
    drawStatus("WIFI OFF", "SSID HIDDEN", footer);
  }
  return false;
}

bool serviceClock(unsigned long now) {
  if (clockReady) return true;
  if (!ntpAttemptActive && !due(now, nextNtpAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus",
             secondsUntil(now, nextNtpAt));
    drawStatus("TIMEOUT", "TLS NOT STARTED", footer);
    return false;
  }
  if (!ntpAttemptActive && due(now, nextNtpAt)) {
    configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
    ntpStartedAt = now;
    ntpAttemptActive = true;
    drawStatus("TIME WAIT", "TLS NOT STARTED", "TIMEOUT 12s");
  }
  if (ntpAttemptActive) {
    if (time(nullptr) > 1700000000) {
      ntpAttemptActive = false;
      clockReady = true;
      drawStatus("TIME OK", "TLS READY", "SHEET NEXT");
      return true;
    }
    if (now - ntpStartedAt >= NTP_TIMEOUT_MS) {
      ntpAttemptActive = false;
      nextNtpAt = now + NTP_RETRY_MS;
      drawStatus("TIMEOUT", "TLS NOT STARTED", "RETRY 30s");
    }
  }
  return false;
}

void readSensor(unsigned long now) {
  if (!due(now, nextSensorAt)) return;
  nextSensorAt = now + SENSOR_INTERVAL_MS;
  const float nextHumidity = dht.readHumidity();
  const float nextTemperature = dht.readTemperature();
  sensorValid = isfinite(nextTemperature) && isfinite(nextHumidity) &&
                nextTemperature >= -20.0f && nextTemperature <= 80.0f &&
                nextHumidity >= 0.0f && nextHumidity <= 100.0f;
  if (!sensorValid) {
    drawStatus("NO DATA", "DHT11 INVALID", "SEND BLOCKED");
    return;
  }
  temperatureC = nextTemperature;
  humidityPct = nextHumidity;
}

String urlEncode(const String &input) {
  static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(input.length() * 3);
  for (size_t index = 0; index < input.length(); ++index) {
    const uint8_t c = static_cast<uint8_t>(input[index]);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else {
      encoded += '%';
      encoded += HEX_DIGITS[c >> 4];
      encoded += HEX_DIGITS[c & 0x0F];
    }
  }
  return encoded;
}

int getAndFollowOnce(const String &requestUrl) {
  static const char *headerKeys[] = {"Location"};
  NetworkClientSecure firstTls;
  firstTls.setCACert(GOOGLE_ROOT_CA);
  firstTls.setHandshakeTimeout(8);
  HTTPClient first;
  first.setConnectTimeout(HTTP_TIMEOUT_MS);
  first.setTimeout(HTTP_TIMEOUT_MS);
  if (!first.begin(firstTls, requestUrl)) return -1001;
  first.collectHeaders(headerKeys, 1);
  const int firstCode = first.GET();
  if (firstCode != HTTP_CODE_FOUND && firstCode != HTTP_CODE_SEE_OTHER) {
    first.end();
    return firstCode;
  }
  const String redirectUrl = first.header("Location");
  first.end();
  if (!redirectUrl.startsWith(GOOGLE_REDIRECT_PREFIX) || redirectUrl.length() > 2048) {
    return -1002;
  }
  drawStatus("REDIRECT", "HOST VERIFIED", "FOLLOW ONCE");
  NetworkClientSecure secondTls;
  secondTls.setCACert(GOOGLE_ROOT_CA);
  secondTls.setHandshakeTimeout(8);
  HTTPClient second;
  second.setConnectTimeout(HTTP_TIMEOUT_MS);
  second.setTimeout(HTTP_TIMEOUT_MS);
  if (!second.begin(secondTls, redirectUrl)) return -1003;
  const int secondCode = second.GET();
  second.end();
  return secondCode;
}

void sendToExistingGas(unsigned long now) {
  if (!sensorValid) {
    drawStatus("NO DATA", "DHT11 INVALID", "SEND BLOCKED");
    nextSendAt = now + SENSOR_INTERVAL_MS;
    return;
  }
  String csv = String(temperatureC, 1) + ',' + String(humidityPct, 1);
  String requestUrl = GOOGLE_SHEETS_GAS_URL;
  requestUrl.reserve(requestUrl.length() + 220);
  requestUrl += F("?type=insert&dateInclude=1&sheetId=");
  requestUrl += urlEncode(GOOGLE_SHEET_ID);
  requestUrl += F("&sheetTag=");
  requestUrl += urlEncode(GOOGLE_SHEET_TAG);
  requestUrl += F("&data=");
  requestUrl += urlEncode(csv);

  drawStatus("SHEET SEND", "EXISTING GAS", "URL HIDDEN");
  const int code = getAndFollowOnce(requestUrl);
  if (code == HTTP_CODE_OK) {
    ++successfulSends;
    char detail[22];
    char footer[22];
    snprintf(detail, sizeof(detail), "HTTP 200 COUNT %lu", successfulSends);
    snprintf(footer, sizeof(footer), "CHECK SHEET 60s");
    drawStatus("HTTP OK", detail, footer);
    nextSendAt = now + SEND_INTERVAL_MS + esp_random() % SEND_JITTER_MS;
    return;
  }
  char detail[22];
  snprintf(detail, sizeof(detail), "HTTP %d", code);
  drawStatus("SEND ERR", detail, "RESULT UNKNOWN");
  nextSendAt = now + SEND_RETRY_MS + esp_random() % SEND_JITTER_MS;
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  const uint8_t address = findOledAddress();
  if (address == 0) return;
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, address);
  if (!oledReady) return;
  display.setRotation(2);
  drawStatus("BOOT", "OLED READY", "CHECK CONFIG");
  dht.begin();
  if (!configurationReady()) {
    drawStatus("CONFIG ERR", "COPY secrets.h", "GAS URL HIDDEN");
    return;
  }
  const unsigned long now = millis();
  nextWifiAt = now;
  nextSensorAt = now;
  nextSendAt = now + 5000UL + esp_random() % SEND_JITTER_MS;
}

void loop() {
  if (!oledReady) {
    blinkError(findOledAddress() == 0 ? 2 : 3);
    return;
  }
  if (!configurationReady()) return;
  const unsigned long now = millis();
  readSensor(now);
  if (!serviceWifi(now) || !serviceClock(now)) return;
  if (due(now, nextSendAt)) {
    sendToExistingGas(now);
  } else if (sensorValid && now % 2000UL < 40UL) {
    char detail[22];
    char footer[22];
    snprintf(detail, sizeof(detail), "T %.1fC H %.0f%%", temperatureC, humidityPct);
    snprintf(footer, sizeof(footer), "NEXT %lus", secondsUntil(now, nextSendAt));
    drawStatus("READY", detail, footer);
  }
}
