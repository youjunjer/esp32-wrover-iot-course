#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

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

struct Job { int worker; int core; };
struct Result { int worker; int core; uint32_t checksum; };
Job jobs[2];
QueueHandle_t results = nullptr;
EventGroupHandle_t startGate = nullptr;
bool initialized = false, nextDual = false, running = false, fatal = false;
uint32_t started = 0, nextRun = 0;
int completed = 0;
Result received[2] = {};
void worker(void *argument) {
  const Job job = *static_cast<Job *>(argument);
  xEventGroupWaitBits(startGate, 1, pdFALSE, pdTRUE, portMAX_DELAY);
  uint32_t value = 1;
  for (int batch = 0; batch < 200; ++batch) {
    for (int i = 0; i < 50000; ++i) value = value * 1664525u + 1013904223u;
    vTaskDelay(1);  // Bounded work with idle time; do not starve the watchdog.
  }
  Result result{job.worker, xPortGetCoreID(), value};
  xQueueSend(results, &result, portMAX_DELAY);
  vTaskDelete(nullptr);
}
void setup() {
  if (!beginOled()) return;
  results = xQueueCreate(2, sizeof(Result)); startGate = xEventGroupCreate();
  initialized = results && startGate;
  if (!initialized) oledStatus("DUAL CORE", "ALLOC ERR");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!initialized || fatal) { delay(100); return; }
  if (!running && static_cast<int32_t>(millis() - nextRun) >= 0) {
    xEventGroupClearBits(startGate, 1); completed = 0;
    TaskHandle_t handles[2] = {};
    jobs[0] = {0, 1}; jobs[1] = {1, nextDual ? 0 : 1};
    for (int i = 0; i < 2; ++i) {
      if (xTaskCreatePinnedToCore(worker, "work", 3072, &jobs[i], 1, &handles[i], jobs[i].core) != pdPASS) {
        // Gate is closed: previously created task owns no resources yet.
        if (handles[0]) vTaskDelete(handles[0]);
        fatal = true; oledStatus("DUAL CORE", "TASK ERR"); return;
      }
    }
    oledStatus("DUAL CORE", nextDual ? "RUN CPU1 + CPU0" : "RUN CPU1 + CPU1", "SAME WORK / 2 TASKS");
    started = millis(); running = true; xEventGroupSetBits(startGate, 1);
  }
  Result result;
  if (running && xQueueReceive(results, &result, 0) == pdTRUE) {
    received[result.worker] = result; ++completed;
    if (completed == 2) {
      char detail[22], footer[22];
      snprintf(detail, sizeof(detail), "ELAPSED %lums", static_cast<unsigned long>(millis() - started));
      snprintf(footer, sizeof(footer), "CPU %d/%d CHECK %s", received[0].core, received[1].core,
               received[0].checksum == received[1].checksum ? "OK" : "ERR");
      oledStatus("DUAL CORE", nextDual ? "DUAL DONE" : "SAME CORE DONE", detail, footer);
      running = false; nextDual = !nextDual; nextRun = millis() + 8000;
    }
  }
  if (running && millis() - started > 30000) {
    fatal = true; oledStatus("DUAL CORE", "TIMEOUT", "RESET AFTER CHECK");
  }
  delay(20);
}
