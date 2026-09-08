#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
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
constexpr unsigned long HTTP_CONNECT_TIMEOUT_MS = 8000UL;
constexpr unsigned long HTTP_READ_TIMEOUT_MS = 8000UL;
constexpr unsigned long BODY_IDLE_TIMEOUT_MS = 5000UL;
constexpr unsigned long FETCH_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long FETCH_JITTER_MAX_MS = 30000UL;
constexpr unsigned long STARTUP_JITTER_MAX_MS = 15000UL;
constexpr unsigned long RETRY_MAX_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;
constexpr size_t MAX_JSON_BYTES = 2048;
constexpr size_t READ_BUDGET_PER_LOOP = 128;

// These are defensive transport bounds, not health thresholds or sensor ranges.
constexpr float MAX_POLLUTANT_UG_M3 = 5000.0f;
constexpr float MAX_EUROPEAN_AQI = 1000.0f;

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
// Open-Meteo air-quality data uses Copernicus CAMS model output. It is not a
// local instrument reading. Attribution must remain in redistributed lessons.

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
NetworkClientSecure tlsClient;
HTTPClient http;
char jsonBody[MAX_JSON_BYTES + 1];

struct AirQualityData {
  bool valid = false;
  float pm10 = NAN;
  float pm25 = NAN;
  float europeanAqi = NAN;
  unsigned long intervalSeconds = 0;
  unsigned long receivedMillis = 0;
  time_t sourceEpoch = 0;
  char sourceTime[20] = "";
};

AirQualityData air;
bool oledReady = false;
bool wifiAttemptActive = false;
bool wifiWasConnected = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool bodyReadActive = false;
bool hasFetchAttempt = false;
bool lastFetchSucceeded = false;
uint8_t errorCode = 0;
unsigned long wifiAttempts = 0;
unsigned long ntpAttempts = 0;
unsigned long fetchFailures = 0;
unsigned long wifiAttemptStartedMillis = 0;
unsigned long ntpAttemptStartedMillis = 0;
unsigned long bodyLastProgressMillis = 0;
unsigned long nextWifiAttemptMillis = 0;
unsigned long nextNtpAttemptMillis = 0;
unsigned long nextFetchMillis = 0;
int lastHttpCode = 0;
int lastTlsError = 0;
int expectedBodyBytes = -1;
size_t jsonBodyBytes = 0;
char lastErrorLabel[20] = "NOT STARTED";
char lastErrorDetail[22] = "";

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
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

unsigned long sourceAgeMinutes() {
  if (!air.valid || air.sourceEpoch <= 0) return 0;
  const time_t currentEpoch = time(nullptr);
  if (currentEpoch <= air.sourceEpoch) return 0;
  return capped(static_cast<unsigned long>(currentEpoch - air.sourceEpoch) /
                60UL);
}

void drawAirData(unsigned long now) {
  char line[24];

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(lastFetchSucceeded ? "OPEN-METEO/CAMS" : "AQ DATA STALE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  if (!lastFetchSucceeded) {
    display.setCursor(0, 13);
    display.print(lastErrorLabel);
  } else {
    display.setCursor(0, 13);
    display.print("EAQI ");
    display.print(air.europeanAqi, 0);
  }

  display.setCursor(0, 24);
  display.print("PM2.5 ");
  display.print(air.pm25, 1);
  display.print(" ug/m3");
  display.setCursor(0, 35);
  display.print("PM10  ");
  display.print(air.pm10, 1);
  display.print(" ug/m3");
  display.setCursor(0, 46);
  display.print("SRC ");
  display.print(air.sourceTime);

  snprintf(line, sizeof(line), "AGE %lum NEXT %lus", sourceAgeMinutes(),
           secondsUntil(now, nextFetchMillis));
  display.setCursor(0, 55);
  display.print(line);
  display.display();
}

void drawFetchFailure(unsigned long now) {
  char footer[24];
  snprintf(footer, sizeof(footer), "RETRY %lus ERR %lu",
           secondsUntil(now, nextFetchMillis), capped(fetchFailures));
  if (air.valid && (now / 3000UL) % 2UL == 0) {
    drawAirData(now);
    return;
  }
  drawStatus("AIR QUALITY", lastErrorLabel, lastErrorDetail, footer);
}

const char *wifiFailureText(wl_status_t status) {
  switch (status) {
    case WL_NO_SSID_AVAIL:
      return "NO AP";
    case WL_CONNECT_FAILED:
      return "CONNECT FAIL";
    case WL_CONNECTION_LOST:
      return "LINK LOST";
    default:
      return "TIMEOUT";
  }
}

