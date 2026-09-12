#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MQTT.h>
#include <mbedtls/base64.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool oledReady = false;
uint8_t oledError = 2;

void oledStatus(const char *title, const char *state, const char *detail = "", const char *footer = "") {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0); display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 17); display.print(state);
  display.setCursor(0, 34); display.print(detail);
  display.setCursor(0, 51); display.print(footer);
  display.display();
}

bool beginOled(int sda, int scl) {
  pinMode(2, OUTPUT); digitalWrite(2, LOW);
  if (!Wire.begin(sda, scl)) { oledError = 3; return false; }
  Wire.setTimeOut(50);
  delay(50);
  for (uint8_t address : {0x3C, 0x3D}) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    if (!display.begin(SSD1306_SWITCHCAPVCC, address, true, false)) {
      oledError = 3; return false;
    }
    display.setRotation(2); display.setTextWrap(false);
    oledReady = true;
    oledStatus("PART 5", "BOOT", "OLED OK");
    return true;
  }
  return false;
}

void oledFallback() {
  for (uint8_t i = 0; i < oledError; ++i) {
    digitalWrite(2, HIGH); delay(160);
    digitalWrite(2, LOW); delay(160);
  }
  delay(1000);
}

#define CAMERA_MODEL_AI_THINKER
#include "../support/course_logic.h"
#include "../support/camera_driver.h"
#if __has_include("camera_config.h")
#include "camera_config.h"
#else
#include "camera_config.example.h"
#endif
bool cameraReady = false;

bool cameraBoot() {
  pinMode(2, OUTPUT); digitalWrite(2, LOW);
  if (!Course::cameraOledPins(CAMERA_OLED_SDA, CAMERA_OLED_SCL)) {
    // No valid I2C bus can be initialized. Four pulses; never fall back to 21/22.
    oledError = 4; return false;
  }
  if (!beginOled(CAMERA_OLED_SDA, CAMERA_OLED_SCL)) return false;
  if (!OLED_PINS_VERIFIED || !ENABLE_CAMERA_TEST) {
    oledStatus("CAMERA PREFLIGHT", "CAM LOCKED", "CHECK OLED / BOARD", "READ CHAPTER 6");
    return false;
  }
  if (!psramFound()) { oledStatus("CAMERA PREFLIGHT", "PSRAM ERR"); return false; }
  oledStatus("CAMERA PREFLIGHT", "CAM INIT", "COEXISTENCE TEST");
  const esp_err_t error = initializeCamera();
  if (error != ESP_OK) {
    char detail[22]; snprintf(detail, sizeof(detail), "CODE 0x%x", static_cast<unsigned>(error));
    oledStatus("CAMERA PREFLIGHT", "CAM INIT ERR", detail); return false;
  }
  cameraReady = true; return true;
}

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif
bool placeholder(const char *value) {
  return !value || !*value || strstr(value, "REPLACE_WITH_");
}

struct FrameItem { uint8_t *data; size_t size; uint32_t capturedAt, sequence; };
bool publishFrame(const FrameItem &item);
QueueHandle_t frames = nullptr;
portMUX_TYPE stateLock = portMUX_INITIALIZER_UNLOCKED;
char captureState[22] = "PIR WARMUP 60s", networkState[22] = "INIT", progressState[22] = "";
bool tasksReady = false;
uint32_t captureStarted = 0;
char bootId[33] = {}, imageTopic[96] = {};
NetworkClientSecure tlsClient;
MQTTClient mqttClient(1024);

