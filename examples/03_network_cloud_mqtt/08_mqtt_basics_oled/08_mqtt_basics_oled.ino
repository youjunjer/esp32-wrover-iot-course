#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MQTT.h>
#include <esp_system.h>
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
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long MQTT_RETRY_MS = 10000UL;
constexpr unsigned long MQTT_JITTER_MS = 5000UL;
constexpr unsigned long MQTT_RETRY_MAX_MS = 5UL * 60UL * 1000UL;
constexpr unsigned long STATUS_INTERVAL_MS = 60000UL;
constexpr char TIME_ZONE[] = "CST-8";

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
NetworkClientSecure tlsClient;
MQTTClient mqttClient(512);
bool oledReady = false;
bool wifiAttemptActive = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool mqttConfigured = false;
unsigned long wifiStartedAt = 0;
unsigned long nextWifiAt = 0;
unsigned long ntpStartedAt = 0;
unsigned long nextNtpAt = 0;
unsigned long nextMqttAt = 0;
unsigned long nextStatusAt = 0;
unsigned long mqttBackoffMs = MQTT_RETRY_MS;
unsigned long statusSequence = 0;
char statusTopic[96] = {};

bool due(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

unsigned long secondsUntil(unsigned long now, unsigned long target) {
  return due(now, target) ? 0 : (target - now + 999UL) / 1000UL;
}

bool placeholder(const char *value) {
  return value == nullptr || value[0] == '\0' || strstr(value, "REPLACE_WITH_") != nullptr;
}

bool validClientId(const char *value) {
  const size_t length = value == nullptr ? 0 : strlen(value);
  if (length == 0 || length > 23) return false;
  for (size_t index = 0; index < length; ++index) {
    const char c = value[index];
    if (!isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
  }
  return true;
}

bool validTopicRoot(const char *value) {
  const size_t length = value == nullptr ? 0 : strlen(value);
  return length >= 8 && length <= 72 && value[0] != '/' &&
         value[length - 1] != '/' && strchr(value, '#') == nullptr &&
         strchr(value, '+') == nullptr;
}

bool configurationReady() {
  return !USING_EXAMPLE_SECRETS && !placeholder(WIFI_SSID) &&
         !placeholder(WIFI_PASSWORD) && !placeholder(MQTT_HOST) &&
         !placeholder(MQTT_USERNAME) && !placeholder(MQTT_PASSWORD) &&
         !placeholder(MQTT_ROOT_CA) && !placeholder(MQTT_TOPIC_ROOT) &&
         !placeholder(MQTT_CLIENT_ID) && MQTT_PORT == 8883 &&
         strstr(MQTT_HOST, "://") == nullptr && strchr(MQTT_HOST, '/') == nullptr &&
         strstr(MQTT_ROOT_CA, "-----BEGIN CERTIFICATE-----") != nullptr &&
         validClientId(MQTT_CLIENT_ID) && validTopicRoot(MQTT_TOPIC_ROOT);
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
  display.print("MQTT BASICS TLS");
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

void disconnectTransport() {
  // Close the transport without a normal MQTT DISCONNECT so the Broker can
  // publish this session's Last Will after an unexpected failure.
  tlsClient.stop();
  nextMqttAt = millis() + mqttBackoffMs + esp_random() % MQTT_JITTER_MS;
  mqttBackoffMs = min(mqttBackoffMs * 2UL, MQTT_RETRY_MAX_MS);
}

bool serviceWifi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) {
    wifiAttemptActive = false;
    return true;
  }
  tlsClient.stop();
  clockReady = false;
  ntpAttemptActive = false;
  if (wifiAttemptActive) {
    const unsigned long elapsed = now - wifiStartedAt;
    if (elapsed >= WIFI_TIMEOUT_MS) {
      WiFi.disconnect(false, false);
      wifiAttemptActive = false;
      nextWifiAt = now + WIFI_RETRY_MS;
      drawStatus("WIFI ERR", "MQTT OFFLINE", "RETRY 10s");
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
    snprintf(footer, sizeof(footer), "RETRY %lus",
             secondsUntil(now, nextWifiAt));
    drawStatus("WIFI OFF", "MQTT OFFLINE", footer);
  }
  return false;
}

bool serviceClock(unsigned long now) {
  if (clockReady) return true;
  if (!ntpAttemptActive && !due(now, nextNtpAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus",
             secondsUntil(now, nextNtpAt));
    drawStatus("TIMEOUT", "TLS BLOCKED", footer);
    return false;
  }
  if (!ntpAttemptActive && due(now, nextNtpAt)) {
    configTzTime(TIME_ZONE, "pool.ntp.org", "time.nist.gov");
    ntpStartedAt = now;
    ntpAttemptActive = true;
    drawStatus("TIME WAIT", "TLS BLOCKED", "TIMEOUT 12s");
  }
  if (ntpAttemptActive) {
    if (time(nullptr) > 1700000000) {
      clockReady = true;
      ntpAttemptActive = false;
      nextMqttAt = now;
      drawStatus("TIME OK", "TLS READY", "MQTT NEXT");
      return true;
    }
    if (now - ntpStartedAt >= NTP_TIMEOUT_MS) {
      ntpAttemptActive = false;
      nextNtpAt = now + NTP_RETRY_MS;
      drawStatus("TIMEOUT", "TLS BLOCKED", "RETRY 30s");
    }
  }
  return false;
}

bool connectMqtt(unsigned long now) {
  if (!due(now, nextMqttAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus", secondsUntil(now, nextMqttAt));
    drawStatus("MQTT OFF", "BROKER HIDDEN", footer);
    return false;
  }
  drawStatus("TLS CONNECT", "VERIFY BROKER", "PORT 8883");
  tlsClient.stop();
  tlsClient.setCACert(MQTT_ROOT_CA);
  tlsClient.setHandshakeTimeout(8);
  tlsClient.setConnectionTimeout(5000);
  tlsClient.setTimeout(5000);
  if (!tlsClient.connect(MQTT_HOST, MQTT_PORT)) {
    drawStatus("TLS ERR", "CA/TIME/NETWORK", "RETRY LATER");
    disconnectTransport();
    return false;
  }

  drawStatus("MQTT CONNECT", "CLIENT UNIQUE", "AUTH HIDDEN");
  if (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, true)) {
    char detail[22];
    snprintf(detail, sizeof(detail), "ERR %d RC %d",
             static_cast<int>(mqttClient.lastError()),
             static_cast<int>(mqttClient.returnCode()));
    drawStatus("MQTT ERR", detail, "CHECK AUTH/ID");
    disconnectTransport();
    return false;
  }
  if (!mqttClient.publish(statusTopic,
                          "{\"online\":true,\"lesson\":8}", true, 1)) {
    drawStatus("PUB ERR", "STATUS NOT SENT", "DISCONNECT");
    disconnectTransport();
    return false;
  }
  statusSequence = 1;
  mqttBackoffMs = MQTT_RETRY_MS;
  nextStatusAt = now + STATUS_INTERVAL_MS;
  drawStatus("MQTT ONLINE", "QOS1 STATUS", "LWT + RETAIN");
  return true;
}

void publishStatus(unsigned long now) {
  char payload[96];
  snprintf(payload, sizeof(payload),
           "{\"online\":true,\"lesson\":8,\"seq\":%lu}", ++statusSequence);
  if (mqttClient.publish(statusTopic, payload, true, 1)) {
    drawStatus("PUB OK", "STATUS RETAINED", "QOS 1");
  } else {
    drawStatus("PUB ERR", "STATUS FAILED", "RECONNECT");
    disconnectTransport();
  }
  nextStatusAt = now + STATUS_INTERVAL_MS;
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
  if (!configurationReady()) {
    drawStatus("CONFIG ERR", "COPY secrets.h", "TLS REQUIRED");
    return;
  }
  snprintf(statusTopic, sizeof(statusTopic), "%s/status", MQTT_TOPIC_ROOT);
  tlsClient.setCACert(MQTT_ROOT_CA);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, tlsClient);
  mqttClient.setOptions(30, true, 3000);
  mqttClient.setWill(statusTopic, "{\"online\":false}", true, 1);
  mqttConfigured = true;
  nextWifiAt = millis();
}

void loop() {
  if (!oledReady) {
    blinkError(findOledAddress() == 0 ? 2 : 3);
    return;
  }
  if (!configurationReady() || !mqttConfigured) return;
  const unsigned long now = millis();
  if (!serviceWifi(now) || !serviceClock(now)) return;
  if (!mqttClient.connected()) {
    connectMqtt(now);
    return;
  }
  if (!mqttClient.loop()) {
    drawStatus("LINK LOST", "MQTT LOOP ERR", "RETRY LATER");
    disconnectTransport();
    return;
  }
  if (due(now, nextStatusAt)) {
    publishStatus(now);
  } else if (now % 2000UL < 30UL) {
    char footer[22];
    snprintf(footer, sizeof(footer), "NEXT %lus SEQ %lu",
             secondsUntil(now, nextStatusAt), statusSequence);
    drawStatus("READY", "TLS MQTT 3.1.1", footer);
  }
}