unsigned long randomBelow(unsigned long exclusiveUpperBound);

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
    lastFetchSucceeded = false;
    if (air.valid) {
      hasFetchAttempt = true;
      strlcpy(lastErrorLabel, "LINK LOST", sizeof(lastErrorLabel));
      strlcpy(lastErrorDetail, "WIFI OFFLINE", sizeof(lastErrorDetail));
    }
    nextWifiAttemptMillis = now;
  }

  if (wifiAttemptActive) {
    const unsigned long elapsed = now - wifiAttemptStartedMillis;
    if (elapsed >= WIFI_CONNECT_TIMEOUT_MS) {
      wifiAttemptActive = false;
      WiFi.disconnect(false, false);
      nextWifiAttemptMillis = now + WIFI_RETRY_MS;
      drawStatus("WIFI", wifiFailureText(status), "SSID HIDDEN", "RETRY 10s");
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
      if (!hasFetchAttempt) {
        nextFetchMillis = now + randomBelow(STARTUP_JITTER_MAX_MS);
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

bool parseLocalIsoTime(const char *text, time_t &epoch) {
  if (text == nullptr || strlen(text) != 16) return false;
  for (size_t index = 0; index < 16; ++index) {
    const bool separator = index == 4 || index == 7 || index == 10 || index == 13;
    if (separator) continue;
    if (text[index] < '0' || text[index] > '9') return false;
  }
  if (text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
      text[13] != ':') {
    return false;
  }

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  if (sscanf(text, "%4d-%2d-%2dT%2d:%2d", &year, &month, &day, &hour,
             &minute) != 5) {
    return false;
  }
  if (year < 2020 || year > 2100 || month < 1 || month > 12 || day < 1 ||
      day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }

  struct tm source = {};
  source.tm_year = year - 1900;
  source.tm_mon = month - 1;
  source.tm_mday = day;
  source.tm_hour = hour;
  source.tm_min = minute;
  source.tm_isdst = -1;
  epoch = mktime(&source);
  return epoch > 0 && source.tm_year == year - 1900 &&
         source.tm_mon == month - 1 && source.tm_mday == day &&
         source.tm_hour == hour && source.tm_min == minute;
}

bool isJsonNumber(JsonVariantConst value) {
  return value.is<float>() || value.is<long>() ||
         value.is<unsigned long>();
}

bool validateAndStore(JsonDocument &doc, char *failureDetail,
                      size_t failureDetailSize) {
  JsonObjectConst current = doc["current"].as<JsonObjectConst>();
  JsonObjectConst units = doc["current_units"].as<JsonObjectConst>();
  if (current.isNull() || units.isNull()) {
    strlcpy(failureDetail, "MISSING OBJECT", failureDetailSize);
    return false;
  }

  if (!current["time"].is<const char *>() ||
      !current["interval"].is<unsigned long>() ||
      !isJsonNumber(current["pm10"]) || !isJsonNumber(current["pm2_5"]) ||
      !isJsonNumber(current["european_aqi"]) ||
      !units["pm10"].is<const char *>() ||
      !units["pm2_5"].is<const char *>() ||
      !units["european_aqi"].is<const char *>()) {
    strlcpy(failureDetail, "MISSING/WRONG TYPE", failureDetailSize);
    return false;
  }

  const char *sourceTime = current["time"].as<const char *>();
  const unsigned long interval = current["interval"].as<unsigned long>();
  const float pm10 = current["pm10"].as<float>();
  const float pm25 = current["pm2_5"].as<float>();
  const float europeanAqi = current["european_aqi"].as<float>();
  const char *pm10Unit = units["pm10"].as<const char *>();
  const char *pm25Unit = units["pm2_5"].as<const char *>();
  const char *aqiUnit = units["european_aqi"].as<const char *>();

  constexpr char MICROGRAMS_PER_CUBIC_METER[] =
      "\xCE\xBC" "g/m" "\xC2\xB3";
  if (strcmp(pm10Unit, MICROGRAMS_PER_CUBIC_METER) != 0 ||
      strcmp(pm25Unit, MICROGRAMS_PER_CUBIC_METER) != 0 ||
      strcmp(aqiUnit, "EAQI") != 0) {
    strlcpy(failureDetail, "UNIT ERR", failureDetailSize);
    return false;
  }

  if (!isfinite(pm10) || !isfinite(pm25) || !isfinite(europeanAqi) ||
      pm10 < 0.0f || pm25 < 0.0f || europeanAqi < 0.0f ||
      pm10 > MAX_POLLUTANT_UG_M3 || pm25 > MAX_POLLUTANT_UG_M3 ||
      europeanAqi > MAX_EUROPEAN_AQI || interval < 1UL ||
      interval > 86400UL) {
    strlcpy(failureDetail, "VALUE OUT OF RANGE", failureDetailSize);
    return false;
  }

  time_t sourceEpoch = 0;
  if (!parseLocalIsoTime(sourceTime, sourceEpoch)) {
    strlcpy(failureDetail, "BAD SOURCE TIME", failureDetailSize);
    return false;
  }

  const time_t currentEpoch = time(nullptr);
  const double ageSeconds = difftime(currentEpoch, sourceEpoch);
  if (ageSeconds < -300.0) {
    strlcpy(failureDetail, "SOURCE FUTURE", failureDetailSize);
    return false;
  }
  if (ageSeconds > 2.0 * 60.0 * 60.0) {
    strlcpy(failureDetail, "SOURCE STALE", failureDetailSize);
    return false;
  }

  air.pm10 = pm10;
  air.pm25 = pm25;
  air.europeanAqi = europeanAqi;
  air.intervalSeconds = interval;
  air.sourceEpoch = sourceEpoch;
  air.receivedMillis = millis();
  strlcpy(air.sourceTime, sourceTime, sizeof(air.sourceTime));
  air.valid = true;
  return true;
}

unsigned long randomBelow(unsigned long exclusiveUpperBound) {
  if (exclusiveUpperBound == 0) return 0;
  return static_cast<unsigned long>(esp_random()) % exclusiveUpperBound;
}

unsigned long retryDelay(int httpCode) {
  if (httpCode == 429) return RETRY_MAX_MS;
  constexpr unsigned long RETRIES[] = {60000UL, 120000UL, 240000UL,
                                        480000UL, 900000UL};
  const size_t index = fetchFailures == 0
                           ? 0
                           : (fetchFailures > 5 ? 4 : fetchFailures - 1);
  return RETRIES[index];
}

void rememberFetchFailure(const char *label, const char *detail, int httpCode,
                          int tlsError) {
  http.end();
  tlsClient.stop();
  bodyReadActive = false;
  strlcpy(lastErrorLabel, label, sizeof(lastErrorLabel));
  strlcpy(lastErrorDetail, detail, sizeof(lastErrorDetail));
  lastFetchSucceeded = false;
  hasFetchAttempt = true;
  lastHttpCode = httpCode;
  lastTlsError = tlsError;
  if (fetchFailures < DISPLAY_COUNTER_MAX) ++fetchFailures;
  nextFetchMillis =
      millis() + retryDelay(httpCode) + randomBelow(FETCH_JITTER_MAX_MS);
  drawFetchFailure(millis());
}

void finishJsonBody() {
  http.end();
  tlsClient.stop();
  bodyReadActive = false;
  jsonBody[jsonBodyBytes] = '\0';

  drawStatus("AIR QUALITY", "JSON PARSE", "ARDUINOJSON 7", "CHECK FIELDS");
  JsonDocument filter;
  filter["current_units"]["pm10"] = true;
  filter["current_units"]["pm2_5"] = true;
  filter["current_units"]["european_aqi"] = true;
  filter["current"]["time"] = true;
  filter["current"]["interval"] = true;
  filter["current"]["pm10"] = true;
  filter["current"]["pm2_5"] = true;
  filter["current"]["european_aqi"] = true;

  JsonDocument doc;
  const DeserializationError jsonError = deserializeJson(
      doc, jsonBody, jsonBodyBytes, DeserializationOption::Filter(filter),
      DeserializationOption::NestingLimit(4));
  if (jsonError) {
    rememberFetchFailure("JSON ERR", jsonError.c_str(), lastHttpCode, 0);
    return;
  }

  char validationDetail[22];
  if (!validateAndStore(doc, validationDetail, sizeof(validationDetail))) {
    const char *label = "FIELD/RANGE ERR";
    const char *detail = validationDetail;
    if (strcmp(validationDetail, "UNIT ERR") == 0) {
      label = "UNIT ERR";
      detail = "EXPECTED API UNIT";
    } else if (strcmp(validationDetail, "SOURCE FUTURE") == 0) {
      label = "SOURCE FUTURE";
      detail = "OVER +5 MIN";
    } else if (strcmp(validationDetail, "SOURCE STALE") == 0) {
      label = "SOURCE STALE";
      detail = "OLDER THAN 2H";
    }
    rememberFetchFailure(label, detail, lastHttpCode, 0);
    return;
  }

  lastFetchSucceeded = true;
  hasFetchAttempt = true;
  lastTlsError = 0;
  fetchFailures = 0;
  nextFetchMillis = millis() + FETCH_INTERVAL_MS +
                    randomBelow(FETCH_JITTER_MAX_MS);
  drawAirData(millis());
}

void startAirQualityFetch() {
  drawStatus("AIR QUALITY", "TLS VERIFY", "CA ISRG ROOT X1",
             "NO INSECURE MODE");
  tlsClient.stop();
  tlsClient.setCACert(OPEN_METEO_ROOT_CA);
  http.end();
  http.useHTTP10(true);
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_READ_TIMEOUT_MS);

  if (!http.begin(tlsClient, API_URL)) {
    rememberFetchFailure("BEGIN ERR", "URL/CLIENT", 0, 0);
    return;
  }

  drawStatus("AIR QUALITY", "HTTPS GET", "OPEN-METEO/CAMS", "WAIT RESPONSE");
  const int httpCode = http.GET();
  if (httpCode <= 0) {
    char tlsMessage[80];
    const int tlsError = tlsClient.lastError(tlsMessage, sizeof(tlsMessage));
    char detail[22];
    snprintf(detail, sizeof(detail), "GET %d TLS %d", httpCode, tlsError);
    rememberFetchFailure("TLS/NET ERR", detail, httpCode, tlsError);
    return;
  }

  if (httpCode != HTTP_CODE_OK) {
    char detail[22];
    snprintf(detail, sizeof(detail), "HTTP %d", httpCode);
    rememberFetchFailure("HTTP ERR", detail, httpCode, 0);
    return;
  }

  expectedBodyBytes = http.getSize();
  if (expectedBodyBytes > static_cast<int>(MAX_JSON_BYTES)) {
    rememberFetchFailure("BODY ERR", "OVER 2048 BYTES", httpCode, 0);
    return;
  }
  lastHttpCode = httpCode;
  jsonBodyBytes = 0;
  jsonBody[0] = '\0';
  bodyLastProgressMillis = millis();
  bodyReadActive = true;
  drawStatus("AIR QUALITY", "JSON READ", "MAX 2048 BYTES", "FILTER + LIMIT 4");
}

void serviceJsonBody(unsigned long now) {
  NetworkClient &stream = http.getStream();
  int available = stream.available();
  size_t budget = READ_BUDGET_PER_LOOP;
  for (; available > 0 && budget > 0; --available, --budget) {
    if (jsonBodyBytes >= MAX_JSON_BYTES) {
      rememberFetchFailure("BODY ERR", "OVER 2048 BYTES", lastHttpCode, 0);
      return;
    }
    const int value = stream.read();
    if (value < 0) break;
    jsonBody[jsonBodyBytes++] = static_cast<char>(value);
    bodyLastProgressMillis = now;
  }
  jsonBody[jsonBodyBytes] = '\0';

  if (expectedBodyBytes >= 0 &&
      jsonBodyBytes >= static_cast<size_t>(expectedBodyBytes)) {
    finishJsonBody();
    return;
  }
  if (!http.connected() && stream.available() == 0) {
    if (expectedBodyBytes >= 0 &&
        jsonBodyBytes < static_cast<size_t>(expectedBodyBytes)) {
      rememberFetchFailure("BODY ERR", "SHORT BODY", lastHttpCode, 0);
      return;
    }
    finishJsonBody();
    return;
  }
  if (now - bodyLastProgressMillis >= BODY_IDLE_TIMEOUT_MS) {
    rememberFetchFailure("BODY ERR", "READ TIMEOUT", lastHttpCode, 0);
    return;
  }

  char detail[22];
  snprintf(detail, sizeof(detail), "BODY %u/2048",
           static_cast<unsigned int>(jsonBodyBytes));
  drawStatus("AIR QUALITY", "JSON READ", detail, "READ BY LOOP");
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
  drawStatus("AIR QUALITY", "BOOT", "OLED READY", "WIFI NEXT");
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
    rememberFetchFailure("LINK LOST", "DURING BODY", lastHttpCode, 0);
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
    serviceJsonBody(now);
    delay(2);
    return;
  }
  if (isDue(now, nextFetchMillis)) {
    startAirQualityFetch();
    return;
  }

  if (!hasFetchAttempt) {
    char footer[22];
    snprintf(footer, sizeof(footer), "START IN %lus",
             secondsUntil(now, nextFetchMillis));
    drawStatus("AIR QUALITY", "JITTER", "CLASS LOAD SPREAD", footer);
  } else if (lastFetchSucceeded) {
    drawAirData(now);
  } else {
    drawFetchFailure(now);
  }
  delay(100);
}
