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
- 從 AB143 課本工作檔匯入第一批 6 張相容圖像，加入來源記錄、替代文字與章節圖說。
- 建立第二篇 OLED 前三章與 3 個範例：I²C 自動尋址、基礎繪圖與明確標示 DEMO 的診斷狀態畫面。
- 鎖定 Adafruit BusIO 1.17.4、GFX 1.12.6 與 SSD1306 2.5.17，並加入目前腳位接線圖、預期畫面及真實截圖規則。
- 加入 Commit `9934db9` 的 GitHub Actions 成功畫面與來源雜湊，圖說明確限定為 CI 編譯證據。
- 建立第二篇第 4～6 章與三個 OLED-first 範例：GPIO 14／33 輸入診斷、PIR 暖機與事件顯示、光敏 ADC 校正流程。
- 匯入三張相容的 AB143 PIR／光敏單圖，加入六張目前接線／預期畫面 SVG 與來源、雜湊、非實機證據標示。
- 加入 Commit `dad0084` 的 GitHub Actions Run 33478348736 成功畫面；鎖定工具鏈已編譯全部 14 個 Sketch，實機驗證仍獨立保留。
