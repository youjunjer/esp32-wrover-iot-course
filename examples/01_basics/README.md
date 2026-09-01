# 第一篇：ESP32 與 Arduino C 基礎

正式範例：

- [`01_hello`](01_hello/README.md)：GPIO 2 快閃三次後顯示心跳，驗證編譯、燒錄與主迴圈。
- [`02_led`](02_led/README.md)：GPIO 2 數位輸出，亮暗各 1 秒。
- [`03_button_led`](03_button_led/README.md)：使用 GPIO 13 外接按鈕與 `INPUT_PULLUP`。
- [`04_button_toggle`](04_button_toggle/README.md)：每次按下切換 LED，並加入 30 ms 防彈跳。
- [`05_traffic_light`](05_traffic_light/README.md)：紅 4／黃 2／綠 15 多輸出與函式。
- [`06_pwm_fade`](06_pwm_fade/README.md)：使用 Core 3.x LEDC API 讓 GPIO 15 LED 漸亮漸暗。
- [`07_rgb_led`](07_rgb_led/README.md)：使用 R15／G2／B4 三路 PWM 混色。
- [`08_ws2812`](08_ws2812/README.md)：使用 GPIO 32 控制單顆 WS2812。

每個範例都是獨立 Sketch，只接當前範例需要的元件。如果指定課程板沒有 GPIO 2 板載 LED，請依第一篇首頁的課程接線方式接上 LED。第一篇只使用可直接觀察的硬體動作，不保留 Serial-only 訊息；第二篇會先建立 OLED 訊息介面，再開始感測器與網路章節。

完整章節、材料清單與可見驗收表請見 [`docs/part1/README.md`](../../docs/part1/README.md)。
