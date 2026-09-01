#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr unsigned long PAGE_INTERVAL_MS = 3000UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

uint8_t errorCode = 0;
bool oledReady = false;
unsigned long lastDrawMs = 0;

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

void drawTextPage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PAGE 1  TEXT");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.println("OLED OK");
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.println("128 x 64 pixels");
  display.display();
}

void drawShapesPage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PAGE 2  SHAPES");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.drawRect(5, 18, 30, 30, SSD1306_WHITE);
  display.fillCircle(62, 33, 15, SSD1306_WHITE);
  display.drawTriangle(91, 48, 106, 18, 121, 48, SSD1306_WHITE);
  display.setCursor(0, 54);
  display.println("RECT  CIRCLE  TRI");
  display.display();
}

void drawProgressPage() {
  const uint8_t percent = (millis() / 100UL) % 101UL;
  const int barWidth = map(percent, 0, 100, 0, 116);
  char percentText[12];
  snprintf(percentText, sizeof(percentText), "%u%%", percent);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PAGE 3  UPDATE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(42, 18);
  display.println(percentText);
  display.drawRect(5, 43, 118, 14, SSD1306_WHITE);
  display.fillRect(6, 44, barWidth, 12, SSD1306_WHITE);
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
  drawTextPage();
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  if (millis() - lastDrawMs < 100UL) return;
  lastDrawMs = millis();

  switch ((millis() / PAGE_INTERVAL_MS) % 3UL) {
    case 0:
      drawTextPage();
      break;
    case 1:
      drawShapesPage();
      break;
    default:
      drawProgressPage();
      break;
  }
}
