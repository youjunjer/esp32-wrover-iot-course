#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>

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
BluetoothSerial bt;
Course::LineReader line;
bool btReady = false, connected = false;
uint32_t received = 0;

void reply(const char *state, const char *message) {
  // Never echo arbitrary user text onto OLED or public logs.
  oledStatus("BT TEXT", state, message, "LF / MAX 64 BYTES");
  bt.println(message);
}
void setup() {
  if (!beginOled()) return;
  oledStatus("BT TEXT", "INIT", "SPP / JUST WORKS");
  bt.enableSSP(false, false);
  btReady = bt.begin("WROVER-CLASS-TEXT");
  oledStatus("BT TEXT", btReady ? "WAIT CLIENT" : "BT INIT ERR", "SPP / JUST WORKS");
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!btReady) { delay(100); return; }
  const bool online = bt.hasClient();
  if (online != connected) {
    connected = online; line.reset();
    oledStatus("BT TEXT", online ? "CONNECTED" : "LINK LOST", "SPP / JUST WORKS");
  }
  if (!connected) { delay(20); return; }
  for (int budget = 0; bt.available() && budget < 64; ++budget) {
    const auto result = line.push(static_cast<char>(bt.read()), millis());
    if (result == Course::LineResult::Waiting) continue;
    if (result == Course::LineResult::Invalid) reply("RX ERR", "INVALID LINE");
    else {
      ++received;
      if (!strcmp(line.text, "PING")) reply("RX OK", "PONG");
      else if (!strcmp(line.text, "HELLO")) reply("RX OK", "HELLO CLASS");
      else reply("CMD ERR", "USE HELLO OR PING");
    }
    line.reset();
  }
  if (line.expired(millis())) { line.reset(); reply("RX TIMEOUT", "LINE TIMEOUT"); }
  delay(10);
}
