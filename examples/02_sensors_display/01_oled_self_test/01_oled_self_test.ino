#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

uint8_t oledAddress = 0;
uint8_t errorCode = 0;
bool oledReady = false;
unsigned long lastScreenUpdateMs = 0;

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};

  for (uint8_t address : CANDIDATE_ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      return address;
    }
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

void drawSelfTestScreen() {
  char addressText[16];
  char uptimeText[16];
  snprintf(addressText, sizeof(addressText), "ADDR 0x%02X", oledAddress);
  snprintf(uptimeText, sizeof(uptimeText), "UP %lus", millis() / 1000UL);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED SELF TEST");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println("INIT OK");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(addressText);
  display.setCursor(0, 53);
  display.println(uptimeText);
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  delay(50);

  oledAddress = findOledAddress();
  if (oledAddress == 0) {
    errorCode = ERROR_NO_OLED;
    return;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    errorCode = ERROR_DISPLAY_INIT;
    return;
  }

  display.setRotation(OLED_ROTATION);
  oledReady = true;
  drawSelfTestScreen();
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  if (millis() - lastScreenUpdateMs >= 1000UL) {
    lastScreenUpdateMs = millis();
    drawSelfTestScreen();
  }
}
