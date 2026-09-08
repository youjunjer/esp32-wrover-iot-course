#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_system.h>
#include <time.h>

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif
#include "open_meteo_root_ca.h"

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long HTTP_TIMEOUT_MS = 8000UL;
constexpr unsigned long BODY_IDLE_TIMEOUT_MS = 5000UL;
constexpr unsigned long HTTP_SUCCESS_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long HTTP_JITTER_MAX_MS = 30000UL;
constexpr unsigned long STARTUP_JITTER_MAX_MS = 15000UL;
constexpr unsigned long HTTP_RETRY_MAX_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;
constexpr size_t MAX_BODY_BYTES = 2048;
constexpr size_t READ_BUDGET_PER_LOOP = 128;
constexpr char TIME_ZONE[] = "CST-8";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";
constexpr char API_URL[] =
    "https://air-quality-api.open-meteo.com/v1/air-quality"
    "?latitude=25.0330&longitude=121.5654"
    "&current=pm10,pm2_5,european_aqi"
    "&timezone=Asia%2FTaipei";

// Data source and attribution:
// https://open-meteo.com/en/docs/air-quality-api
// This chapter only counts the bounded response body. The endpoint still uses
// Open-Meteo air-quality data derived from Copernicus CAMS model output.

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
NetworkClientSecure tlsClient;
HTTPClient http;

bool oledReady = false;
bool wifiAttemptActive = false;
bool wifiWasConnected = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool bodyReadActive = false;
bool hasHttpResult = false;
bool lastHttpSucceeded = false;
uint8_t errorCode = 0;
unsigned long wifiAttempts = 0;
unsigned long ntpAttempts = 0;
unsigned long httpFailures = 0;
unsigned long wifiAttemptStartedMillis = 0;
unsigned long ntpAttemptStartedMillis = 0;
unsigned long bodyLastProgressMillis = 0;
unsigned long nextWifiAttemptMillis = 0;
unsigned long nextNtpAttemptMillis = 0;
unsigned long nextHttpAttemptMillis = 0;
int lastHttpCode = 0;
int lastTlsError = 0;
int expectedBodyBytes = -1;
size_t lastBodyBytes = 0;
char lastFailureLabel[20] = "NOT STARTED";

uint8_t findOledAddress() {
  constexpr uint8_t ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void blinkErrorCode(uint8_t code) {
  for (uint8_t pulse = 0; pulse < code; ++pulse) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(160);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(160);
  }
  delay(1000);
}

unsigned long capped(unsigned long value) {
  return value > DISPLAY_COUNTER_MAX ? DISPLAY_COUNTER_MAX : value;
}

bool isDue(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

unsigned long secondsUntil(unsigned long now, unsigned long target) {
  if (isDue(now, target)) return 0;
  return capped((target - now + 999UL) / 1000UL);
}

unsigned long randomBelow(unsigned long exclusiveUpperBound) {
  if (exclusiveUpperBound == 0) return 0;
  return static_cast<unsigned long>(esp_random()) % exclusiveUpperBound;
}

void drawStatus(const char *title, const char *state, const char *detail,
                const char *footer) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  const bool compactState = strlen(state) > 10;
  display.setTextSize(compactState ? 1 : 2);
  display.setCursor(0, compactState ? 20 : 14);
  display.print(state);
  display.setTextSize(1);
  display.setCursor(0, 39);
  display.print(detail);
  display.setCursor(0, 52);
  display.print(footer);
  display.display();
}

bool startsWithPlaceholder(const char *value) {
  return value == nullptr || strncmp(value, "REPLACE_WITH_", 13) == 0;
}

bool secretsNeedConfiguration() {
  return USING_EXAMPLE_SECRETS || startsWithPlaceholder(WIFI_SSID) ||
         startsWithPlaceholder(WIFI_PASSWORD) || strlen(WIFI_SSID) == 0;
}

