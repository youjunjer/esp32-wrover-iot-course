#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int DHT_PIN = 14;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long DHT_INTERVAL_MS = 2000UL;
constexpr unsigned long DATA_STALE_MS = 10000UL;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT11);

uint8_t errorCode = 0;
bool oledReady = false;
bool hasReadAttempt = false;
bool hasGoodData = false;
bool lastReadSucceeded = false;
float lastTemperature = NAN;
float lastHumidity = NAN;
unsigned long bootMillis = 0;
unsigned long lastReadMillis = 0;
unsigned long lastGoodMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long readFailures = 0;

enum class DhtState { WAIT, OK, NO_DATA, READ_ERR, STALE };

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

DhtState currentDhtState(unsigned long now) {
  if (!hasReadAttempt) return DhtState::WAIT;
  if (!hasGoodData) return DhtState::NO_DATA;
  if (now - lastGoodMillis >= DATA_STALE_MS) return DhtState::STALE;
  return lastReadSucceeded ? DhtState::OK : DhtState::READ_ERR;
}

const char *stateText(DhtState state) {
  switch (state) {
    case DhtState::WAIT:
      return "WAIT";
    case DhtState::OK:
      return "OK";
    case DhtState::NO_DATA:
      return "NO DATA";
    case DhtState::READ_ERR:
      return "READ ERR";
    default:
      return "STALE";
  }
}

unsigned long cappedAgeSeconds(unsigned long now) {
  const unsigned long ageSeconds = (now - lastGoodMillis) / 1000UL;
  return ageSeconds > DISPLAY_COUNTER_MAX ? DISPLAY_COUNTER_MAX : ageSeconds;
}

void drawDhtState(unsigned long now) {
  const DhtState state = currentDhtState(now);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DHT11 GPIO14");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(stateText(state));
  display.setTextSize(1);

  if (state == DhtState::WAIT) {
    const unsigned long elapsed = now - bootMillis;
    const unsigned long waitLeft =
        elapsed >= DHT_INTERVAL_MS
            ? 0
            : (DHT_INTERVAL_MS - elapsed + 999UL) / 1000UL;
    display.setCursor(0, 38);
    display.print("FIRST READ ");
    display.print(waitLeft);
    display.print("s");
  } else if (state == DhtState::NO_DATA) {
    display.setCursor(0, 38);
    display.print("CHECK VCC/DATA/GND");
  } else {
    display.setCursor(0, 38);
    display.print(state == DhtState::OK ? "T " : "LAST ");
    display.print(lastTemperature, 1);
    display.print("C ");
    display.print(lastHumidity, 1);
    display.print("%");
    display.setCursor(0, 50);
    display.print("AGE ");
    display.print(cappedAgeSeconds(now));
    display.print("s");
  }

  display.setCursor(76, 50);
  display.print("ERR ");
  display.print(readFailures);
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setTimeOut(50);
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

  display.setTextWrap(false);
  display.setRotation(OLED_ROTATION);
  oledReady = true;
  dht.begin();
  bootMillis = millis();
  lastReadMillis = bootMillis;
  drawDhtState(bootMillis);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  if (now - lastReadMillis >= DHT_INTERVAL_MS) {
    lastReadMillis = now;
    hasReadAttempt = true;

    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();
    if (isnan(humidity) || isnan(temperature)) {
      lastReadSucceeded = false;
      if (readFailures < DISPLAY_COUNTER_MAX) ++readFailures;
    } else {
      lastHumidity = humidity;
      lastTemperature = temperature;
      lastGoodMillis = now;
      hasGoodData = true;
      lastReadSucceeded = true;
    }
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;
  drawDhtState(now);
}
