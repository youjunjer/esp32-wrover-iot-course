#include <cstring>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <LiquidCrystal_PCF8574.h>

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
constexpr unsigned long LCD_PROBE_INTERVAL_MS = 2000UL;
constexpr unsigned long LCD_RETRY_INTERVAL_MS = 5000UL;
constexpr unsigned long LCD_FULL_REFRESH_INTERVAL_MS = 10000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT11);
LiquidCrystal_PCF8574 lcd27(0x27);
LiquidCrystal_PCF8574 lcd3F(0x3F);
LiquidCrystal_PCF8574 *lcdDevice = nullptr;

uint8_t degreeGlyph[8] = {
    0b00110, 0b01001, 0b01001, 0b00110,
    0b00000, 0b00000, 0b00000, 0b00000};
char lcdCache[2][16] = {};

uint8_t errorCode = 0;
uint8_t oledAddress = 0;
uint8_t lcdAddress = 0;
bool oledReady = false;
bool lcdReady = false;
bool hasReadAttempt = false;
bool hasGoodData = false;
bool lastReadSucceeded = false;
float lastTemperature = NAN;
float lastHumidity = NAN;
unsigned long lastReadMillis = 0;
unsigned long lastGoodMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long lastLcdProbeMillis = 0;
unsigned long lastLcdRetryMillis = 0;
unsigned long lastLcdFullRefreshMillis = 0;
unsigned long readFailures = 0;

enum class DhtState { WAIT, OK, NO_DATA, READ_ERR, STALE };

bool addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
    if (addressResponds(address)) return address;
  }
  return 0;
}

uint8_t findLcdAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x27, 0x3F};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
    if (addressResponds(address)) return address;
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

void drawStartup(const char *state, const char *detail) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DHT11 + LCD1602");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(state);
  display.setTextSize(1);
  display.setCursor(0, 46);
  display.print(detail);
  display.display();
}

bool connectLcd() {
  lcdReady = false;
  lcdAddress = findLcdAddress();
  if (lcdAddress == 0) {
    lcdDevice = nullptr;
    return false;
  }

  lcdDevice = lcdAddress == 0x27 ? &lcd27 : &lcd3F;
  lcdDevice->begin(16, 2);
  if (!lcdDevice->isConnected()) {
    lcdDevice = nullptr;
    lcdAddress = 0;
    return false;
  }

  lcdDevice->setBacklight(255);
  lcdDevice->createChar(1, degreeGlyph);
  memset(lcdCache, 0, sizeof(lcdCache));
  lastLcdFullRefreshMillis = millis();
  lcdReady = true;
  return true;
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

unsigned long lcdRetrySeconds(unsigned long now) {
  const unsigned long elapsed = now - lastLcdRetryMillis;
  if (elapsed >= LCD_RETRY_INTERVAL_MS) return 0;
  return (LCD_RETRY_INTERVAL_MS - elapsed + 999UL) / 1000UL;
}

void drawOled(unsigned long now) {
  const DhtState state = currentDhtState(now);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DHT11 + LCD1602");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 14);
  if (lcdReady) {
    display.print("LCD ACK 0x");
    display.print(lcdAddress, HEX);
  } else {
    display.print("LCD NO ACK R");
    display.print(lcdRetrySeconds(now));
    display.print("s");
  }
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(stateText(state));
  display.setTextSize(1);
  display.setCursor(0, 46);

  if (state == DhtState::WAIT) {
    display.print("FIRST READ 2s");
  } else if (state == DhtState::NO_DATA) {
    display.print("CHECK DHT WIRING");
  } else {
    display.print(state == DhtState::OK ? "T " : "LAST ");
    display.print(lastTemperature, 1);
    display.print("C ");
    display.print(lastHumidity, 0);
    display.print("%");
  }

  display.setCursor(0, 56);
  if (hasGoodData) {
    display.print("AGE ");
    display.print(cappedAgeSeconds(now));
    display.print("s ");
  }
  display.print("ERR ");
  display.print(readFailures);
  display.display();
}

void writeLcdLine(uint8_t row, const char *text) {
  if (!lcdReady || lcdDevice == nullptr || row > 1) return;

  char padded[16];
  memset(padded, ' ', sizeof(padded));
  const size_t length = min(strlen(text), sizeof(padded));
  memcpy(padded, text, length);
  if (memcmp(lcdCache[row], padded, sizeof(padded)) == 0) return;

  lcdDevice->setCursor(0, row);
  for (char character : padded) {
    lcdDevice->write(static_cast<uint8_t>(character));
  }
  memcpy(lcdCache[row], padded, sizeof(padded));
}

void updateLcd(unsigned long now) {
  if (!lcdReady) return;

  const DhtState state = currentDhtState(now);
  char line0[24];
  char line1[24];
  if (state == DhtState::OK) {
    snprintf(line0, sizeof(line0), "T:%4.1f%cC H:%2.0f%%",
             lastTemperature, 1, lastHumidity);
    snprintf(line1, sizeof(line1), "DHT OK AGE %lus",
             cappedAgeSeconds(now));
  } else if ((state == DhtState::READ_ERR || state == DhtState::STALE) &&
             hasGoodData) {
    snprintf(line0, sizeof(line0), "DHT %s", stateText(state));
    snprintf(line1, sizeof(line1), "LAST AGE %lus",
             cappedAgeSeconds(now));
  } else {
    snprintf(line0, sizeof(line0), "DHT %s", stateText(state));
    snprintf(line1, sizeof(line1), "NO LIVE DATA");
  }

  writeLcdLine(0, line0);
  writeLcdLine(1, line1);
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL, 100000);
  Wire.setTimeOut(50);
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

  display.setTextWrap(false);
  display.setRotation(OLED_ROTATION);
  oledReady = true;
  drawStartup("LCD SCAN", "TRY 0x27 / 0x3F");
  connectLcd();

  dht.begin();
  const unsigned long now = millis();
  lastReadMillis = now;
  lastLcdProbeMillis = now;
  lastLcdRetryMillis = now;
  drawOled(now);
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

  if (lcdReady && now - lastLcdProbeMillis >= LCD_PROBE_INTERVAL_MS) {
    lastLcdProbeMillis = now;
    if (lcdDevice == nullptr || !lcdDevice->isConnected()) {
      lcdReady = false;
      lcdDevice = nullptr;
      lcdAddress = 0;
      lastLcdRetryMillis = now;
    }
  }
  if (!lcdReady && now - lastLcdRetryMillis >= LCD_RETRY_INTERVAL_MS) {
    lastLcdRetryMillis = now;
    connectLcd();
  }
  if (lcdReady &&
      now - lastLcdFullRefreshMillis >= LCD_FULL_REFRESH_INTERVAL_MS) {
    memset(lcdCache, 0, sizeof(lcdCache));
    lastLcdFullRefreshMillis = now;
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;
  drawOled(now);
  updateLcd(now);
}