void drawHttpResult(unsigned long now) {
  char footer[24];
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("HTTPS GET");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.print(lastHttpSucceeded ? "TLS VERIFIED" : lastFailureLabel);
  display.setCursor(0, 27);
  display.print(lastHttpCode > 0 ? "HTTP " : "GET ");
  display.print(lastHttpCode);
  display.setCursor(0, 40);
  display.print("TLS ");
  display.print(lastTlsError);
  display.print(" BODY ");
  display.print(lastBodyBytes);
  snprintf(footer, sizeof(footer), "%s %lus ERR %lu",
           lastHttpSucceeded ? "NEXT" : "RETRY",
           secondsUntil(now, nextHttpAttemptMillis), capped(httpFailures));
  display.setCursor(0, 53);
  display.print(footer);
  display.display();
}

const char *wifiFailureText(wl_status_t status) {
  if (status == WL_NO_SSID_AVAIL) return "NO AP";
  if (status == WL_CONNECT_FAILED) return "CONNECT FAIL";
  if (status == WL_CONNECTION_LOST) return "LINK LOST";
  return "TIMEOUT";
}

void startWifiAttempt(unsigned long now) {
  if (wifiAttempts < DISPLAY_COUNTER_MAX) ++wifiAttempts;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect(false, false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiAttemptStartedMillis = now;
  wifiAttemptActive = true;
  drawStatus("WIFI", "CONNECT", "SSID HIDDEN", "TIMEOUT 15s");
}

bool serviceWifi(unsigned long now) {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiAttemptActive = false;
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      char footer[22];
      snprintf(footer, sizeof(footer), "RSSI %d dBm", WiFi.RSSI());
      drawStatus("WIFI", "ONLINE", "IP READY", footer);
      if (!clockReady) nextNtpAttemptMillis = now;
    }
    return true;
  }

  if (wifiWasConnected) {
    wifiWasConnected = false;
    wifiAttemptActive = false;
    clockReady = false;
    ntpAttemptActive = false;
    if (hasHttpResult) {
      lastHttpSucceeded = false;
      strlcpy(lastFailureLabel, "LINK LOST", sizeof(lastFailureLabel));
    }
    nextWifiAttemptMillis = now;
  }

  if (wifiAttemptActive) {
    const unsigned long elapsed = now - wifiAttemptStartedMillis;
    if (elapsed >= WIFI_CONNECT_TIMEOUT_MS) {
      wifiAttemptActive = false;
      WiFi.disconnect(false, false);
      nextWifiAttemptMillis = now + WIFI_RETRY_MS;
      drawStatus("WIFI", wifiFailureText(status), "SSID HIDDEN",
                 "RETRY 10s");
    } else {
      char detail[22];
      char footer[22];
      snprintf(detail, sizeof(detail), "TRY %lu", capped(wifiAttempts));
      snprintf(footer, sizeof(footer), "LEFT %lus",
               capped((WIFI_CONNECT_TIMEOUT_MS - elapsed + 999UL) / 1000UL));
      drawStatus("WIFI", "CONNECT", detail, footer);
    }
    return false;
  }

  if (isDue(now, nextWifiAttemptMillis)) {
    startWifiAttempt(now);
  } else {
    char footer[22];
    snprintf(footer, sizeof(footer), "RETRY %lus",
             secondsUntil(now, nextWifiAttemptMillis));
    drawStatus("WIFI", "OFFLINE", "SSID HIDDEN", footer);
  }
  return false;
}

