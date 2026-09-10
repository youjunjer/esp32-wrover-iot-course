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
constexpr unsigned long PUBLISH_INTERVAL_MS = 30000UL;
constexpr size_t MAX_COMMAND_BYTES = 48;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
NetworkClientSecure tlsClient;
MQTTClient mqttClient(512);
bool oledReady = false;
bool wifiAttemptActive = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool mqttConfigured = false;
bool commandPending = false;
bool commandOversize = false;
unsigned long wifiStartedAt = 0;
unsigned long nextWifiAt = 0;
unsigned long ntpStartedAt = 0;
unsigned long nextNtpAt = 0;
unsigned long nextMqttAt = 0;
unsigned long nextPublishAt = 0;
unsigned long publishSequence = 0;
unsigned long mqttBackoffMs = MQTT_RETRY_MS;
char statusTopic[96] = {};
char dataTopic[96] = {};
char commandTopic[96] = {};
char ackTopic[96] = {};
char pendingCommand[MAX_COMMAND_BYTES + 1] = {};

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
  display.print("MQTT PUB / SUB");
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

void messageReceived(String &topic, String &payload) {
  if (!topic.equals(commandTopic)) return;
  commandPending = true;
  commandOversize = payload.length() > MAX_COMMAND_BYTES;
  if (commandOversize) {
    pendingCommand[0] = '\0';
    return;
  }
  payload.toCharArray(pendingCommand, sizeof(pendingCommand));
}

void scheduleMqttRetry(unsigned long now) {
  // An unexpected failure closes TLS without MQTT DISCONNECT so Last Will is
  // not suppressed.
  tlsClient.stop();
  nextMqttAt = now + mqttBackoffMs + esp_random() % MQTT_JITTER_MS;
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
    configTzTime("CST-8", "pool.ntp.org", "time.nist.gov");
    ntpStartedAt = now;
    ntpAttemptActive = true;
    drawStatus("TIME WAIT", "TLS BLOCKED", "TIMEOUT 12s");
  }
  if (ntpAttemptActive) {
    if (time(nullptr) > 1700000000) {
      clockReady = true;
      ntpAttemptActive = false;
      nextMqttAt = now;
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
    scheduleMqttRetry(now);
    return false;
  }
  if (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, true)) {
    char detail[22];
    snprintf(detail, sizeof(detail), "ERR %d RC %d",
             static_cast<int>(mqttClient.lastError()),
             static_cast<int>(mqttClient.returnCode()));
    drawStatus("MQTT ERR", detail, "CHECK AUTH/ID");
    scheduleMqttRetry(now);
    return false;
  }
  if (!mqttClient.publish(statusTopic, "{\"online\":true}", true, 1) ||
      !mqttClient.subscribe(commandTopic, 1)) {
    drawStatus("SUB ERR", "STATUS OR CMD", "RECONNECT");
    scheduleMqttRetry(now);
    return false;
  }
  nextPublishAt = now + 3000UL;
  mqttBackoffMs = MQTT_RETRY_MS;
  drawStatus("SUB OK", "EXACT CMD TOPIC", "QOS 1");
  return true;
}

void processCommand() {
  commandPending = false;
  const bool ping = !commandOversize && strcmp(pendingCommand, "PING") == 0;
  const char *reply = ping ? "PONG" : "REJECT";
  if (mqttClient.publish(ackTopic, reply, false, 1)) {
    drawStatus(ping ? "CMD OK" : "CMD REJECT",
               commandOversize ? "PAYLOAD TOO LARGE" : reply,
               "ACK QOS 1");
  } else {
    drawStatus("PUB ERR", "ACK NOT SENT", "RECONNECT");
    scheduleMqttRetry(millis());
  }
  commandOversize = false;
}

void publishHeartbeat(unsigned long now) {
  char payload[64];
  snprintf(payload, sizeof(payload), "heartbeat %lu", ++publishSequence);
  if (mqttClient.publish(dataTopic, payload, false, 1)) {
    char footer[22];
    snprintf(footer, sizeof(footer), "SEQ %lu QOS 1", publishSequence);
    drawStatus("PUB OK", "NO RETAIN", footer);
  } else {
    drawStatus("PUB ERR", "DATA NOT SENT", "RECONNECT");
    scheduleMqttRetry(now);
  }
  nextPublishAt = now + PUBLISH_INTERVAL_MS;
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
  snprintf(dataTopic, sizeof(dataTopic), "%s/data", MQTT_TOPIC_ROOT);
  snprintf(commandTopic, sizeof(commandTopic), "%s/cmd", MQTT_TOPIC_ROOT);
  snprintf(ackTopic, sizeof(ackTopic), "%s/ack", MQTT_TOPIC_ROOT);
  tlsClient.setCACert(MQTT_ROOT_CA);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, tlsClient);
  mqttClient.setOptions(30, true, 3000);
  mqttClient.setWill(statusTopic, "{\"online\":false}", true, 1);
  mqttClient.onMessage(messageReceived);
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
    scheduleMqttRetry(now);
    return;
  }
  if (commandPending) processCommand();
  if (mqttClient.connected() && due(now, nextPublishAt)) publishHeartbeat(now);
}
