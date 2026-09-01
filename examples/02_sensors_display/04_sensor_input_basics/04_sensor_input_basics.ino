#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int DIGITAL_INPUT_PIN = 14;
constexpr int ANALOG_INPUT_PIN = 33;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

uint8_t errorCode = 0;
bool oledReady = false;
unsigned long lastDisplayMillis = 0;

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

void drawInputs(int digitalValue, int analogValue) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("INPUT CHECK");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 15);
  display.print("D14  ");
  display.print(digitalValue == HIGH ? "HIGH" : "LOW");
  display.setCursor(0, 29);
  display.print("A33  ");
  display.print(analogValue);
  display.setCursor(0, 43);
  display.print("OPEN = UNDEFINED");

  const int barWidth = map(analogValue, 0, 4095, 0, 124);
  display.drawRect(1, 54, 126, 9, SSD1306_WHITE);
  display.fillRect(3, 56, constrain(barWidth, 0, 122), 5, SSD1306_WHITE);
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

  pinMode(DIGITAL_INPUT_PIN, INPUT_PULLDOWN);
  pinMode(ANALOG_INPUT_PIN, INPUT);
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

  const int digitalValue = digitalRead(DIGITAL_INPUT_PIN);
  const int analogValue = analogRead(ANALOG_INPUT_PIN);
  drawInputs(digitalValue, analogValue);
}