bool serviceNtp(unsigned long now) {
  if (clockReady) return true;
  if (!ntpAttemptActive && isDue(now, nextNtpAttemptMillis)) {
    if (ntpAttempts < DISPLAY_COUNTER_MAX) ++ntpAttempts;
    configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
    ntpAttemptStartedMillis = now;
    ntpAttemptActive = true;
    drawStatus("NTP", "SYNC", "CST-8", "TIMEOUT 12s");
    return false;
  }

  if (ntpAttemptActive) {
    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 0)) {
      char timeText[22];
      strftime(timeText, sizeof(timeText), "%m-%d %H:%M:%S", &timeInfo);
      ntpAttemptActive = false;
      clockReady = true;
      if (!hasHttpResult) {
        nextHttpAttemptMillis = now + randomBelow(STARTUP_JITTER_MAX_MS);
      }
      drawStatus("NTP", "TIME OK", timeText, "TLS READY");
      return true;
    }
    const unsigned long elapsed = now - ntpAttemptStartedMillis;
    if (elapsed >= NTP_SYNC_TIMEOUT_MS) {
      ntpAttemptActive = false;
      nextNtpAttemptMillis = now + NTP_RETRY_MS;
      drawStatus("NTP", "TIMEOUT", "TLS NOT STARTED", "RETRY 30s");
    } else {
      char detail[22];
      char footer[22];
      snprintf(detail, sizeof(detail), "TRY %lu", capped(ntpAttempts));
      snprintf(footer, sizeof(footer), "LEFT %lus",
               capped((NTP_SYNC_TIMEOUT_MS - elapsed + 999UL) / 1000UL));
      drawStatus("NTP", "SYNC", detail, footer);
    }
    return false;
  }

  char footer[22];
  snprintf(footer, sizeof(footer), "RETRY %lus",
           secondsUntil(now, nextNtpAttemptMillis));
  drawStatus("NTP", "TIMEOUT", "TLS NOT STARTED", footer);
  return false;
}

unsigned long httpRetryDelay(int httpCode) {
  if (httpCode == 429) return HTTP_RETRY_MAX_MS;
  constexpr unsigned long RETRIES[] = {60000UL, 120000UL, 240000UL,
                                        480000UL, 900000UL};
  const size_t index = httpFailures == 0
                           ? 0
                           : (httpFailures > 5 ? 4 : httpFailures - 1);
  return RETRIES[index];
}

void rememberHttpFailure(const char *label, int httpCode, int tlsError,
                         size_t bodyBytes) {
  http.end();
  tlsClient.stop();
  bodyReadActive = false;
  strlcpy(lastFailureLabel, label, sizeof(lastFailureLabel));
  lastHttpSucceeded = false;
  hasHttpResult = true;
  lastHttpCode = httpCode;
  lastTlsError = tlsError;
  lastBodyBytes = bodyBytes;
  if (httpFailures < DISPLAY_COUNTER_MAX) ++httpFailures;
  nextHttpAttemptMillis = millis() + httpRetryDelay(httpCode) +
                          randomBelow(HTTP_JITTER_MAX_MS);
  drawHttpResult(millis());
}

void finishBodyRead() {
  if (lastBodyBytes == 0) {
    rememberHttpFailure("BODY EMPTY", lastHttpCode, 0, 0);
    return;
  }
  http.end();
  tlsClient.stop();
  bodyReadActive = false;
  lastHttpSucceeded = true;
  hasHttpResult = true;
  lastTlsError = 0;
  httpFailures = 0;
  nextHttpAttemptMillis = millis() + HTTP_SUCCESS_INTERVAL_MS +
                          randomBelow(HTTP_JITTER_MAX_MS);
  drawHttpResult(millis());
}

void startHttpsGet() {
  drawStatus("HTTPS", "TLS VERIFY", "CA ISRG ROOT X1", "NO INSECURE MODE");
  tlsClient.stop();
  tlsClient.setCACert(OPEN_METEO_ROOT_CA);
  http.end();
  http.useHTTP10(true);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(tlsClient, API_URL)) {
    rememberHttpFailure("BEGIN ERR", 0, 0, 0);
    return;
  }

  drawStatus("HTTPS", "GET", "TLS CA CHECK ON", "MAX WAIT 8s");
  const int httpCode = http.GET();
  if (httpCode <= 0) {
    char tlsMessage[80];
    const int tlsError = tlsClient.lastError(tlsMessage, sizeof(tlsMessage));
    rememberHttpFailure("TLS/NET ERR", httpCode, tlsError, 0);
    return;
  }
  if (httpCode != HTTP_CODE_OK) {
    rememberHttpFailure("HTTP ERR", httpCode, 0, 0);
    return;
  }

  expectedBodyBytes = http.getSize();
  if (expectedBodyBytes > static_cast<int>(MAX_BODY_BYTES)) {
    rememberHttpFailure("BODY TOO LARGE", httpCode, 0,
                        static_cast<size_t>(expectedBodyBytes));
    return;
  }
  lastHttpCode = httpCode;
  lastBodyBytes = 0;
  bodyLastProgressMillis = millis();
  bodyReadActive = true;
  drawStatus("HTTPS", "HTTP 200", "BODY 0/2048 B", "READ BY LOOP");
}

