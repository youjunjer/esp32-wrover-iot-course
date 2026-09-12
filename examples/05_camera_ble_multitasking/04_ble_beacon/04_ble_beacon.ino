#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

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
bool advertising = false;
void setup() {
  if (!beginOled()) return;
  oledStatus("BLE BEACON", "INIT", "PUBLIC LAB ID");
  BLEDevice::init("WROVER-LAB");
  BLEAdvertising *advertiser = BLEDevice::getAdvertising();
  if (!advertiser) { oledStatus("BLE BEACON", "INIT ERR"); return; }
  BLEAdvertisementData data;
  data.setFlags(0x06);
  data.setManufacturerData(String(reinterpret_cast<const char *>(Course::LAB_BEACON), sizeof(Course::LAB_BEACON)));
  advertiser->setScanResponse(false);
  advertiser->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  advertising = advertiser->setAdvertisementData(data) && advertiser->start();
  oledStatus("BLE BEACON", advertising ? "ADV STARTED" : "ADV ERR", "MAJOR 1 / MINOR 1", "VERIFY ON SCANNER");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  delay(1000);
}
