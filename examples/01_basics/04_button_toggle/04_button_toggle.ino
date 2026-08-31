constexpr uint8_t LED_PIN = 2;
constexpr uint8_t BUTTON_PIN = 13;
constexpr uint32_t DEBOUNCE_MS = 30;

bool ledOn = false;
int lastRawButtonState = HIGH;
int stableButtonState = HIGH;
uint32_t rawStateChangedAt = 0;

void setup() {
  // 啟動時先將輸出設為安全的熄燈狀態。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 外接按鈕只需連接 GPIO 13 與 GND。
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 以上電後的實際讀值當作初始狀態，避免重啟就誤切換。
  lastRawButtonState = digitalRead(BUTTON_PIN);
  stableButtonState = lastRawButtonState;
  rawStateChangedAt = millis();
}

void loop() {
  const int rawButtonState = digitalRead(BUTTON_PIN);

  // 只要原始讀值改變，就重新計算穩定時間。
  if (rawButtonState != lastRawButtonState) {
    lastRawButtonState = rawButtonState;
    rawStateChangedAt = millis();
  }

  // 讀值連續穩定 30 ms 才承認狀態改變，濾除機械接點彈跳。
  if (rawButtonState != stableButtonState &&
      millis() - rawStateChangedAt >= DEBOUNCE_MS) {
    stableButtonState = rawButtonState;

    // 只在「確認按下」的瞬間切換一次，長按不會連續切換。
    if (stableButtonState == LOW) {
      ledOn = !ledOn;
      digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    }
  }

  delay(1);
}