void serviceBodyRead(unsigned long now) {
  NetworkClient &stream = http.getStream();
  size_t budget = READ_BUDGET_PER_LOOP;
  int available = stream.available();
  for (; available > 0 && budget > 0; --available, --budget) {
    if (lastBodyBytes >= MAX_BODY_BYTES) {
      rememberHttpFailure("BODY TOO LARGE", lastHttpCode, 0, lastBodyBytes);
      return;
    }
    if (stream.read() < 0) break;
    ++lastBodyBytes;
    bodyLastProgressMillis = now;
  }
  if (expectedBodyBytes >= 0 &&
      lastBodyBytes >= static_cast<size_t>(expectedBodyBytes)) {
    finishBodyRead();
    return;
  }
  if (!http.connected() && stream.available() == 0) {
    if (expectedBodyBytes >= 0 &&
        lastBodyBytes < static_cast<size_t>(expectedBodyBytes)) {
      rememberHttpFailure("BODY SHORT", lastHttpCode, 0, lastBodyBytes);
      return;
    }
    finishBodyRead();
    return;
  }
  if (now - bodyLastProgressMillis >= BODY_IDLE_TIMEOUT_MS) {
    rememberHttpFailure("BODY TIMEOUT", lastHttpCode, 0, lastBodyBytes);
    return;
  }
  char detail[22];
  snprintf(detail, sizeof(detail), "BODY %u/2048 B",
           static_cast<unsigned int>(lastBodyBytes));
  drawStatus("HTTPS", "HTTP 200", detail, "READ BY LOOP");
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setTimeOut(50);
  delay(50);
  const uint8_t address = findOledAddress();
  if (address == 0) {
    errorCode = ERROR_NO_OLED;
    return;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    errorCode = ERROR_DISPLAY_INIT;
    return;
  }
  display.setTextWrap(false);
  display.setRotation(OLED_ROTATION);
  oledReady = true;
  drawStatus("HTTPS GET", "BOOT", "OLED READY", "WIFI NEXT");
  nextWifiAttemptMillis = millis();
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }
  if (secretsNeedConfiguration()) {
    drawStatus("WIFI", "CONFIG ERR", "COPY secrets.h", "FILL REPLACE...");
    delay(100);
    return;
  }

  const unsigned long now = millis();
  if (bodyReadActive && WiFi.status() != WL_CONNECTED) {
    rememberHttpFailure("LINK LOST", lastHttpCode, 0, lastBodyBytes);
  }
  if (!serviceWifi(now)) {
    delay(25);
    return;
  }
  if (!serviceNtp(now)) {
    delay(25);
    return;
  }
  if (bodyReadActive) {
    serviceBodyRead(now);
    delay(2);
    return;
  }
  if (isDue(now, nextHttpAttemptMillis)) {
    startHttpsGet();
    return;
  }
  if (hasHttpResult) {
    drawHttpResult(now);
  } else {
    char footer[22];
    snprintf(footer, sizeof(footer), "START IN %lus",
             secondsUntil(now, nextHttpAttemptMillis));
    drawStatus("HTTPS", "JITTER", "CLASS LOAD SPREAD", footer);
  }
  delay(100);
}
