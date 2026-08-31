// 使用三組 LEDC PWM 控制四腳 RGB LED。
// 顏色循環與紅燈錯誤碼都是可見輸出，不需要序列監控器。

constexpr uint8_t RED_PIN = 15;
constexpr uint8_t GREEN_PIN = 2;
constexpr uint8_t BLUE_PIN = 4;

constexpr uint32_t PWM_FREQUENCY_HZ = 5000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t PWM_MAX_DUTY = 255;
constexpr uint8_t RGB_BRIGHTNESS_LIMIT = 96;

struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

constexpr RgbColor COLORS[] = {
    {255, 0, 0},      // 紅
    {255, 255, 0},    // 黃
    {0, 255, 0},      // 綠
    {0, 255, 255},    // 青
    {0, 0, 255},      // 藍
    {255, 0, 255},    // 紫
    {255, 255, 255},  // 白
    {0, 0, 0},        // 關閉
};

bool pwmReady = false;

uint8_t toOutputDuty(uint8_t brightness) {
  const uint8_t limitedBrightness =
      (static_cast<uint16_t>(brightness) * RGB_BRIGHTNESS_LIMIT) / PWM_MAX_DUTY;
  return limitedBrightness;
}

bool setRgb(uint8_t red, uint8_t green, uint8_t blue) {
  const bool redOk = ledcWrite(RED_PIN, toOutputDuty(red));
  const bool greenOk = ledcWrite(GREEN_PIN, toOutputDuty(green));
  const bool blueOk = ledcWrite(BLUE_PIN, toOutputDuty(blue));
  return redOk && greenOk && blueOk;
}

void setDigitalRgb(bool redOn, bool greenOn, bool blueOn) {
  digitalWrite(RED_PIN, redOn ? HIGH : LOW);
  digitalWrite(GREEN_PIN, greenOn ? HIGH : LOW);
  digitalWrite(BLUE_PIN, blueOn ? HIGH : LOW);
}

void prepareErrorOutput() {
  // 解除可能已配置成功的 LEDC 腳位，再改用數位輸出顯示錯誤碼。
  ledcDetach(RED_PIN);
  ledcDetach(GREEN_PIN);
  ledcDetach(BLUE_PIN);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setDigitalRgb(false, false, false);
}

void blinkInitError() {
  // 紅燈短閃三次代表 LEDC ERR。
  for (uint8_t pulse = 0; pulse < 3; ++pulse) {
    setDigitalRgb(true, false, false);
    delay(180);
    setDigitalRgb(false, false, false);
    delay(180);
  }
  delay(1200);
}

void setup() {
  const bool redReady = ledcAttach(RED_PIN, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
  const bool greenReady = ledcAttach(GREEN_PIN, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
  const bool blueReady = ledcAttach(BLUE_PIN, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
  pwmReady = redReady && greenReady && blueReady;

  if (!pwmReady) {
    prepareErrorOutput();
    return;
  }

  // 啟動完成時先統一關燈。
  if (!setRgb(0, 0, 0)) {
    pwmReady = false;
    prepareErrorOutput();
  }
}

void loop() {
  if (!pwmReady) {
    blinkInitError();
    return;
  }

  for (const RgbColor &color : COLORS) {
    if (!setRgb(color.red, color.green, color.blue)) {
      pwmReady = false;
      prepareErrorOutput();
      return;
    }
    delay(700);
  }
}
