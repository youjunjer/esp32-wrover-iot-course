#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int TRIG_PIN = 25;
constexpr int ECHO_PIN = 14;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long ECHO_TIMEOUT_US = 30000UL;
constexpr unsigned long MEASUREMENT_INTERVAL_MS = 250UL;
constexpr float SOUND_CM_PER_US = 0.0343F;
constexpr float MIN_DISTANCE_CM = 2.0F;
constexpr float MAX_DISTANCE_CM = 400.0F;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

enum class DistanceStatus { OK, TIMEOUT, RANGE_ERR };

struct DistanceReading {
  DistanceStatus status;
  unsigned long echoUs;
  float distanceCm;
};

uint8_t errorCode = 0;
bool oledReady = false;
unsigned long lastMeasurementMs = 0;
unsigned int consecutiveInvalidReadings = 0;

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

DistanceReading measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  const unsigned long echoUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (echoUs == 0) return {DistanceStatus::TIMEOUT, 0, 0.0F};

  const float distanceCm = echoUs * SOUND_CM_PER_US / 2.0F;
  if (distanceCm < MIN_DISTANCE_CM || distanceCm > MAX_DISTANCE_CM) {
    return {DistanceStatus::RANGE_ERR, echoUs, distanceCm};
  }
  return {DistanceStatus::OK, echoUs, distanceCm};
}

const char *statusText(DistanceStatus status) {
  switch (status) {
    case DistanceStatus::OK:
      return "OK";
    case DistanceStatus::TIMEOUT:
      return "TIMEOUT";
    default:
      return "RANGE ERR";
  }
}

void drawReading(const DistanceReading &reading) {
  char detail[22];
  char footer[22];
  if (reading.status == DistanceStatus::OK) {
    snprintf(detail, sizeof(detail), "DIST %.1f cm", reading.distanceCm);
    snprintf(footer, sizeof(footer), "ECHO %lu us", reading.echoUs);
  } else if (reading.status == DistanceStatus::TIMEOUT) {
    snprintf(detail, sizeof(detail), "NO VALID DIST");
    snprintf(footer, sizeof(footer), "INVALID %u / 30ms",
             consecutiveInvalidReadings);
  } else {
    snprintf(detail, sizeof(detail), "VALUE %.1f cm", reading.distanceCm);
    snprintf(footer, sizeof(footer), "OUT OF 2-400");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ULTRASONIC");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(statusText(reading.status));
  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print(detail);
  display.setCursor(0, 54);
  display.print(footer);
  display.display();
}

void drawInit() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ULTRASONIC");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("INIT");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("T25 E14 / 30ms");
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(OLED_SDA, OLED_SCL);
  delay(50);
  const uint8_t address = findOledAddress();
  if (address == 0) {
    errorCode = ERROR_NO_OLED;
    return;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    errorCode = ERROR_DISPLAY_INIT;
    return;
  }

  display.setRotation(OLED_ROTATION);
  display.setTextWrap(false);
  oledReady = true;
  drawInit();
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  if (now - lastMeasurementMs < MEASUREMENT_INTERVAL_MS) return;
  lastMeasurementMs = now;

  const DistanceReading reading = measureDistance();
  if (reading.status == DistanceStatus::OK) {
    consecutiveInvalidReadings = 0;
  } else if (consecutiveInvalidReadings < 999U) {
    ++consecutiveInvalidReadings;
  }
  drawReading(reading);
}
