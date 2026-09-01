# 第一篇：ESP32 與 Arduino C 基礎

第一篇建立後續課程共同使用的程式觀念與硬體操作習慣。所有結果都能直接從 LED、按鈕、紅綠燈、RGB LED 或 WS2812 觀察，不需要序列監控器。

## LED 接線先說明

本課程為簡化實作，後續接線表會直接標示 GPIO 連接 LED，不再逐例畫出外接電阻。若使用裸的一般 LED 或 RGB LED，請在每個發光通道與 GPIO 之間各串接 220～330Ω；只有已確認內建保護的板載燈或燈組才依其原有接法。

## 學習目標

完成第一篇後，學生能夠：

1. 說明 ESP32 Wrover 的用途、3.3V 邏輯與主要 GPIO 限制。
2. 使用 `esp32:esp32:esp32wrover` 編譯及燒錄範例。
3. 解釋 `setup()`、`loop()`、變數、判斷、迴圈與函式。
4. 使用 `pinMode()`、`digitalRead()` 與 `digitalWrite()`。
5. 使用 `INPUT_PULLUP` 與軟體防彈跳讀取按鈕。
6. 使用 ESP32 Core 3.x LEDC API 控制 PWM 亮度與 RGB 色彩。
7. 使用 WS2812 單顆全彩 LED，並限制亮度避免過大電流。

## 章節

1. [認識物聯網與 ESP32 Wrover](01_iot_and_wrover.md)
2. [開發工具與第一支程式](02_toolchain.md)
3. [Arduino C 程式結構](03_program_structure.md)
4. [變數、資料型態與運算](04_data_and_operators.md)
5. [判斷、迴圈與函式](05_control_and_functions.md)
6. [數位輸入與輸出](06_digital_io.md)
7. [PWM、RGB LED 與 WS2812](07_pwm_and_color_leds.md)

教材版本差異、來源範例對應與現代化決策另見 [第一篇來源與遷移記錄](source-map.md)。

第一篇已將相容的 AB143 課本圖像放入數位輸出、PWM 與 RGB 章節；各圖的版本、書頁、原始檔名及轉換方式見 [第一篇圖像來源記錄](../assets/ab143/part1/SOURCES.md)。舊工具畫面、錯誤腳位、不同板型及來源不明的第三方圖片不直接沿用。

## 材料

- ESP32 Wrover 相容課程板與可傳輸資料的 USB 線。
- 麵包板與杜邦線。
- 紅、黃、綠及其他顏色 LED。
- 按鈕一顆。
- 共陰極 RGB LED 一顆。
- WS2812 單顆模組或課程板上的 WS2812。

## 範例與可見驗收

| 範例 | 重點 | 不使用序列監控的完成判定 |
|---|---|---|
| `01_hello` | 編譯、燒錄、`setup()`、`loop()` | GPIO 2 快閃三次後持續顯示心跳 |
| `02_led` | 數位輸出 | LED 規律亮滅 |
| `03_button_led` | `INPUT_PULLUP`、數位輸入 | 按住按鈕時 LED 改變狀態 |
| `04_button_toggle` | 邊緣偵測、防彈跳、布林狀態 | 每按一次切換 LED 開關 |
| `05_traffic_light` | 函式、流程與多輸出 | 綠、黃、紅依指定時間循環 |
| `06_pwm_fade` | LEDC PWM、`for` 迴圈 | LED 平滑漸亮、漸暗 |
| `07_rgb_led` | 三色 PWM 與色彩混合 | RGB LED 依序顯示指定色彩 |
| `08_ws2812` | 可定址全彩 LED 與函式庫 | WS2812 依序顯示白、紅、綠、藍並關閉 |

第一篇不安排需要讀取數值的感測器。光敏電阻、MQ-2、DHT11 等類比或感測內容會在第二篇 OLED 訊息介面完成後進行。

## 完成條件

- 所有範例以 `esp32:esp32:esp32wrover` 及鎖定版本通過 GitHub Actions 編譯。
- 程式不使用 Serial-only 訊息。
- 每個範例都附接線、預期結果、常見錯誤與安全提醒。
- 實體板驗證需另外記錄；僅通過編譯不能宣稱硬體已驗證。

## 目前驗證狀態

- 2026-09-01：[GitHub Actions #33413746238](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33413746238) 使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11、`esp32:esp32:esp32wrover` 與 `Adafruit NeoPixel` 1.15.5，編譯通過全部 8 個 Sketch。
- 實體板燒錄、接線與可見結果尚未驗證，不與 CI 編譯結果混為一談。
