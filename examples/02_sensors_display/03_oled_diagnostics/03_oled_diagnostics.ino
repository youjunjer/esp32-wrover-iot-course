#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long STATE_INTERVAL_MS = 3000UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

struct DemoState {
  const char *module;
  const char *status;
  const char *detail;
};

constexpr DemoState DEMO_STATES[] = {
    {"SYSTEM", "BOOT", "Starting"},
    {"OLED", "READY", "Address OK"},
    {"SENSOR", "NO DATA", "Value --"},
    {"NETWORK", "TIMEOUT", "Retry 2"},
    {"SENSOR", "STALE", "Age 12s"},
    {"SYSTEM", "ERR", "Code E21"},
};

uint8_t errorCode = 0;
bool oledReady = false;
size_t lastStateIndex = static_cast<size_t>(-1);

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

void drawDemoState(const DemoState &state) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DEMO STATUS");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.print(state.module);
  display.setTextSize(2);
  display.setCursor(0, 27);
  display.print(state.status);
  display.setTextSize(1);
  display.setCursor(0, 53);
  display.print(state.detail);
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
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const size_t stateCount = sizeof(DEMO_STATES) / sizeof(DEMO_STATES[0]);
  const size_t stateIndex = (millis() / STATE_INTERVAL_MS) % stateCount;
  if (stateIndex == lastStateIndex) return;

  lastStateIndex = stateIndex;
  drawDemoState(DEMO_STATES[stateIndex]);
}
