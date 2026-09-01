#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int LIGHT_PIN = 33;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long DISPLAY_INTERVAL_MS = 300UL;
constexpr size_t SAMPLE_COUNT = 8;

// 多數課程用光敏模組在較暗時 AO 數值較高；若實測相反，改為 false。
constexpr bool CALIBRATION_READY = false;
constexpr bool DARK_IS_HIGH = true;
constexpr int BRIGHT_THRESHOLD = 1200;
constexpr int DARK_THRESHOLD = 2800;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

uint8_t errorCode = 0;
bool oledReady = false;
unsigned long lastDisplayMillis = 0;
int observedMin = 4095;
int observedMax = 0;

enum class LightLevel { UNCAL, BRIGHT, MID, DARK };

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

int readAverage() {
  uint32_t total = 0;
  for (size_t sample = 0; sample < SAMPLE_COUNT; ++sample) {
    total += analogRead(LIGHT_PIN);
    delay(2);
  }
  return static_cast<int>(total / SAMPLE_COUNT);
}

LightLevel classifyLight(int raw) {
  if (!CALIBRATION_READY) return LightLevel::UNCAL;
  if (DARK_IS_HIGH) {
    if (raw <= BRIGHT_THRESHOLD) return LightLevel::BRIGHT;
    if (raw >= DARK_THRESHOLD) return LightLevel::DARK;
  } else {
    if (raw >= BRIGHT_THRESHOLD) return LightLevel::BRIGHT;
    if (raw <= DARK_THRESHOLD) return LightLevel::DARK;
  }
  return LightLevel::MID;
}

const char *levelText(LightLevel level) {
  switch (level) {
    case LightLevel::UNCAL:
      return "UNCAL";
    case LightLevel::BRIGHT:
      return "BRIGHT";
    case LightLevel::DARK:
      return "DARK";
    default:
      return "MID";
  }
}

void drawLight(int raw, LightLevel level) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LIGHT GPIO33");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 15);
  display.print("RAW ");
  display.print(raw);
  display.setTextSize(2);
  display.setCursor(0, 27);
  display.print(levelText(level));
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("MIN ");
  display.print(observedMin);
  display.print(" MAX ");
  display.print(observedMax);

  const int barWidth = map(raw, 0, 4095, 0, 124);
  display.drawRect(1, 56, 126, 8, SSD1306_WHITE);
  display.fillRect(3, 58, constrain(barWidth, 0, 122), 4, SSD1306_WHITE);
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
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;

  const int raw = readAverage();
  observedMin = min(observedMin, raw);
  observedMax = max(observedMax, raw);
  drawLight(raw, classifyLight(raw));
}
