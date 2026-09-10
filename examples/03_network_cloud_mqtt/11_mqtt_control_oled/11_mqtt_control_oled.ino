#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MQTT.h>
#include <esp_system.h>
#include <sys/time.h>
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
constexpr int RED_LED_PIN = 4;
constexpr int YELLOW_LED_PIN = 2;
constexpr int GREEN_LED_PIN = 15;
constexpr int RELAY_PIN = 18;
constexpr int SERVO_PIN = 5;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long MQTT_RETRY_MS = 10000UL;
constexpr unsigned long MQTT_JITTER_MS = 5000UL;
constexpr unsigned long MQTT_RETRY_MAX_MS = 5UL * 60UL * 1000UL;
constexpr size_t MAX_COMMAND_BYTES = 320;
constexpr size_t MAX_ACK_BYTES = 256;
constexpr unsigned long OUTPUT_LEASE_MAX_MS = 30000UL;
constexpr unsigned long MIN_REMAINING_LEASE_MS = 2000UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
NetworkClientSecure tlsClient;
MQTTClient mqttClient(640);
Servo servo;
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
unsigned long outputLeaseUntil = 0;
unsigned long mqttBackoffMs = MQTT_RETRY_MS;
char statusTopic[96] = {};
char commandTopic[96] = {};
char ackTopic[96] = {};
char pendingCommand[MAX_COMMAND_BYTES + 1] = {};
char bootId[9] = {};
unsigned long lastCommandSequence = 0;

bool due(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

unsigned long secondsUntil(unsigned long now, unsigned long target) {
  return due(now, target) ? 0 : (target - now + 999UL) / 1000UL;
}

bool placeholder(const char *value) {
  return value == nullptr || value[0] == '\0' || strstr(value, "REPLACE_WITH_") != nullptr;
}

bool validSimpleId(const char *value, size_t maximum) {
  const size_t length = value == nullptr ? 0 : strlen(value);
  if (length == 0 || length > maximum) return false;
  for (size_t index = 0; index < length; ++index) {
    const char c = value[index];
    if (!isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') return false;
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
         validSimpleId(MQTT_CLIENT_ID, 23) && validTopicRoot(MQTT_TOPIC_ROOT);
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
  display.print("MQTT SAFE CONTROL");
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

void safeOutputs() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);  // Active-low relay: HIGH is OFF.
  if (servo.attached()) {
    servo.detach();
  }
  outputLeaseUntil = 0;
}

void initializeSafeOutputs() {
  // Set output latches before changing pin mode so the active-low relay never
  // receives an intentional LOW pulse during setup.
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  safeOutputs();
}

void blinkError(uint8_t pulses) {
  for (uint8_t index = 0; index < pulses; ++index) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(160);
    digitalWrite(YELLOW_LED_PIN, LOW);
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
  safeOutputs();
  // Unexpected failures close TLS without MQTT DISCONNECT so Last Will is not
  // suppressed. The next connect() replaces the library's internal state.
  tlsClient.stop();
  nextMqttAt = now + mqttBackoffMs + esp_random() % MQTT_JITTER_MS;
  mqttBackoffMs = min(mqttBackoffMs * 2UL, MQTT_RETRY_MAX_MS);
}

bool serviceWifi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) {
    wifiAttemptActive = false;
    return true;
  }
  safeOutputs();
  tlsClient.stop();
  clockReady = false;
  ntpAttemptActive = false;
  if (wifiAttemptActive) {
    const unsigned long elapsed = now - wifiStartedAt;
    if (elapsed >= WIFI_TIMEOUT_MS) {
      WiFi.disconnect(false, false);
      wifiAttemptActive = false;
      nextWifiAt = now + WIFI_RETRY_MS;
      drawStatus("WIFI ERR", "ALL OUTPUTS OFF", "RETRY 10s");
    } else {
      char footer[20];
      snprintf(footer, sizeof(footer), "LEFT %lus SAFE OFF",
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
    drawStatus("WIFI WAIT", "ALL OUTPUTS OFF", "TIMEOUT 15s");
  } else {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus OFF",
             secondsUntil(now, nextWifiAt));
    drawStatus("WIFI OFF", "ALL OUTPUTS OFF", footer);
  }
  return false;
}

bool serviceClock(unsigned long now) {
  if (clockReady) return true;
  safeOutputs();
  if (!ntpAttemptActive && !due(now, nextNtpAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus OFF",
             secondsUntil(now, nextNtpAt));
    drawStatus("TIMEOUT", "CONTROL BLOCKED", footer);
    return false;
  }
  if (!ntpAttemptActive && due(now, nextNtpAt)) {
    configTzTime("CST-8", "pool.ntp.org", "time.nist.gov");
    ntpStartedAt = now;
    ntpAttemptActive = true;
    drawStatus("TIME WAIT", "CONTROL BLOCKED", "SAFE OFF");
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
      drawStatus("TIMEOUT", "CONTROL BLOCKED", "RETRY 30s OFF");
    }
  }
  return false;
}

