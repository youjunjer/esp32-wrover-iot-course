// 使用 ESP32 Core 3.x 的新版 LEDC API，讓 LED 反覆漸亮、漸暗。
// 本範例不依賴序列監控器，LED 動作就是程式的可見執行狀態。

constexpr uint8_t PWM_LED_PIN = 15;
constexpr uint32_t PWM_FREQUENCY_HZ = 5000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint16_t PWM_MAX_DUTY = (1U << PWM_RESOLUTION_BITS) - 1U;
constexpr uint16_t PWM_STEP_DELAY_MS = 10;
constexpr uint16_t PWM_END_PAUSE_MS = 1000;

bool pwmReady = false;

void blinkErrorCode(uint8_t pulseCount) {
  // LEDC 初始化或寫入失敗時，重複閃爍指定次數作為可見錯誤碼。
  for (uint8_t pulse = 0; pulse < pulseCount; ++pulse) {
    digitalWrite(PWM_LED_PIN, HIGH);
    delay(180);
    digitalWrite(PWM_LED_PIN, LOW);
    delay(180);
  }
  delay(1200);
}

bool writeDuty(uint16_t duty) {
  if (ledcWrite(PWM_LED_PIN, duty)) {
    return true;
  }

  pwmReady = false;
  ledcDetach(PWM_LED_PIN);
  pinMode(PWM_LED_PIN, OUTPUT);
  digitalWrite(PWM_LED_PIN, LOW);
  return false;
}

void setup() {
  // Core 3.x 會自動分配 LEDC channel；本教材不使用舊版 API。
  pwmReady = ledcAttach(PWM_LED_PIN, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);

  if (!pwmReady) {
    pinMode(PWM_LED_PIN, OUTPUT);
    digitalWrite(PWM_LED_PIN, LOW);
  }
}

void loop() {
  if (!pwmReady) {
    // 三次短閃代表 LEDC ERR。
    blinkErrorCode(3);
    return;
  }

  // 亮度由 0 緩慢增加到 255。
  for (uint16_t duty = 0; duty <= PWM_MAX_DUTY; ++duty) {
    if (!writeDuty(duty)) {
      return;
    }
    delay(PWM_STEP_DELAY_MS);
  }
  delay(PWM_END_PAUSE_MS);

  // 亮度由 255 緩慢降低到 0。
  for (int16_t duty = PWM_MAX_DUTY; duty >= 0; --duty) {
    if (!writeDuty(static_cast<uint16_t>(duty))) {
      return;
    }
    delay(PWM_STEP_DELAY_MS);
  }
  delay(PWM_END_PAUSE_MS);
}
