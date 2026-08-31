constexpr uint8_t RED_LED_PIN = 4;
constexpr uint8_t YELLOW_LED_PIN = 2;
constexpr uint8_t GREEN_LED_PIN = 15;

constexpr uint32_t RED_DURATION_MS = 3000;
constexpr uint32_t GREEN_DURATION_MS = 5000;
constexpr uint32_t YELLOW_DURATION_MS = 1000;

void setTrafficLights(bool redOn, bool yellowOn, bool greenOn) {
  // 轉換前先關閉全部 LED，避免更新腳位時短暫顯示錯誤燈號。
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  digitalWrite(RED_LED_PIN, redOn ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellowOn ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, greenOn ? HIGH : LOW);
}

void setup() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // 啟動完成前不亮任何燈，作為安全的預設狀態。
  setTrafficLights(false, false, false);
}

void loop() {
  // 以紅燈作為安全起點；各色持續時間沿用 AB143 範例。
  setTrafficLights(true, false, false);
  delay(RED_DURATION_MS);

  // 綠燈：通行。
  setTrafficLights(false, false, true);
  delay(GREEN_DURATION_MS);

  // 黃燈：提醒即將回到紅燈。
  setTrafficLights(false, true, false);
  delay(YELLOW_DURATION_MS);
}