bool publishStatus(bool online) {
  JsonDocument status;
  status["online"] = online;
  status["boot_id"] = bootId;
  char payload[160];
  const size_t written = serializeJson(status, payload, sizeof(payload));
  return written > 0 && mqttClient.publish(statusTopic, payload, true, 1);
}

bool connectMqtt(unsigned long now) {
  if (!due(now, nextMqttAt)) {
    char footer[20];
    snprintf(footer, sizeof(footer), "RETRY %lus SAFE OFF",
             secondsUntil(now, nextMqttAt));
    drawStatus("MQTT OFF", "ALL OUTPUTS OFF", footer);
    return false;
  }
  safeOutputs();
  drawStatus("TLS CONNECT", "CONTROL BLOCKED", "PORT 8883");
  tlsClient.stop();
  tlsClient.setCACert(MQTT_ROOT_CA);
  tlsClient.setHandshakeTimeout(8);
  tlsClient.setConnectionTimeout(5000);
  tlsClient.setTimeout(5000);
  if (!tlsClient.connect(MQTT_HOST, MQTT_PORT)) {
    drawStatus("TLS ERR", "CA/TIME/NETWORK", "SAFE OFF");
    scheduleMqttRetry(now);
    return false;
  }
  if (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, true)) {
    char detail[22];
    snprintf(detail, sizeof(detail), "ERR %d RC %d",
             static_cast<int>(mqttClient.lastError()),
             static_cast<int>(mqttClient.returnCode()));
    drawStatus("MQTT ERR", detail, "SAFE OFF");
    scheduleMqttRetry(now);
    return false;
  }

  // MQTT 3.1.1 clears a retained message by publishing a zero-byte retained
  // payload. Clear this device's exact command topic before subscribing.
  if (!mqttClient.publish(commandTopic, "", true, 1)) {
    drawStatus("RETAIN ERR", "CLEAR FAILED", "SAFE OFF");
    scheduleMqttRetry(now);
    return false;
  }
  drawStatus("RETAIN CLEAR", "CMD TOPIC EMPTY", "SAFE OFF");
  if (!mqttClient.subscribe(commandTopic, 1) || !publishStatus(true)) {
    drawStatus("SUB ERR", "CMD/STATUS FAILED", "SAFE OFF");
    scheduleMqttRetry(now);
    return false;
  }
  commandPending = false;
  mqttBackoffMs = MQTT_RETRY_MS;
  drawStatus("CONTROL READY", "BOOT ID PUBLISHED", "WAIT COMMAND");
  return true;
}

bool publishAck(const char *commandId, unsigned long commandSequence, bool ok,
                const char *reason) {
  JsonDocument ack;
  ack["v"] = 1;
  ack["cmd_id"] = commandId;
  ack["cmd_seq"] = commandSequence;
  ack["ok"] = ok;
  ack["reason"] = reason;
  ack["boot_id"] = bootId;
  ack["ts"] = time(nullptr);
  char payload[MAX_ACK_BYTES + 1];
  const size_t length = serializeJson(ack, payload, sizeof(payload));
  if (length == 0 || !mqttClient.publish(ackTopic, payload, false, 1)) {
    safeOutputs();
    drawStatus("ACK ERR", reason, "SAFE OFF");
    return false;
  }
  return true;
}

