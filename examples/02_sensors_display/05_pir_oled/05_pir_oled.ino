#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int PIR_PIN = 14;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long PIR_WARMUP_MS = 60000UL;
constexpr unsigned long PIR_STABLE_MS = 150UL;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

uint8_t errorCode = 0;
bool oledReady = false;
bool warmupFinished = false;
bool lastRawPirHigh = false;
bool stablePirHigh = false;
unsigned long bootMillis = 0;
unsigned long rawChangeMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long motionEvents = 0;

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void blinkErrorCode(uint8_t code) {
  for (uint8_t pulse = 0; pulse < code; ++pulse) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(160);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(160);
  }
  delay(1000);
}

void drawPirState(const char *state, const char *detail, bool rawHigh) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PIR GPIO14");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(state);
  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print(detail);
  display.setCursor(0, 54);
  display.print("RAW ");
  display.print(rawHigh ? "HIGH" : "LOW");
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(50);

  const uint8_t address = findOledAddress();
  if (address == 0) {
    errorCode = 2;
    return;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    errorCode = 3;
    return;
  }

  display.setRotation(OLED_ROTATION);
  oledReady = true;
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  bootMillis = millis();
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  const bool pirHigh = digitalRead(PIR_PIN) == HIGH;

  if (!warmupFinished) {
    const unsigned long elapsed = now - bootMillis;
    if (elapsed < PIR_WARMUP_MS) {
      if (now - lastDisplayMillis >= DISPLAY_INTERVAL_MS) {
        lastDisplayMillis = now;
        const unsigned long secondsLeft =
            (PIR_WARMUP_MS - elapsed + 999UL) / 1000UL;
        char detail[20];
        snprintf(detail, sizeof(detail), "LEFT %lus", secondsLeft);
        drawPirState("WARMUP", detail, pirHigh);
      }
      return;
    }

    warmupFinished = true;
    lastRawPirHigh = pirHigh;
    stablePirHigh = pirHigh;
    rawChangeMillis = now;
  }

  if (pirHigh != lastRawPirHigh) {
    lastRawPirHigh = pirHigh;
    rawChangeMillis = now;
  }
  if (stablePirHigh != lastRawPirHigh &&
      now - rawChangeMillis >= PIR_STABLE_MS) {
    stablePirHigh = lastRawPirHigh;
    if (stablePirHigh) ++motionEvents;
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;

  char detail[20];
  snprintf(detail, sizeof(detail), "EVENTS %lu", motionEvents);
  drawPirState(stablePirHigh ? "MOTION" : "CLEAR", detail, pirHigh);
}
