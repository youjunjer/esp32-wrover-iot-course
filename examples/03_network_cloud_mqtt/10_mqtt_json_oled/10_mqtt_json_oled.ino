#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MQTT.h>
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

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int DHT_PIN = 14;
constexpr int LIGHT_PIN = 33;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long NTP_TIMEOUT_MS = 12000UL;
constexpr unsigned long NTP_RETRY_MS = 30000UL;
constexpr unsigned long MQTT_RETRY_MS = 10000UL;
constexpr unsigned long MQTT_JITTER_MS = 5000UL;
constexpr unsigned long MQTT_RETRY_MAX_MS = 5UL * 60UL * 1000UL;
constexpr unsigned long SENSOR_INTERVAL_MS = 2500UL;
constexpr unsigned long PUBLISH_INTERVAL_MS = 30000UL;
constexpr unsigned long SENSOR_STALE_MS = 10000UL;
constexpr size_t MAX_JSON_BYTES = 384;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT11);
NetworkClientSecure tlsClient;
MQTTClient mqttClient(512);
bool oledReady = false;
bool wifiAttemptActive = false;
bool ntpAttemptActive = false;
bool clockReady = false;
bool mqttConfigured = false;
bool sensorValid = false;
unsigned long wifiStartedAt = 0;
unsigned long nextWifiAt = 0;
unsigned long ntpStartedAt = 0;
unsigned long nextNtpAt = 0;
unsigned long nextMqttAt = 0;
unsigned long nextSensorAt = 0;
unsigned long lastSensorAt = 0;
unsigned long nextPublishAt = 0;
unsigned long publishSequence = 0;
unsigned long mqttBackoffMs = MQTT_RETRY_MS;
float temperatureC = NAN;
float humidityPct = NAN;
int lightRaw = 0;
char statusTopic[96] = {};
char dataTopic[96] = {};

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
  display.print("MQTT JSON SENSOR");
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

void readSensors(unsigned long now) {
  if (!due(now, nextSensorAt)) return;
  nextSensorAt = now + SENSOR_INTERVAL_MS;
  const float nextHumidity = dht.readHumidity();
  const float nextTemperature = dht.readTemperature();
  const int nextLight = analogRead(LIGHT_PIN);
  sensorValid = isfinite(nextTemperature) && isfinite(nextHumidity) &&
                nextTemperature >= -20.0f && nextTemperature <= 80.0f &&
                nextHumidity >= 0.0f && nextHumidity <= 100.0f &&
                nextLight >= 0 && nextLight <= 4095;
  if (!sensorValid) {
    drawStatus("NO DATA", "PUBLISH BLOCKED", "CHECK SENSOR");
    return;
  }
  temperatureC = nextTemperature;
  humidityPct = nextHumidity;
  lightRaw = nextLight;
  lastSensorAt = now;
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
  if (!mqttClient.publish(statusTopic, "{\"online\":true,\"schema\":1}", true, 1)) {
    drawStatus("PUB ERR", "STATUS NOT SENT", "RECONNECT");
    scheduleMqttRetry(now);
    return false;
  }
  nextPublishAt = now + 3000UL;
  mqttBackoffMs = MQTT_RETRY_MS;
  drawStatus("MQTT ONLINE", "JSON SCHEMA V1", "QOS 1");
  return true;
}

void publishJson(unsigned long now) {
  if (!sensorValid || now - lastSensorAt > SENSOR_STALE_MS) {
    drawStatus(sensorValid ? "STALE" : "NO DATA", "PUBLISH BLOCKED", "CHECK SENSOR");
    nextPublishAt = now + SENSOR_INTERVAL_MS;
    return;
  }
  const time_t currentEpoch = time(nullptr);
  if (currentEpoch <= 1700000000) {
    clockReady = false;
    nextNtpAt = now;
    drawStatus("TIME STALE", "PUBLISH BLOCKED", "NTP RETRY");
    return;
  }

  JsonDocument document;
  document["v"] = 1;
  document["device"] = MQTT_CLIENT_ID;
  document["seq"] = ++publishSequence;
  document["ts"] = currentEpoch;
  document["temp"] = roundf(temperatureC * 10.0f) / 10.0f;
  document["humi"] = roundf(humidityPct * 10.0f) / 10.0f;
  document["light"] = lightRaw;
  document["status"] = "ok";
  const size_t required = measureJson(document);
  if (required == 0 || required > MAX_JSON_BYTES) {
    drawStatus("JSON TOO LARGE", "PUBLISH BLOCKED", "LIMIT 384 B");
    nextPublishAt = now + PUBLISH_INTERVAL_MS;
    return;
  }
  char payload[MAX_JSON_BYTES + 1];
  const size_t written = serializeJson(document, payload, sizeof(payload));
  if (written != required) {
    drawStatus("JSON ERR", "SERIALIZE FAILED", "PUBLISH BLOCKED");
    nextPublishAt = now + PUBLISH_INTERVAL_MS;
    return;
  }
  if (mqttClient.publish(dataTopic, payload, false, 1)) {
    char detail[22];
    char footer[22];
    snprintf(detail, sizeof(detail), "T %.1f H %.0f L %d", temperatureC,
             humidityPct, lightRaw);
    snprintf(footer, sizeof(footer), "%u B SEQ %lu",
             static_cast<unsigned int>(written), publishSequence);
    drawStatus("PUB OK", detail, footer);
  } else {
    drawStatus("PUB ERR", "JSON NOT SENT", "RECONNECT");
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
  dht.begin();
  analogReadResolution(12);
  if (!configurationReady()) {
    drawStatus("CONFIG ERR", "COPY secrets.h", "TLS REQUIRED");
    return;
  }
  snprintf(statusTopic, sizeof(statusTopic), "%s/status", MQTT_TOPIC_ROOT);
  snprintf(dataTopic, sizeof(dataTopic), "%s/data", MQTT_TOPIC_ROOT);
  tlsClient.setCACert(MQTT_ROOT_CA);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, tlsClient);
  mqttClient.setOptions(30, true, 3000);
  mqttClient.setWill(statusTopic, "{\"online\":false}", true, 1);
  mqttConfigured = true;
  const unsigned long now = millis();
  nextWifiAt = now;
  nextSensorAt = now;
}

void loop() {
  if (!oledReady) {
    blinkError(findOledAddress() == 0 ? 2 : 3);
    return;
  }
  if (!configurationReady() || !mqttConfigured) return;
  const unsigned long now = millis();
  readSensors(now);
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
  if (due(now, nextPublishAt)) publishJson(now);
}
