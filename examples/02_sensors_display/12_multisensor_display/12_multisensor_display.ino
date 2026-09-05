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
constexpr int LIGHT_PIN = 33;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long DHT_INTERVAL_MS = 2000UL;
constexpr unsigned long DATA_STALE_MS = 10000UL;
constexpr unsigned long LIGHT_INTERVAL_MS = 300UL;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;
constexpr unsigned long PAGE_INTERVAL_MS = 3000UL;
constexpr unsigned long LCD_PROBE_INTERVAL_MS = 2000UL;
constexpr unsigned long LCD_RETRY_INTERVAL_MS = 5000UL;
constexpr unsigned long LCD_FULL_REFRESH_INTERVAL_MS = 10000UL;
constexpr unsigned long DISPLAY_COUNTER_MAX = 9999UL;
constexpr size_t LIGHT_SAMPLE_COUNT = 8;

constexpr bool CALIBRATION_READY = false;
constexpr bool DARK_IS_HIGH = true;
constexpr int BRIGHT_THRESHOLD = 1200;
constexpr int DARK_THRESHOLD = 2800;

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
uint8_t currentPage = 0;
bool oledReady = false;
bool lcdReady = false;
bool hasReadAttempt = false;
bool hasGoodDht = false;
bool lastDhtReadSucceeded = false;
bool lightReady = false;
float lastTemperature = NAN;
float lastHumidity = NAN;
int lightRaw = 0;
int observedLightMin = 4095;
int observedLightMax = 0;
unsigned long bootMillis = 0;
unsigned long lastDhtReadMillis = 0;
unsigned long lastDhtGoodMillis = 0;
unsigned long lastLightReadMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long lastPageMillis = 0;
unsigned long lastLcdProbeMillis = 0;
unsigned long lastLcdRetryMillis = 0;
unsigned long lastLcdFullRefreshMillis = 0;
unsigned long dhtFailures = 0;

enum class DhtState { WAIT, OK, NO_DATA, READ_ERR, STALE };
enum class LightLevel { UNCAL, BRIGHT, MID, DARK };

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
  display.print("MULTISENSOR");
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
  if (!hasGoodDht) return DhtState::NO_DATA;
  if (now - lastDhtGoodMillis >= DATA_STALE_MS) return DhtState::STALE;
  return lastDhtReadSucceeded ? DhtState::OK : DhtState::READ_ERR;
}

