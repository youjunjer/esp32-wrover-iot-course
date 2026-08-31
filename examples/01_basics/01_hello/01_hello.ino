constexpr uint8_t STATUS_LED_PIN = 2;

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  // Three short flashes show that setup completed after upload or reset.
  for (uint8_t count = 0; count < 3; ++count) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(150);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(150);
  }
}

void loop() {
  // A short heartbeat every two seconds shows that loop is still running.
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(100);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(1900);
}
