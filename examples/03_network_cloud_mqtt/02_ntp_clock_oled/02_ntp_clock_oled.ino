#include <Arduino.h>
#include <atomic>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_sntp.h>
#include <time.h>

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 15000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long NTP_STALE_MS = 90UL * 60UL * 1000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;
constexpr unsigned long WIFI_BACKOFF_MS[] = {
    3000UL, 6000UL, 12000UL, 30000UL};
constexpr time_t VALID_EPOCH = 1704067200;
constexpr char TIME_ZONE[] = "CST-8";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

enum class ClockState {
  CONFIG_ERR,
  WIFI_CONNECT_START,
  WIFI_CONNECT_WAIT,
  WIFI_BACKOFF,
  NTP_CONFIG,
  NTP_SYNC_WAIT,
  NTP_BACKOFF,
  TIME_OK
};

enum class WifiFailure { NONE, NO_AP, CONNECT_ERR, TIMEOUT, LINK_LOST };

std::atomic<bool> timeSyncNotified{false};
uint8_t errorCode = 0;
bool oledReady = false;
bool hasValidTime = false;
ClockState clockState = ClockState::CONFIG_ERR;
WifiFailure wifiFailure = WifiFailure::NONE;
unsigned long stateSinceMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long lastSyncMillis = 0;
unsigned long wifiBackoffDurationMillis = WIFI_BACKOFF_MS[0];
uint8_t wifiBackoffIndex = 0;

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

bool hasPlaceholderSecrets() {
  return USING_EXAMPLE_SECRETS || WIFI_SSID[0] == '\0' ||
         strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0 ||
         strcmp(WIFI_PASSWORD, "YOUR_WIFI_PASSWORD") == 0;
}

void noteTimeSync(struct timeval *timeValue) {
  (void)timeValue;
  timeSyncNotified.store(true, std::memory_order_release);
}

void changeState(ClockState nextState, unsigned long now) {
  clockState = nextState;
  stateSinceMillis = now;
}

unsigned long cappedSeconds(unsigned long milliseconds) {
  const unsigned long seconds = milliseconds / 1000UL;
  return seconds > DISPLAY_COUNTER_MAX ? DISPLAY_COUNTER_MAX : seconds;
}

unsigned long secondsRemaining(unsigned long now, unsigned long duration) {
  const unsigned long elapsed = now - stateSinceMillis;
  if (elapsed >= duration) return 0;
  return cappedSeconds(duration - elapsed + 999UL);
}

bool clockIsValid() {
  return time(nullptr) >= VALID_EPOCH;
}

const char *wifiFailureText(WifiFailure failure) {
  switch (failure) {
    case WifiFailure::NO_AP:
      return "NO AP";
    case WifiFailure::CONNECT_ERR:
      return "CONNECT ERR";
    case WifiFailure::TIMEOUT:
      return "WIFI TIMEOUT";
    case WifiFailure::LINK_LOST:
      return "NET LOST";
    default:
      return "WIFI ERR";
  }
}

void startWifiBackoff(WifiFailure failure, unsigned long now) {
  wifiFailure = failure;
  wifiBackoffDurationMillis = WIFI_BACKOFF_MS[wifiBackoffIndex];
  if (wifiBackoffIndex + 1 <
      sizeof(WIFI_BACKOFF_MS) / sizeof(WIFI_BACKOFF_MS[0])) {
    ++wifiBackoffIndex;
  }
  changeState(ClockState::WIFI_BACKOFF, now);
}

bool formatClock(char *dateText, size_t dateSize, char *timeText,
                 size_t timeSize) {
  const time_t epoch = time(nullptr);
  if (epoch < VALID_EPOCH) return false;

  struct tm localTime;
  if (localtime_r(&epoch, &localTime) == nullptr) return false;
  strftime(dateText, dateSize, "%Y-%m-%d", &localTime);
  strftime(timeText, timeSize, "%H:%M:%S", &localTime);
  return true;
}