const char *dhtStateText(DhtState state) {
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

bool isDhtFault(DhtState state) {
  return state == DhtState::NO_DATA || state == DhtState::READ_ERR ||
         state == DhtState::STALE;
}

unsigned long cappedDhtAgeSeconds(unsigned long now) {
  const unsigned long ageSeconds = (now - lastDhtGoodMillis) / 1000UL;
  return ageSeconds > DISPLAY_COUNTER_MAX ? DISPLAY_COUNTER_MAX : ageSeconds;
}

unsigned long lcdRetrySeconds(unsigned long now) {
  const unsigned long elapsed = now - lastLcdRetryMillis;
  if (elapsed >= LCD_RETRY_INTERVAL_MS) return 0;
  return (LCD_RETRY_INTERVAL_MS - elapsed + 999UL) / 1000UL;
}

int readLightAverage() {
  uint32_t total = 0;
  for (size_t sample = 0; sample < LIGHT_SAMPLE_COUNT; ++sample) {
    total += analogRead(LIGHT_PIN);
    delay(2);
  }
  return static_cast<int>(total / LIGHT_SAMPLE_COUNT);
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

const char *lightLevelText(LightLevel level) {
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

void drawHeader(const char *title) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void drawFooter(unsigned long now) {
  display.setTextSize(1);
  display.setCursor(0, 56);
  if (lcdReady) {
    display.print("LCD ACK 0x");
    display.print(lcdAddress, HEX);
  } else {
    display.print("LCD NO ACK R");
    display.print(lcdRetrySeconds(now));
    display.print("s");
  }
}

void drawSummary(unsigned long now) {
  const DhtState state = currentDhtState(now);
  drawHeader("SUMMARY");
  display.setCursor(0, 14);
  display.print("DHT ");
  display.print(dhtStateText(state));
  if (state == DhtState::OK) {
    display.setCursor(0, 26);
    display.print("T ");
    display.print(lastTemperature, 1);
    display.print("C  H ");
    display.print(lastHumidity, 0);
    display.print("%");
  } else {
    display.setCursor(0, 26);
    display.print("NO LIVE DHT DATA");
  }
  display.setCursor(0, 42);
  display.print("LIGHT ");
  if (lightReady) {
    display.print(lightRaw);
    display.print(" ");
    display.print(lightLevelText(classifyLight(lightRaw)));
  } else {
    display.print("WAIT");
  }
  drawFooter(now);
  display.display();
}

void drawDhtPage(unsigned long now) {
  const DhtState state = currentDhtState(now);
  drawHeader("DHT11 GPIO14");
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(dhtStateText(state));
  display.setTextSize(1);
  display.setCursor(0, 38);
  if (state == DhtState::WAIT) {
    const unsigned long elapsed = now - bootMillis;
    const unsigned long waitLeft =
        elapsed >= DHT_INTERVAL_MS
            ? 0
            : (DHT_INTERVAL_MS - elapsed + 999UL) / 1000UL;
    display.print("FIRST READ ");
    display.print(waitLeft);
    display.print("s");
  } else if (state == DhtState::OK) {
    display.print("T ");
    display.print(lastTemperature, 1);
    display.print("C H ");
    display.print(lastHumidity, 0);
    display.print("%");
  } else if (hasGoodDht) {
    display.print("LAST T ");
    display.print(lastTemperature, 1);
    display.print(" H ");
    display.print(lastHumidity, 0);
    display.print("%");
  } else {
    display.print("CHECK VCC/DATA/GND");
  }
  display.setCursor(0, 48);
  if (hasGoodDht) {
    display.print("AGE ");
    display.print(cappedDhtAgeSeconds(now));
    display.print("s ");
  }
  display.print("ERR ");
  display.print(dhtFailures);
  drawFooter(now);
  display.display();
}

void drawLightPage(unsigned long now) {
  drawHeader("LIGHT GPIO33");
  display.setCursor(0, 14);
  if (!lightReady) {
    display.setTextSize(2);
    display.print("WAIT");
  } else {
    display.print("RAW ");
    display.print(lightRaw);
    display.setTextSize(2);
    display.setCursor(0, 26);
    display.print(lightLevelText(classifyLight(lightRaw)));
    display.setTextSize(1);
    display.setCursor(0, 47);
    display.print("MIN ");
    display.print(observedLightMin);
    display.print(" MAX ");
    display.print(observedLightMax);
  }
  drawFooter(now);
  display.display();
}

void drawSystemPage(unsigned long now) {
  drawHeader("SYSTEM");
  display.setCursor(0, 14);
  display.print("OLED 0x");
  display.print(oledAddress, HEX);
  display.print(" OK");
  display.setCursor(0, 26);
  if (lcdReady) {
    display.print("LCD ACK 0x");
    display.print(lcdAddress, HEX);
  } else {
    display.print("LCD NO ACK R");
    display.print(lcdRetrySeconds(now));
    display.print("s");
  }
  display.setCursor(0, 38);
  display.print("DHT ");
  display.print(dhtStateText(currentDhtState(now)));
  display.setCursor(0, 48);
  display.print("UP ");
  display.print((now - bootMillis) / 1000UL);
  display.print("s");
  drawFooter(now);
  display.display();
}

void drawCurrentPage(unsigned long now) {
  switch (currentPage) {
    case 0:
      drawSummary(now);
      break;
    case 1:
      drawDhtPage(now);
      break;
    case 2:
      drawLightPage(now);
      break;
    default:
      drawSystemPage(now);
      break;
  }
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
  } else {
    snprintf(line0, sizeof(line0), "DHT %s", dhtStateText(state));
  }

  if (lightReady) {
    snprintf(line1, sizeof(line1), "L:%4d %s", lightRaw,
             lightLevelText(classifyLight(lightRaw)));
  } else {
    snprintf(line1, sizeof(line1), "LIGHT WAIT");
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
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  bootMillis = millis();
  lastDhtReadMillis = bootMillis;
  lastLightReadMillis = bootMillis;
  lastPageMillis = bootMillis;
  lastLcdProbeMillis = bootMillis;
  lastLcdRetryMillis = bootMillis;
  drawCurrentPage(bootMillis);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  if (now - lastDhtReadMillis >= DHT_INTERVAL_MS) {
    lastDhtReadMillis = now;
    hasReadAttempt = true;
    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();
    if (isnan(humidity) || isnan(temperature)) {
      lastDhtReadSucceeded = false;
      if (dhtFailures < DISPLAY_COUNTER_MAX) ++dhtFailures;
    } else {
      lastHumidity = humidity;
      lastTemperature = temperature;
      lastDhtGoodMillis = now;
      hasGoodDht = true;
      lastDhtReadSucceeded = true;
    }
  }

  if (now - lastLightReadMillis >= LIGHT_INTERVAL_MS) {
    lastLightReadMillis = now;
    lightRaw = readLightAverage();
    observedLightMin = min(observedLightMin, lightRaw);
    observedLightMax = max(observedLightMax, lightRaw);
    lightReady = true;
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

  const DhtState state = currentDhtState(now);
  if (isDhtFault(state)) {
    currentPage = 1;
    lastPageMillis = now;
  } else if (now - lastPageMillis >= PAGE_INTERVAL_MS) {
    lastPageMillis = now;
    currentPage = (currentPage + 1) % 4;
  }

  if (now - lastDisplayMillis < DISPLAY_INTERVAL_MS) return;
  lastDisplayMillis = now;
  drawCurrentPage(now);
  updateLcd(now);
}
