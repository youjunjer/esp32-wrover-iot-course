# 1. 認識物聯網與 ESP32 Wrover

## 物聯網的基本資料流

物聯網系統通常包含四層：

1. 裝置：感測環境或控制設備。
2. 通訊：使用 Wi-Fi、Bluetooth、BLE 或 MQTT 傳遞資料。
3. 服務：儲存、轉換及分析資料。
4. 介面：以 OLED、網頁或 Dashboard 呈現狀態。

第一篇先完成裝置層。後續篇章才逐步加入 OLED、網路、雲端、Node-RED、相機與能源監測。

## 本課程的開發板基準

- Arduino IDE 開發板：`ESP32 Wrover Module`
- Arduino CLI FQBN：`esp32:esp32:esp32wrover`
- 邏輯電壓：3.3V
- 相機章仍使用相同 Wrover 板型，只在程式中選擇 AI Thinker 相機腳位。

ESP32 GPIO 不應直接承受 5V 訊號。連接外部模組前，先確認供電、輸出電壓與是否需要分壓或準位轉換。

## 模組與開發板不同

ESP32 Wrover 模組整合 ESP32 晶片、Flash、PSRAM 與天線；開發板則再加上 USB-to-UART、供電、重啟／BOOT 鍵與引出腳位。在 Arduino IDE 選擇 `ESP32 Wrover Module` 是指編譯參數，不代表每塊板子的 USB 晶片、板載 LED 或接頭位置完全相同。

本課程使用的經典 ESP32 可提供 2.4 GHz Wi-Fi、Bluetooth Classic 與 BLE，Wrover 版型的 PSRAM 也會在後續相機章使用。因為 PSRAM 與相機會佔用特定腳位，不能把所有標示為 GPIO 的腳都視為可任意分配。一般模式與 AI Thinker 相機模式的界線另見 [`docs/hardware-pin-modes.md`](../hardware-pin-modes.md)。

## 第一篇腳位

| 用途 | GPIO | 說明 |
|---|---:|---|
| 狀態 LED | 2 | 使用板載 LED，或將 LED 接到 GPIO 2 |
| 外接按鈕 | 13 | 按鈕接 GPIO 13 與 GND，使用 `INPUT_PULLUP` |
| 紅燈 | 4 | 紅綠燈範例使用 |
| 黃燈 | 2 | 與狀態 LED 共用，範例不會同時執行 |
| 綠燈／PWM LED／RGB 紅色 | 15 | 啟動期間不可被外部電路強制到錯誤電位 |
| RGB 綠色 | 2 | 與狀態 LED 共用，範例不會同時執行 |
| RGB 藍色 | 4 | 與紅綠燈紅燈腳位共用，範例不會同時執行 |
| WS2812 資料 | 32 | 一般模式使用；相機模式會占用此腳位 |

每個 Arduino Sketch 都是獨立範例，不代表上表所有元件可以同時接上並執行。

## 安全原則

- 接線或更換元件前先拔除 USB 或外部電源。
- 不將 5V 訊號直接接到 ESP32 GPIO。
- WS2812 與 ESP32 必須共地，並限制第一個測試的亮度。
- GPIO 2、4、15 與 32 在其他模式可能有不同用途，切換章節時重新核對接線。

## 學習檢查

學生應能回答：為何本課程統一使用 Wrover FQBN、GPIO 為何不能直接輸入 5V，以及一般模式與相機模式為何需要不同接線。
