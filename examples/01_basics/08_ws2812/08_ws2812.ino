#include <Adafruit_NeoPixel.h>

// 一顆 WS2812 的資料輸入腳位延續既有能源監測專案，使用 GPIO 32。
constexpr uint8_t WS2812_PIN = 32;
constexpr uint16_t WS2812_COUNT = 1;
constexpr uint8_t WS2812_BRIGHTNESS = 32;
constexpr uint16_t COLOR_DURATION_MS = 500;

Adafruit_NeoPixel pixels(WS2812_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);

void showColor(uint8_t red, uint8_t green, uint8_t blue, uint16_t durationMs) {
  pixels.setPixelColor(0, pixels.Color(red, green, blue));
  pixels.show();
  delay(durationMs);
}

void showBootPattern() {
  // 三次白色短閃表示程式已進入 setup() 並送出 WS2812 資料。
  for (uint8_t pulse = 0; pulse < 3; ++pulse) {
    showColor(255, 255, 255, 120);
    showColor(0, 0, 0, 120);
  }
  delay(500);
}

void setup() {
  pixels.begin();
  pixels.setBrightness(WS2812_BRIGHTNESS);
  pixels.clear();
  pixels.show();
  showBootPattern();
}

void loop() {
  showColor(255, 255, 255, COLOR_DURATION_MS);  // 白
  showColor(255, 0, 0, COLOR_DURATION_MS);      // 紅
  showColor(0, 255, 0, COLOR_DURATION_MS);      // 綠
  showColor(0, 0, 255, COLOR_DURATION_MS);      // 藍
  showColor(0, 0, 0, COLOR_DURATION_MS);        // 關閉
}