bool applyTraffic(const char *value) {
  if (strcmp(value, "off") == 0) return true;
  if (strcmp(value, "red") == 0) {
    digitalWrite(RED_LED_PIN, HIGH);
    return true;
  }
  if (strcmp(value, "yellow") == 0) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
    return true;
  }
  if (strcmp(value, "green") == 0) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    return true;
  }
  return false;
}

void rejectCommand(const char *commandId, unsigned long commandSequence,
                   const char *reason) {
  safeOutputs();
  drawStatus("CMD REJECT", reason, "ALL OUTPUTS OFF");
  publishAck(commandId, commandSequence, false, reason);
}

void processCommand() {
  commandPending = false;
  if (commandOversize) {
    commandOversize = false;
    rejectCommand("unknown", 0, "TOO LARGE");
    return;
  }
  JsonDocument command;
  const DeserializationError error = deserializeJson(
      command, pendingCommand, DeserializationOption::NestingLimit(2));
  if (error || command.size() != 8) {
    rejectCommand("unknown", 0, "JSON/SCHEMA ERR");
    return;
  }

  const char *commandId = command["cmd_id"] | "";
  const unsigned long commandSequence = command["cmd_seq"] | 0UL;
  const char *requestedBootId = command["boot_id"] | "";
  const char *target = command["target"] | "";
  if (!command["v"].is<int>() || command["v"].as<int>() != 1 ||
      !validSimpleId(commandId, 24) || !command["boot_id"].is<const char *>() ||
      !command["cmd_seq"].is<unsigned long>() || commandSequence == 0 ||
      !command["ts"].is<int64_t>() || !command["ttl"].is<int>() ||
      !command["target"].is<const char *>() || command["value"].isNull()) {
    rejectCommand(validSimpleId(commandId, 24) ? commandId : "unknown",
                  commandSequence, "SCHEMA ERR");
    return;
  }
  if (strcmp(requestedBootId, bootId) != 0) {
    rejectCommand(commandId, commandSequence, "BOOT ID ERR");
    return;
  }
  if (commandSequence <= lastCommandSequence) {
    rejectCommand(commandId, commandSequence, "REPLAY/DUPLICATE");
    return;
  }
  const int64_t commandEpoch = command["ts"].as<int64_t>();
  const int ttl = command["ttl"].as<int>();
  struct timeval wallNow = {};
  if (gettimeofday(&wallNow, nullptr) != 0) {
    rejectCommand(commandId, commandSequence, "TIME ERR");
    return;
  }
  const int64_t nowEpoch = static_cast<int64_t>(wallNow.tv_sec);
  if (ttl < 5 || ttl > 30 || commandEpoch < 1700000000LL ||
      commandEpoch > nowEpoch + 2) {
    rejectCommand(commandId, commandSequence, "EXPIRED");
    return;
  }
  const int64_t nowEpochMs = nowEpoch * 1000LL + wallNow.tv_usec / 1000LL;
  const int64_t expiresAtMs = (commandEpoch + ttl) * 1000LL;
  if (expiresAtMs <= nowEpochMs) {
    rejectCommand(commandId, commandSequence, "EXPIRED");
    return;
  }
  const int64_t remainingLeaseMs64 = expiresAtMs - nowEpochMs;
  if (remainingLeaseMs64 < static_cast<int64_t>(MIN_REMAINING_LEASE_MS)) {
    rejectCommand(commandId, commandSequence, "TTL TOO SHORT");
    return;
  }
  const unsigned long leaseDurationMs = min(
      static_cast<unsigned long>(remainingLeaseMs64), OUTPUT_LEASE_MAX_MS);
  const unsigned long leaseDeadline = millis() + leaseDurationMs;

  safeOutputs();
  bool applied = false;
  char result[22] = {};
  if (strcmp(target, "traffic") == 0 && command["value"].is<const char *>()) {
    const char *value = command["value"].as<const char *>();
    applied = applyTraffic(value);
    snprintf(result, sizeof(result), "TRAFFIC %s", applied ? value : "ERR");
  } else if (strcmp(target, "relay") == 0 && command["value"].is<const char *>()) {
    const char *value = command["value"].as<const char *>();
    if (strcmp(value, "on") == 0 || strcmp(value, "off") == 0) {
      digitalWrite(RELAY_PIN, strcmp(value, "on") == 0 ? LOW : HIGH);
      applied = true;
      snprintf(result, sizeof(result), "RELAY %s", value);
    }
  } else if (strcmp(target, "servo") == 0 && command["value"].is<int>()) {
    const int angle = command["value"].as<int>();
    if (angle >= 0 && angle <= 180) {
      servo.setPeriodHertz(50);
      servo.attach(SERVO_PIN, 500, 2400);
      if (servo.attached()) {
        servo.write(angle);
        applied = true;
        snprintf(result, sizeof(result), "SERVO %d DEG", angle);
      }
    }
  }
  if (!applied) {
    rejectCommand(commandId, commandSequence, "TARGET/VALUE ERR");
    return;
  }

  lastCommandSequence = commandSequence;
  bool requestsOff = false;
  if (command["value"].is<const char *>()) {
    requestsOff = strcmp(command["value"].as<const char *>(), "off") == 0;
  }
  if (!requestsOff) {
    outputLeaseUntil = leaseDeadline;
  }
  drawStatus("CMD OK", result,
             digitalRead(RELAY_PIN) == LOW ? "RELAY ON NO LOAD" : "ACK QOS 1");
  if (!publishAck(commandId, commandSequence, true, "applied")) {
    return;
  }
  if (outputLeaseUntil != 0 && due(millis(), outputLeaseUntil)) {
    safeOutputs();
    drawStatus("LEASE EXPIRED", "ALL OUTPUTS OFF", "NEW CMD REQUIRED");
  }
}

