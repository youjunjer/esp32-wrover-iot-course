void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Wrover upload OK");
}

void loop() {
  Serial.println("ESP32 Wrover running");
  delay(1000);
}
