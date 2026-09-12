#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEScan.h>

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

#include "../support/course_logic.h"
BLEScan *scanner = nullptr;
portMUX_TYPE statsLock = portMUX_INITIALIZER_UNLOCKED;
uint32_t labPackets = 0;
int labRssi = -127;
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) override {
    if (!device.haveManufacturerData()) return;
    const String bytes = device.getManufacturerData();
    if (!Course::isLabBeacon(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length())) return;
    portENTER_CRITICAL(&statsLock);
    ++labPackets; labRssi = device.getRSSI();
    portEXIT_CRITICAL(&statsLock);
  }
};
ScanCallbacks callbacks;
bool scanning = false;
uint32_t scanStarted = 0, nextScan = 0;
void setup() {
  if (!beginOled()) return;
  oledStatus("BLE SCAN", "INIT");
  BLEDevice::init(""); scanner = BLEDevice::getScan();
  if (!scanner) { oledStatus("BLE SCAN", "INIT ERR"); return; }
  scanner->setAdvertisedDeviceCallbacks(&callbacks, true);
  scanner->setActiveScan(false);
  scanner->setInterval(100); scanner->setWindow(60);
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!scanner) { delay(100); return; }
  const uint32_t now = millis();
  if (scanning) {
    if (now - scanStarted > 4000) {
      scanner->stop(); scanning = false; nextScan = now + 3000;
      oledStatus("BLE SCAN", "SCAN TIMEOUT", "RETRY IN 3s");
    } else if (!scanner->isScanning()) {
      scanning = false; nextScan = now + 3000;
      uint32_t count; int rssi;
      portENTER_CRITICAL(&statsLock); count = labPackets; rssi = labRssi; portEXIT_CRITICAL(&statsLock);
      char detail[22]; snprintf(detail, sizeof(detail), "PACKETS %lu", static_cast<unsigned long>(count));
      char signal[22]; snprintf(signal, sizeof(signal), "RSSI %d dBm", rssi);
      oledStatus("BLE SCAN", count ? "LAB SEEN" : "NO DATA", detail, count ? signal : "NO ATTENDANCE CLAIM");
      scanner->clearResults();
    }
  } else if (static_cast<int32_t>(now - nextScan) >= 0) {
    portENTER_CRITICAL(&statsLock); labPackets = 0; labRssi = -127; portEXIT_CRITICAL(&statsLock);
    oledStatus("BLE SCAN", "SCAN 2s", "LAB BEACON ONLY");
    // The synchronous pointer overload returns a results pointer even on failure.
    // Use the asynchronous bool overload so startup failure reaches the OLED.
    scanning = scanner->start(2, static_cast<void (*)(BLEScanResults)>(nullptr), false);
    scanStarted = now;
    if (!scanning) { nextScan = now + 3000; oledStatus("BLE SCAN", "SCAN ERR", "RETRY IN 3s"); }
  }
  delay(20);
}
