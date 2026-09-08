#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
constexpr unsigned long WIFI_SCAN_TIMEOUT_MS = 12000UL;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_STABLE_RESET_MS = 30000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;
constexpr unsigned long WIFI_BACKOFF_MS[] = {
    3000UL, 6000UL, 12000UL, 30000UL};

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

enum class WifiState {
  CONFIG_ERR,
  SCAN_START,
  SCAN_WAIT,
  CONNECT_START,
  CONNECT_WAIT,
  ONLINE,
  BACKOFF
};

enum class WifiFailure {
  NONE,
  SCAN_ERR,
  SCAN_TIMEOUT,
  NO_AP,
  CONNECT_ERR,
  CONNECT_TIMEOUT,
  LINK_LOST
};

uint8_t errorCode = 0;
bool oledReady = false;
WifiState wifiState = WifiState::CONFIG_ERR;
WifiState stateAfterBackoff = WifiState::CONNECT_START;
WifiFailure wifiFailure = WifiFailure::NONE;
unsigned long stateSinceMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long onlineSinceMillis = 0;
unsigned long backoffDurationMillis = WIFI_BACKOFF_MS[0];
uint8_t backoffIndex = 0;
int16_t scannedNetworkCount = 0;
int32_t targetRssi = 0;

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

void changeState(WifiState nextState, unsigned long now) {
  wifiState = nextState;
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

const char *failureText(WifiFailure failure) {
  switch (failure) {
    case WifiFailure::SCAN_ERR:
      return "SCAN ERR";
    case WifiFailure::SCAN_TIMEOUT:
      return "SCAN T/O";
    case WifiFailure::NO_AP:
      return "NO AP";
    case WifiFailure::CONNECT_ERR:
      return "CONN ERR";
    case WifiFailure::CONNECT_TIMEOUT:
      return "TIMEOUT";
    case WifiFailure::LINK_LOST:
      return "LINK LOST";
    default:
      return "WIFI ERR";
  }
}

void startBackoff(WifiFailure failure, WifiState nextState,
                  unsigned long now) {
  wifiFailure = failure;
  stateAfterBackoff = nextState;
  backoffDurationMillis = WIFI_BACKOFF_MS[backoffIndex];
  if (backoffIndex + 1 <
      sizeof(WIFI_BACKOFF_MS) / sizeof(WIFI_BACKOFF_MS[0])) {
    ++backoffIndex;
  }
  changeState(WifiState::BACKOFF, now);
}

void finishScan(int16_t networkCount, unsigned long now) {
  scannedNetworkCount = networkCount < 0 ? 0 : networkCount;
  bool targetFound = false;
  targetRssi = 0;

  for (int16_t index = 0; index < networkCount; ++index) {
    if (WiFi.SSID(index) == WIFI_SSID) {
      targetFound = true;
      targetRssi = WiFi.RSSI(index);
      break;
    }
  }

  WiFi.scanDelete();
  if (targetFound) {
    changeState(WifiState::CONNECT_START, now);
  } else {
    startBackoff(WifiFailure::NO_AP, WifiState::SCAN_START, now);
  }
}

void drawWifiScreen(unsigned long now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("WIFI OLED");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 14);

  switch (wifiState) {
    case WifiState::CONFIG_ERR:
      display.print("CONFIG ERR");
      display.setTextSize(1);
      display.setCursor(0, 39);
      display.print("COPY secrets.h");
      display.setCursor(0, 51);
      display.print("EDIT WIFI VALUES");
      break;

    case WifiState::SCAN_START:
    case WifiState::SCAN_WAIT:
      display.print("SCAN");
      display.setTextSize(1);
      display.setCursor(0, 39);
      display.print("SEARCH AP ");
      display.print(cappedSeconds(now - stateSinceMillis));
      display.print("s");
      display.setCursor(0, 51);
      display.print("SSID HIDDEN ON OLED");
      break;

    case WifiState::CONNECT_START:
    case WifiState::CONNECT_WAIT:
      display.print("CONNECT");
      display.setTextSize(1);
      display.setCursor(0, 39);
      display.print("AP ");
      display.print(scannedNetworkCount);
      display.print(" TARGET YES");
      display.setCursor(0, 51);
      display.print("LEFT ");
      display.print(secondsRemaining(now, WIFI_CONNECT_TIMEOUT_MS));
      display.print("s RSSI ");
      display.print(targetRssi);
      break;

    case WifiState::ONLINE:
      display.print("ONLINE");
      display.setTextSize(1);
      display.setCursor(0, 39);
      display.print("IP ASSIGNED");
      display.setCursor(0, 51);
      display.print("RSSI ");
      display.print(WiFi.RSSI());
      display.print(" UP ");
      display.print(cappedSeconds(now - onlineSinceMillis));
      display.print("s");
      break;

    case WifiState::BACKOFF:
      display.print(failureText(wifiFailure));
      display.setTextSize(1);
      display.setCursor(0, 39);
      display.print("RETRY ");
      display.print(secondsRemaining(now, backoffDurationMillis));
      display.print("s");
      display.setCursor(0, 51);
      display.print("SSID/PASS NOT SHOWN");
      break;
  }

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
    changeState(WifiState::CONFIG_ERR, stateSinceMillis);
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    changeState(WifiState::SCAN_START, stateSinceMillis);
  }
  drawWifiScreen(stateSinceMillis);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();

  switch (wifiState) {
    case WifiState::CONFIG_ERR:
      break;

    case WifiState::SCAN_START: {
      WiFi.scanDelete();
      const int16_t result = WiFi.scanNetworks(true);
      if (result == WIFI_SCAN_FAILED) {
        startBackoff(WifiFailure::SCAN_ERR, WifiState::SCAN_START, now);
      } else if (result == WIFI_SCAN_RUNNING) {
        changeState(WifiState::SCAN_WAIT, now);
      } else {
        finishScan(result, now);
      }
      break;
    }

    case WifiState::SCAN_WAIT: {
      const int16_t result = WiFi.scanComplete();
      if (result == WIFI_SCAN_RUNNING) {
        if (now - stateSinceMillis >= WIFI_SCAN_TIMEOUT_MS) {
          WiFi.scanDelete();
          startBackoff(WifiFailure::SCAN_TIMEOUT, WifiState::SCAN_START,
                       now);
        }
      } else if (result == WIFI_SCAN_FAILED) {
        startBackoff(WifiFailure::SCAN_ERR, WifiState::SCAN_START, now);
      } else {
        finishScan(result, now);
      }
      break;
    }

    case WifiState::CONNECT_START:
      WiFi.disconnect(false, false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      changeState(WifiState::CONNECT_WAIT, now);
      break;

    case WifiState::CONNECT_WAIT: {
      const wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED) {
        onlineSinceMillis = now;
        changeState(WifiState::ONLINE, now);
      } else if (status == WL_NO_SSID_AVAIL) {
        WiFi.disconnect(false, false);
        startBackoff(WifiFailure::NO_AP, WifiState::SCAN_START, now);
      } else if (status == WL_CONNECT_FAILED) {
        WiFi.disconnect(false, false);
        startBackoff(WifiFailure::CONNECT_ERR, WifiState::CONNECT_START,
                     now);
      } else if (now - stateSinceMillis >= WIFI_CONNECT_TIMEOUT_MS) {
        WiFi.disconnect(false, false);
        startBackoff(WifiFailure::CONNECT_TIMEOUT,
                     WifiState::CONNECT_START, now);
      }
      break;
    }

    case WifiState::ONLINE:
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(false, false);
        startBackoff(WifiFailure::LINK_LOST, WifiState::CONNECT_START, now);
      } else if (now - onlineSinceMillis >= WIFI_STABLE_RESET_MS) {
        backoffIndex = 0;
      }
      break;

    case WifiState::BACKOFF:
      if (now - stateSinceMillis >= backoffDurationMillis) {
        changeState(stateAfterBackoff, now);
      }
      break;
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;
  drawWifiScreen(now);
}
