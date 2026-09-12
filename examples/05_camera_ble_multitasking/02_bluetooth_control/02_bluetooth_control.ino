#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <DHT.h>
#include <ESP32Servo.h>

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
DHT dht(14, DHT11);
Servo servo;
Course::LineReader line;
bool btReady = false, connected = false, outputsActive = false;
uint32_t leaseStarted = 0, lastRead = 0, lastGood = 0, lastPaint = 0;
float temperature = NAN, humidity = NAN;
const char *commandState = "SAFE OFF";
bool sensorGood = false;

void safeOutputs() {
  digitalWrite(4, LOW);
  if (servo.attached()) servo.detach();
  outputsActive = false;
}
void command(const char *text) {
  if (!strcmp(text, "OFF")) { safeOutputs(); commandState = "SAFE OFF"; return; }
  const int angle = Course::servoAngle(text);
  if (strcmp(text, "LED ON") && strcmp(text, "LED OFF") && angle < 0) {
    safeOutputs(); commandState = "CMD ERR"; return;
  }
  if (digitalRead(13) != LOW) { safeOutputs(); commandState = "HOLD BTN13"; return; }
  if (angle >= 0) {
    if (!servo.attached()) servo.attach(5, 500, 2400);
    if (!servo.attached()) { safeOutputs(); commandState = "SERVO ERR"; return; }
    servo.write(angle);
  } else digitalWrite(4, !strcmp(text, "LED ON") ? HIGH : LOW);
  outputsActive = true; leaseStarted = millis(); commandState = "CMD OK";
}
void paint() {
  char sensor[22];
  if (!lastGood) snprintf(sensor, sizeof(sensor), "DHT NO DATA");
  else if (millis() - lastGood > 10000) snprintf(sensor, sizeof(sensor), "DHT STALE");
  else if (!sensorGood) snprintf(sensor, sizeof(sensor), "DHT READ ERR");
  else snprintf(sensor, sizeof(sensor), "T%.1fC H%.0f%%", temperature, humidity);
  oledStatus("BT DHT CONTROL", commandState, sensor, connected ? "CONNECTED / LEASE 5s" : "WAIT CLIENT");
}
void setup() {
  digitalWrite(4, LOW); pinMode(4, OUTPUT); pinMode(13, INPUT_PULLUP);
  safeOutputs();
  if (!beginOled()) return;
  dht.begin();
  servo.setPeriodHertz(50);
  oledStatus("BT DHT CONTROL", "INIT", "SPP / JUST WORKS");
  bt.enableSSP(false, false); btReady = bt.begin("WROVER-CLASS-CONTROL");
  if (!btReady) commandState = "BT INIT ERR";
  paint();
}
void loop() {
  if (!oledReady) { safeOutputs(); oledFallback(); return; }
  if (!btReady) { delay(100); return; }
  const bool online = bt.hasClient();
  if (online != connected) {
    connected = online; safeOutputs(); line.reset(); commandState = online ? "SAFE OFF" : "LINK LOST";
  }
  if (outputsActive && (!connected || digitalRead(13) != LOW || millis() - leaseStarted >= 5000)) {
    safeOutputs(); commandState = "LEASE/BTN OFF";
  }
  for (int budget = 0; connected && bt.available() && budget < 64; ++budget) {
    const auto result = line.push(static_cast<char>(bt.read()), millis());
    if (result == Course::LineResult::Waiting) continue;
    if (result == Course::LineResult::Ready) command(line.text);
    else { safeOutputs(); commandState = "RX ERR"; }
    line.reset(); paint(); bt.println(commandState);
  }
  if (line.expired(millis())) { line.reset(); safeOutputs(); commandState = "RX TIMEOUT"; }
  if (millis() - lastRead >= 2000) {
    lastRead = millis(); temperature = dht.readTemperature(); humidity = dht.readHumidity();
    sensorGood = isfinite(temperature) && isfinite(humidity) && humidity >= 0 && humidity <= 100;
    if (sensorGood) lastGood = millis();
    paint();
    if (connected) {
      if (sensorGood) bt.printf("T=%.1f,H=%.0f\n", temperature, humidity);
      else bt.println("DHT NO DATA");
    }
  }
  if (millis() - lastPaint >= 200) { lastPaint = millis(); paint(); }
  delay(10);
}