void setState(char *target, const char *value) {
  portENTER_CRITICAL(&stateLock); snprintf(target, 22, "%s", value); portEXIT_CRITICAL(&stateLock);
}
void captureTask(void *) {
  bool warmupComplete = false;
  bool wasHigh = true;  // A fresh LOW -> HIGH is required after warmup.
  uint32_t lastCapture = 0, sequence = 0;
  for (;;) {
    if (millis() - captureStarted < 60000) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
    if (!warmupComplete) { warmupComplete = true; setState(captureState, "WAIT PIR LOW"); }
    const bool high = digitalRead(CAMERA_PIR_PIN) == HIGH;
    if (!high && wasHigh) setState(captureState, "PIR READY");
    const bool trigger = high && !wasHigh;
    wasHigh = high;
    if (trigger && (!lastCapture || millis() - lastCapture >= 30000)) {
      lastCapture = millis();
      if (uxQueueMessagesWaiting(frames)) { setState(captureState, "QUEUE FULL / DROP"); continue; }
      setState(captureState, "CAPTURE");
      camera_fb_t *frame = esp_camera_fb_get();
      if (!frame) { setState(captureState, "FRAME ERR"); continue; }
      if (frame->format != PIXFORMAT_JPEG || !frame->len || frame->len > Course::JPEG_LIMIT) {
        esp_camera_fb_return(frame); setState(captureState, "FRAME SIZE ERR"); continue;
      }
      FrameItem item = {static_cast<uint8_t *>(heap_caps_malloc(frame->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
                        frame->len, millis(), ++sequence};
      if (item.data) memcpy(item.data, frame->buf, frame->len);
      esp_camera_fb_return(frame);
      if (!item.data) { setState(captureState, "ALLOC ERR"); continue; }
      if (xQueueSend(frames, &item, 0) != pdTRUE) { free(item.data); setState(captureState, "QUEUE FULL / DROP"); }
      else setState(captureState, "QUEUED / HOLD 30s");
    } else if (trigger) setState(captureState, "RATE LIMIT 30s");
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
bool networkReady() {
  if (WiFi.status() != WL_CONNECTED) {
    mqttClient.disconnect(); tlsClient.stop();
    setState(networkState, "WIFI CONNECT 15s");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) vTaskDelay(pdMS_TO_TICKS(100));
    if (WiFi.status() != WL_CONNECTED) { WiFi.disconnect(); setState(networkState, "WIFI TIMEOUT"); return false; }
  }
  if (time(nullptr) < 1767225600) {
    setState(networkState, "NTP SYNC 12s");
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    const uint32_t start = millis();
    while (time(nullptr) < 1767225600 && millis() - start < 12000) vTaskDelay(pdMS_TO_TICKS(100));
    if (time(nullptr) < 1767225600) { setState(networkState, "NTP TIMEOUT"); return false; }
  }
  if (!mqttClient.connected()) {
    setState(networkState, "MQTT TLS CONNECT");
    if (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      setState(networkState, "TLS/MQTT ERR"); return false;
    }
    setState(networkState, "MQTT ONLINE"); setState(progressState, "WAIT PIR");
  }
  return true;
}
bool publishDocument(JsonDocument &document) {
  char payload[850];
  if (measureJson(document) >= sizeof(payload)) return false;
  const size_t size = serializeJson(document, payload, sizeof(payload));
  return mqttClient.publish(imageTopic, payload, static_cast<int>(size), false, 1);
}
bool publishFrame(const FrameItem &item) {
  if (millis() - item.capturedAt > 15000) { setState(networkState, "FRAME STALE / DROP"); return false; }
  char id[46], checksum[9];
  snprintf(id, sizeof(id), "%s-%lu", bootId, static_cast<unsigned long>(item.sequence));
  snprintf(checksum, sizeof(checksum), "%08lx", static_cast<unsigned long>(Course::crc32(item.data, item.size)));
  JsonDocument document;
  document["v"] = 1; document["kind"] = "begin"; document["id"] = id;
  document["bytes"] = item.size; document["chunks"] = Course::chunkCount(item.size);
  document["crc32"] = checksum; document["mime"] = "image/jpeg";
  setState(networkState, "PUBLISH BEGIN");
  if (!publishDocument(document)) return false;
  for (size_t index = 0, offset = 0; offset < item.size; ++index, offset += Course::CHUNK_BYTES) {
    if (millis() - item.capturedAt > 30000 || !mqttClient.connected()) return false;
    unsigned char encoded[513]; size_t written = 0;
    if (mbedtls_base64_encode(encoded, sizeof(encoded), &written, item.data + offset,
          min(Course::CHUNK_BYTES, item.size - offset)) != 0) return false;
    encoded[written] = 0;
    document.clear(); document["v"] = 1; document["kind"] = "chunk"; document["id"] = id;
    document["index"] = index; document["data"] = reinterpret_cast<const char *>(encoded);
    char progress[22]; snprintf(progress, sizeof(progress), "CHUNK %u/%u", static_cast<unsigned>(index + 1), static_cast<unsigned>(Course::chunkCount(item.size)));
    setState(networkState, "PUBLISH CHUNKS"); setState(progressState, progress);
    if (!publishDocument(document)) return false;
    mqttClient.loop(); vTaskDelay(1);
  }
  if (millis() - item.capturedAt > 30000) return false;
  document.clear(); document["v"] = 1; document["kind"] = "end"; document["id"] = id; document["crc32"] = checksum;
  setState(networkState, "PUBLISH END");
  return publishDocument(document);
}
void networkTask(void *) {
  WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(false);
  tlsClient.setCACert(MQTT_ROOT_CA); tlsClient.setTimeout(5000); tlsClient.setHandshakeTimeout(5);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, tlsClient);
  mqttClient.setOptions(30, true, 5000);
  for (;;) {
    if (!networkReady()) {
      // Any queued frame expires while offline; never claim it as a new capture.
      setState(progressState, "RETRY IN 10s"); vTaskDelay(pdMS_TO_TICKS(10000)); continue;
    }
    mqttClient.loop();
    if (!mqttClient.connected()) { setState(networkState, "LINK LOST / RETRY"); continue; }
    FrameItem item;
    if (xQueueReceive(frames, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
      const bool stale = millis() - item.capturedAt > 15000;
      const bool ok = !stale && publishFrame(item);
      free(item.data);  // Network owns every dequeued allocation, including failures.
      setState(networkState, stale ? "FRAME STALE / DROP" : (ok ? "BROKER ACK" : "PUB ERR / DROP"));
      setState(progressState, ok ? "VERIFY ON RECEIVER" : "WAIT NEXT PIR");
      if (!ok && !stale) { mqttClient.disconnect(); tlsClient.stop(); vTaskDelay(pdMS_TO_TICKS(10000)); }
    }
  }
}
void setup() {
  if (!cameraBoot()) return;
  if (!PIR_PIN_VERIFIED || !Course::alternativePin(CAMERA_PIR_PIN) || CAMERA_PIR_PIN == CAMERA_OLED_SDA || CAMERA_PIR_PIN == CAMERA_OLED_SCL) {
    oledStatus("CAMERA MQTT", "PIR CONFIG ERR", "CHECK BOARD / PINS"); return;
  }
  const size_t topicSize = strlen(MQTT_TOPIC_ROOT);
  if (USING_EXAMPLE_SECRETS || placeholder(WIFI_SSID) || placeholder(WIFI_PASSWORD) || placeholder(MQTT_HOST) ||
      placeholder(MQTT_USERNAME) || placeholder(MQTT_PASSWORD) || placeholder(MQTT_CLIENT_ID) || placeholder(MQTT_TOPIC_ROOT) ||
      placeholder(MQTT_ROOT_CA) || MQTT_PORT != 8883 || strncmp(MQTT_ROOT_CA, "-----BEGIN CERTIFICATE-----", 26) != 0 ||
      strchr(MQTT_HOST, '/') || strchr(MQTT_HOST, ':') || strlen(MQTT_CLIENT_ID) > 23 ||
      topicSize < 8 || topicSize > 72 || MQTT_TOPIC_ROOT[0] == '/' || MQTT_TOPIC_ROOT[topicSize - 1] == '/' ||
      strchr(MQTT_TOPIC_ROOT, '#') || strchr(MQTT_TOPIC_ROOT, '+')) {
    oledStatus("CAMERA MQTT", "CONFIG ERR", "CHECK secrets.h"); return;
  }
  snprintf(imageTopic, sizeof(imageTopic), "%s/image", MQTT_TOPIC_ROOT);
  for (int i = 0; i < 4; ++i) snprintf(bootId + i * 8, 9, "%08lx", static_cast<unsigned long>(esp_random()));
  pinMode(CAMERA_PIR_PIN, INPUT); captureStarted = millis();
  frames = xQueueCreate(1, sizeof(FrameItem));
  if (!frames) { oledStatus("CAMERA MQTT", "QUEUE ERR"); return; }
  // Start network first. If producer creation fails, network only idles on an empty queue.
  const bool networkOk = xTaskCreatePinnedToCore(networkTask, "network", 12288, nullptr, 1, nullptr, 0) == pdPASS;
  tasksReady = networkOk && xTaskCreatePinnedToCore(captureTask, "capture", 6144, nullptr, 1, nullptr, 1) == pdPASS;
  if (!tasksReady) oledStatus("CAMERA MQTT", "TASK ERR", "RESET AFTER CHECK");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!tasksReady) { delay(100); return; }
  char capture[22], network[22], progress[22];
  portENTER_CRITICAL(&stateLock);
  memcpy(capture, captureState, sizeof(capture)); memcpy(network, networkState, sizeof(network)); memcpy(progress, progressState, sizeof(progress));
  portEXIT_CRITICAL(&stateLock);
  oledStatus("PIR + CAMERA + MQTT", capture, network, progress);
  delay(200);
}
