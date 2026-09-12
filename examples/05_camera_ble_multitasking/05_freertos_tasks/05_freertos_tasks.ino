#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

bool beginOled(int sda = 21, int scl = 22) {
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

struct Sample { uint32_t sequence, sampledAt; int core; };
QueueHandle_t samples = nullptr;
bool taskReady = false;
void producer(void *) {
  TickType_t lastWake = xTaskGetTickCount();
  Sample sample = {};
  for (;;) {
    ++sample.sequence; sample.sampledAt = millis(); sample.core = xPortGetCoreID();
    xQueueOverwrite(samples, &sample);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(500));
  }
}
void setup() {
  if (!beginOled()) return;
  oledStatus("FREERTOS", "INIT", "QUEUE LENGTH 1");
  samples = xQueueCreate(1, sizeof(Sample));
  taskReady = samples && xTaskCreate(producer, "producer", 3072, nullptr, 1, nullptr) == pdPASS;
  if (!taskReady) oledStatus("FREERTOS", "TASK/QUEUE ERR", "RESET AFTER CHECK");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!taskReady) { delay(100); return; }
  Sample sample;
  if (xQueuePeek(samples, &sample, pdMS_TO_TICKS(100)) != pdTRUE) {
    oledStatus("FREERTOS", "NO DATA", "WAIT PRODUCER"); return;
  }
  char detail[22], footer[22];
  snprintf(detail, sizeof(detail), "SEQ %lu / CPU %d", static_cast<unsigned long>(sample.sequence), sample.core);
  snprintf(footer, sizeof(footer), "AGE %lums / UI CPU%d", static_cast<unsigned long>(millis() - sample.sampledAt), xPortGetCoreID());
  oledStatus("FREERTOS", millis() - sample.sampledAt > 1500 ? "STALE" : "OK", detail, footer);
  delay(200);
}
