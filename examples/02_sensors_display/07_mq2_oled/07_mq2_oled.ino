#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int MQ2_PIN = 33;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long MQ2_WARMUP_MS = 60000UL;
constexpr unsigned long DISPLAY_INTERVAL_MS = 300UL;
constexpr size_t SAMPLE_COUNT = 8;

// 必須先以手邊的 MQ-2 完成基準校正，才能改為 true。
// 下列數字不是 ppm，也不是氣體安全標準。
constexpr bool CALIBRATION_READY = false;
constexpr bool RAW_RISES_WITH_GAS = true;
constexpr int LOCAL_BASELINE = 1000;
constexpr int REL_WARN_DELTA = 300;
constexpr int HIGH_DELTA = 700;
constexpr int ADC_LOW_LIMIT = 5;
constexpr int ADC_HIGH_LIMIT = 4090;
constexpr uint8_t ADC_FAULT_CONFIRM_SAMPLES = 3;
constexpr uint8_t ADC_RECOVERY_CONFIRM_SAMPLES = 3;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

enum class GasLevel {
  ADC_CHECK,
  ADC_ERR,
  UNCAL,
  BASELINE,
  REL_WARN,
  REL_HIGH
};

uint8_t errorCode = 0;
bool oledReady = false;
unsigned long warmupStartedMs = 0;
unsigned long lastDisplayMs = 0;
int observedMin = 4095;
int observedMax = 0;
uint8_t invalidAdcSamples = 0;
uint8_t validRecoverySamples = 0;
bool adcFault = false;

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
    total += analogRead(MQ2_PIN);
    delay(2);
  }
  return static_cast<int>(total / SAMPLE_COUNT);
}

bool adcSampleOutOfRange(int raw) {
  return raw <= ADC_LOW_LIMIT || raw >= ADC_HIGH_LIMIT;
}

void updateAdcHealth(int raw) {
  if (adcSampleOutOfRange(raw)) {
    validRecoverySamples = 0;
    if (invalidAdcSamples < ADC_FAULT_CONFIRM_SAMPLES) ++invalidAdcSamples;
    if (invalidAdcSamples >= ADC_FAULT_CONFIRM_SAMPLES) adcFault = true;
    return;
  }

  invalidAdcSamples = 0;
  if (!adcFault) {
    validRecoverySamples = 0;
    return;
  }

  if (validRecoverySamples < ADC_RECOVERY_CONFIRM_SAMPLES) {
    ++validRecoverySamples;
  }
  if (validRecoverySamples >= ADC_RECOVERY_CONFIRM_SAMPLES) {
    adcFault = false;
    validRecoverySamples = 0;
  }
}

GasLevel classifyGas(int raw) {
  if (adcFault) return GasLevel::ADC_ERR;
  if (invalidAdcSamples > 0) return GasLevel::ADC_CHECK;
  if (!CALIBRATION_READY) return GasLevel::UNCAL;

  const int signedChange = RAW_RISES_WITH_GAS
                               ? raw - LOCAL_BASELINE
                               : LOCAL_BASELINE - raw;
  const int increase = max(0, signedChange);
  if (increase >= HIGH_DELTA) return GasLevel::REL_HIGH;
  if (increase >= REL_WARN_DELTA) return GasLevel::REL_WARN;
  return GasLevel::BASELINE;
}

const char *levelText(GasLevel level) {
  switch (level) {
    case GasLevel::ADC_CHECK:
      return "ADC CHECK";
    case GasLevel::ADC_ERR:
      return "ADC ERR";
    case GasLevel::BASELINE:
      return "BASELINE";
    case GasLevel::REL_WARN:
      return "REL WARN";
    case GasLevel::REL_HIGH:
      return "REL HIGH";
    default:
      return "UNCAL";
  }
}

void drawWarmup(unsigned long secondsLeft) {
  char detail[20];
  snprintf(detail, sizeof(detail), "LEFT %lus", secondsLeft);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MQ-2 GPIO33");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print("WARMUP");
  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print(detail);
  display.setCursor(0, 55);
  display.print("STARTUP ONLY");
  display.display();
}

void drawInit() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MQ-2 GPIO33");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("INIT");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("ADC / NO PPM");
  display.display();
}

void drawGas(int raw, GasLevel level) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MQ-2 GPIO33");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.print("RAW ");
  display.print(raw);
  display.print(" NO PPM");
  display.setTextSize(2);
  display.setCursor(0, 27);
  display.print(levelText(level));
  display.setTextSize(1);
  display.setCursor(0, 51);
  if (level == GasLevel::ADC_CHECK || level == GasLevel::ADC_ERR) {
    display.print("CHECK AO / DIVIDER");
  } else {
    display.print("MIN ");
    display.print(observedMin);
    display.print(" MAX ");
    display.print(observedMax);
  }
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(MQ2_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

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
  delay(600);
  warmupStartedMs = millis();
  drawWarmup(MQ2_WARMUP_MS / 1000UL);
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }

  const unsigned long now = millis();
  if (now - lastDisplayMs < DISPLAY_INTERVAL_MS) return;
  lastDisplayMs = now;

  const unsigned long warmupElapsed = now - warmupStartedMs;
  if (warmupElapsed < MQ2_WARMUP_MS) {
    const unsigned long secondsLeft =
        (MQ2_WARMUP_MS - warmupElapsed + 999UL) / 1000UL;
    drawWarmup(secondsLeft);
    return;
  }

  const int raw = readAverage();
  updateAdcHealth(raw);
  if (!adcSampleOutOfRange(raw) && !adcFault) {
    observedMin = min(observedMin, raw);
    observedMax = max(observedMax, raw);
  }
  drawGas(raw, classifyGas(raw));
}
