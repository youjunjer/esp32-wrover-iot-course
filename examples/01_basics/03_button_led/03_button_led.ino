constexpr uint8_t LED_PIN = 2;
constexpr uint8_t BUTTON_PIN = 13;

void setup() {
  // 啟動時先關閉 LED，避免重啟時出現意外亮燈。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 使用內建上拉電阻：放開為 HIGH，按下並接地後為 LOW。
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  const bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

  // 按住按鈕時亮燈，放開後立即熄燈。
  digitalWrite(LED_PIN, buttonPressed ? HIGH : LOW);

  // 短暫延遲可降低不必要的重複讀取，也不會影響操作感受。
  delay(10);
}
