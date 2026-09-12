#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

uint32_t lastCapture = 0;
void setup() {
  if (cameraBoot()) oledStatus("CAMERA PREFLIGHT", "CAM READY", "FIRST FRAME IN 5s");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!cameraReady) { delay(100); return; }
  if (millis() - lastCapture >= 5000) {
    lastCapture = millis();
    oledStatus("CAMERA PREFLIGHT", "CAPTURE");
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) oledStatus("CAMERA PREFLIGHT", "FRAME ERR", "RETRY IN 5s");
    else {
      char detail[22]; snprintf(detail, sizeof(detail), "%ux%u %uB", static_cast<unsigned>(frame->width),
        static_cast<unsigned>(frame->height), static_cast<unsigned>(frame->len));
      oledStatus("CAMERA PREFLIGHT", frame->format == PIXFORMAT_JPEG ? "FRAME OK" : "FORMAT ERR", detail, "CHECK OLED STABILITY");
      esp_camera_fb_return(frame);
    }
  }
  delay(20);
}