void setup() {
  initializeSafeOutputs();
  Wire.begin(OLED_SDA, OLED_SCL);
  const uint8_t address = findOledAddress();
  if (address == 0) return;
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, address);
  if (!oledReady) return;
  display.setRotation(2);
  drawStatus("SAFE OFF", "OLED READY", "CHECK CONFIG");
  if (!configurationReady()) {
    drawStatus("CONFIG ERR", "COPY secrets.h", "ALL OUTPUTS OFF");
    return;
  }
  snprintf(bootId, sizeof(bootId), "%08lx",
           static_cast<unsigned long>(esp_random()));
  snprintf(statusTopic, sizeof(statusTopic), "%s/status", MQTT_TOPIC_ROOT);
  snprintf(commandTopic, sizeof(commandTopic), "%s/cmd", MQTT_TOPIC_ROOT);
  snprintf(ackTopic, sizeof(ackTopic), "%s/ack", MQTT_TOPIC_ROOT);
  tlsClient.setCACert(MQTT_ROOT_CA);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, tlsClient);
  mqttClient.setOptions(30, true, 1000);
  mqttClient.setWill(statusTopic, "{\"online\":false}", true, 1);
  mqttClient.onMessage(messageReceived);
  mqttConfigured = true;
  nextWifiAt = millis();
}

void loop() {
  if (!oledReady) {
    safeOutputs();
    blinkError(findOledAddress() == 0 ? 2 : 3);
    return;
  }
  if (!configurationReady() || !mqttConfigured) {
    safeOutputs();
    return;
  }
  const unsigned long now = millis();
  if (!serviceWifi(now) || !serviceClock(now)) return;
  if (!mqttClient.connected()) {
    connectMqtt(now);
    return;
  }
  if (!mqttClient.loop()) {
    drawStatus("LINK LOST", "ALL OUTPUTS OFF", "RETRY LATER");
    scheduleMqttRetry(now);
    return;
  }
  if (commandPending) processCommand();
  if (outputLeaseUntil != 0 && due(millis(), outputLeaseUntil)) {
    safeOutputs();
    drawStatus("LEASE EXPIRED", "ALL OUTPUTS OFF", "NEW CMD REQUIRED");
  }
}