void drawClockScreen(unsigned long now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("NTP CLOCK UTC+8");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  if (clockState == ClockState::CONFIG_ERR) {
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print("CONFIG ERR");
    display.setTextSize(1);
    display.setCursor(0, 39);
    display.print("COPY secrets.h");
    display.setCursor(0, 51);
    display.print("EDIT WIFI VALUES");
    display.display();
    return;
  }

  if (clockState == ClockState::WIFI_CONNECT_START ||
      clockState == ClockState::WIFI_CONNECT_WAIT) {
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print("WIFI");
    display.setTextSize(1);
    display.setCursor(0, 39);
    display.print("CONNECT LEFT ");
    display.print(secondsRemaining(now, WIFI_CONNECT_TIMEOUT_MS));
    display.print("s");
    display.setCursor(0, 51);
    display.print("SSID/PASS HIDDEN");
    display.display();
    return;
  }

  if (clockState == ClockState::NTP_CONFIG ||
      clockState == ClockState::NTP_SYNC_WAIT) {
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print("NTP SYNC");
    display.setTextSize(1);
    display.setCursor(0, 39);
    display.print("LEFT ");
    display.print(secondsRemaining(now, NTP_SYNC_TIMEOUT_MS));
    display.print("s");
    display.setCursor(0, 51);
    display.print("TZ CST-8 = UTC+8");
    display.display();
    return;
  }

  if (clockState == ClockState::NTP_BACKOFF) {
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print("NTP T/O");
    display.setTextSize(1);
    display.setCursor(0, 39);
    display.print("NO TIME");
    display.setCursor(0, 51);
    display.print("RETRY ");
    display.print(secondsRemaining(now, NTP_RETRY_MS));
    display.print("s");
    display.display();
    return;
  }

  char dateText[11] = "NO DATE";
  char timeText[9] = "--:--:--";
  const bool formatted = formatClock(dateText, sizeof(dateText), timeText,
                                     sizeof(timeText));

  if (clockState == ClockState::WIFI_BACKOFF) {
    display.setTextSize(1);
    display.setCursor(0, 14);
    display.print(wifiFailureText(wifiFailure));
    display.setTextSize(2);
    display.setCursor(0, 25);
    display.print(formatted ? timeText : "NO TIME");
    display.setTextSize(1);
    display.setCursor(0, 49);
    display.print("RETRY ");
    display.print(secondsRemaining(now, wifiBackoffDurationMillis));
    display.print("s CLOCK LOCAL");
    display.display();
    return;
  }

  const unsigned long syncAgeMillis = now - lastSyncMillis;
  const bool syncStale = syncAgeMillis >= NTP_STALE_MS;
  display.setTextSize(1);
  display.setCursor(0, 13);
  display.print(syncStale ? "NTP STALE" : "TIME OK");
  display.setTextSize(2);
  display.setCursor(0, 23);
  display.print(formatted ? timeText : "NO TIME");
  display.setTextSize(1);
  display.setCursor(0, 47);
  display.print(formatted ? dateText : "NO DATE");
  display.setCursor(0, 56);
  display.print("SYNC ");
  display.print(cappedSeconds(syncAgeMillis));
  display.print("s NET ON");
  display.display();
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
  stateSinceMillis = millis();

  if (hasPlaceholderSecrets()) {
    changeState(ClockState::CONFIG_ERR, stateSinceMillis);
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    changeState(ClockState::WIFI_CONNECT_START, stateSinceMillis);
  }
  drawClockScreen(stateSinceMillis);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();

  if (timeSyncNotified.exchange(false, std::memory_order_acq_rel)) {
    if (clockIsValid()) {
      hasValidTime = true;
      lastSyncMillis = now;
      if (WiFi.status() == WL_CONNECTED) {
        changeState(ClockState::TIME_OK, now);
      }
    }
  }

  switch (clockState) {
    case ClockState::CONFIG_ERR:
      break;

    case ClockState::WIFI_CONNECT_START:
      WiFi.disconnect(false, false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      changeState(ClockState::WIFI_CONNECT_WAIT, now);
      break;

    case ClockState::WIFI_CONNECT_WAIT: {
      const wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED) {
        wifiBackoffIndex = 0;
        changeState(hasValidTime && clockIsValid() ? ClockState::TIME_OK
                                                  : ClockState::NTP_CONFIG,
                    now);
      } else if (status == WL_NO_SSID_AVAIL) {
        WiFi.disconnect(false, false);
        startWifiBackoff(WifiFailure::NO_AP, now);
      } else if (status == WL_CONNECT_FAILED) {
        WiFi.disconnect(false, false);
        startWifiBackoff(WifiFailure::CONNECT_ERR, now);
      } else if (now - stateSinceMillis >= WIFI_CONNECT_TIMEOUT_MS) {
        WiFi.disconnect(false, false);
        startWifiBackoff(WifiFailure::TIMEOUT, now);
      }
      break;
    }

    case ClockState::WIFI_BACKOFF:
      if (now - stateSinceMillis >= wifiBackoffDurationMillis) {
        changeState(ClockState::WIFI_CONNECT_START, now);
      }
      break;

    case ClockState::NTP_CONFIG:
      timeSyncNotified.store(false, std::memory_order_release);
      sntp_set_time_sync_notification_cb(noteTimeSync);
      configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
      changeState(ClockState::NTP_SYNC_WAIT, now);
      break;

    case ClockState::NTP_SYNC_WAIT:
      if (WiFi.status() != WL_CONNECTED) {
        startWifiBackoff(WifiFailure::LINK_LOST, now);
      } else if (clockIsValid()) {
        hasValidTime = true;
        lastSyncMillis = now;
        changeState(ClockState::TIME_OK, now);
      } else if (now - stateSinceMillis >= NTP_SYNC_TIMEOUT_MS) {
        changeState(ClockState::NTP_BACKOFF, now);
      }
      break;

    case ClockState::NTP_BACKOFF:
      if (WiFi.status() != WL_CONNECTED) {
        startWifiBackoff(WifiFailure::LINK_LOST, now);
      } else if (clockIsValid()) {
        hasValidTime = true;
        lastSyncMillis = now;
        changeState(ClockState::TIME_OK, now);
      } else if (now - stateSinceMillis >= NTP_RETRY_MS) {
        changeState(ClockState::NTP_CONFIG, now);
      }
      break;

    case ClockState::TIME_OK:
      if (!clockIsValid()) {
        hasValidTime = false;
        changeState(ClockState::NTP_CONFIG, now);
      } else if (WiFi.status() != WL_CONNECTED) {
        startWifiBackoff(WifiFailure::LINK_LOST, now);
      }
      break;
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;
  drawClockScreen(now);
}
