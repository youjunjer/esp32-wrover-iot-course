# Changelog

## Unreleased

- 建立 AB143 ESP32 Wrover 物聯網與能源監測教材的 GitHub 骨架。
- 統一 Arduino CLI FQBN 為 `esp32:esp32:esp32wrover`。
- 建立六篇教材分類、安全邊界與交接文件。
- 加入兩個 GPIO 2 可見狀態範例；初始工具鏈驗證不再依賴序列監控。
- 調整第二篇為 OLED 先行，所有後續 ESP32 執行階段訊息以 OLED 為主要介面，Serial 僅作同步副本。
- 加入感測器、網路、MQTT、相機與能源監測的 OLED 狀態及錯誤碼規範。
- 完成第一篇 7 章課文與 8 個可見輸出範例，包含按鈕防彈跳、紅綠燈、新版 LEDC PWM、RGB LED 與 WS2812。
- 統一 AB143 第一篇的腳位與時序，記錄舊版 RGB／PWM 文字矛盾，移除 Serial-only 與舊版 PWM API。
- 將 `Adafruit NeoPixel` 鎖定為 1.15.5，並將全部第一篇 Sketch 納入 GitHub Actions 編譯清單。
- 第一篇 LED 接線注意事項集中於首頁，後續章節與範例統一採簡化接線圖。
