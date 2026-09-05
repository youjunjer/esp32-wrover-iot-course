#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int TRIG_PIN = 25;
constexpr int ECHO_PIN = 14;
constexpr int BUZZER_PIN = 13;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr uint32_t BUZZER_ATTACH_HZ = 1000;
constexpr uint8_t BUZZER_RESOLUTION_BITS = 8;
constexpr unsigned long ECHO_TIMEOUT_US = 30000UL;
constexpr unsigned long MEASUREMENT_INTERVAL_MS = 250UL;
constexpr float SOUND_CM_PER_US = 0.0343F;
constexpr float MIN_DISTANCE_CM = 2.0F;
constexpr float MAX_DISTANCE_CM = 400.0F;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

enum class DistanceStatus { OK, TIMEOUT, RANGE_ERR };

struct DistanceReading {
  DistanceStatus status;
  float distanceCm;
};

struct RadarOutput {
  const char *state;
  uint32_t frequencyHz;
};

uint8_t errorCode = 0;
bool oledReady = false;
bool buzzerReady = false;
unsigned long lastMeasurementMs = 0;

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void forceBuzzerPinLow() {
  if (buzzerReady) {
    ledcDetach(BUZZER_PIN);
  }
  buzzerReady = false;
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

bool silenceBuzzer() {
  if (!buzzerReady) {
    forceBuzzerPinLow();
    return true;
  }
  if (ledcWrite(BUZZER_PIN, 0)) return true;

  // 若 LEDC 無法確認 duty=0，立即解除 LEDC 並退回 GPIO LOW。
  forceBuzzerPinLow();
  return false;
}

void blinkErrorCode(uint8_t code) {
  silenceBuzzer();
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
  if (echoUs == 0) return {DistanceStatus::TIMEOUT, 0.0F};

  const float distanceCm = echoUs * SOUND_CM_PER_US / 2.0F;
  if (distanceCm < MIN_DISTANCE_CM || distanceCm > MAX_DISTANCE_CM) {
    return {DistanceStatus::RANGE_ERR, distanceCm};
  }
  return {DistanceStatus::OK, distanceCm};
}

RadarOutput chooseRadarOutput(float distanceCm) {
  if (distanceCm < 15.0F) return {"DANGER", 2000};
  if (distanceCm < 50.0F) return {"WARNING", 1000};
  if (distanceCm < 100.0F) return {"CAUTION", 500};
  return {"SAFE", 0};
}

bool applyFrequency(uint32_t frequencyHz) {
  if (!buzzerReady) return false;
  if (frequencyHz == 0) return silenceBuzzer();
  if (ledcWriteTone(BUZZER_PIN, frequencyHz) != 0) return true;

  forceBuzzerPinLow();
  return false;
}

void drawRadar(const char *state, const char *detail, const char *footer) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PARKING RADAR");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(state);
  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print(detail);
  display.setCursor(0, 54);
  display.print(footer);
  display.display();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  // 在 LEDC 附加之前先保證蜂鳴器是靜音狀態。
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  buzzerReady =
      ledcAttach(BUZZER_PIN, BUZZER_ATTACH_HZ, BUZZER_RESOLUTION_BITS);
  if (buzzerReady) silenceBuzzer();

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
  if (!buzzerReady) {
    drawRadar("BUZZER ERR", "LEDC ATTACH FAIL", "SAFE OFF");
    return;
  }
  drawRadar("INIT", "T25 E14 B13", "SAFE OFF");
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }
  if (!buzzerReady) {
    forceBuzzerPinLow();
    return;
  }

  const unsigned long now = millis();
  if (now - lastMeasurementMs < MEASUREMENT_INTERVAL_MS) return;
  lastMeasurementMs = now;

  const DistanceReading reading = measureDistance();
  if (reading.status != DistanceStatus::OK) {
    if (!silenceBuzzer()) {
      drawRadar("BUZZER ERR", "SILENCE FAIL", "GPIO LOW");
      return;
    }
    drawRadar(reading.status == DistanceStatus::TIMEOUT ? "TIMEOUT"
                                                        : "RANGE ERR",
              "NO VALID DIST", "SAFE OFF");
    return;
  }

  const RadarOutput output = chooseRadarOutput(reading.distanceCm);
  if (!applyFrequency(output.frequencyHz)) {
    drawRadar("BUZZER ERR", "TONE WRITE FAIL", "SAFE OFF");
    return;
  }

  char detail[20];
  char footer[20];
  snprintf(detail, sizeof(detail), "DIST %.1f cm", reading.distanceCm);
  if (output.frequencyHz == 0) {
    snprintf(footer, sizeof(footer), "SILENT");
  } else {
    snprintf(footer, sizeof(footer), "TONE %lu Hz",
             static_cast<unsigned long>(output.frequencyHz));
  }
  drawRadar(output.state, detail, footer);
}
