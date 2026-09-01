# 第二篇來源與遷移記錄

## 來源基準

- AB143 第三版印刷 P79：I²C 位址掃描概念。
- AB143 第四版第 3 章 P35：PIR 外觀、腳位與調整旋鈕。
- AB143 第四版第 4 章 P43：四線式光敏模組的 AO／DO 外觀。
- AB143 第四版第 6 章：DHT11、顯示器比較、I²C 與 1602 LCD；保留到後續相符章節再使用。
- `esp32-mqtt-energy-meter/04_PIR`：GPIO 14 的既有 PIR 一般模式基準；新版移除紅綠燈並加入 OLED 暖機與診斷。
- `esp32-mqtt-energy-meter/06_oled_photo` 與專案硬體表：光敏 GPIO 33 ADC 基準。
- `esp32-mqtt-energy-meter/14_ENERGY_OLED`：Adafruit SSD1306、GPIO 21／22 與課程板旋轉方向。
- `esp32-mqtt-energy-meter/20_oled_show`：目前能源專案的 OLED 狀態與頁面設計。
- `docs/oled-status-standard.md`：OLED 優先、錯誤保留、資料新鮮度與敏感資訊邊界。

## 本次現代化決策

- 使用 `Adafruit SSD1306` 與 `Adafruit GFX`，對齊能源專案主程式，不直接搬用早期 U8g2 教學範例。
- 一般模式固定 SDA GPIO 21、SCL GPIO 22；相機模式另案實測，不宣稱可直接共用。
- 先檢查 `0x3C`、`0x3D`，不以 Serial Monitor 作為唯一掃描結果。
- OLED 無法初始化時使用 GPIO 2 閃爍碼，解決「螢幕壞了卻只能在螢幕顯示錯誤」的循環問題。
- 診斷版型的固定狀態全部標示 `DEMO`，不冒充真實感測器、網路或硬體結果。
- 前六個範例先保持自足；等指定課程板完成第一次 OLED 實測後，再決定共用狀態函式介面。
- PIR 從舊版 GPIO 16／17 與錄放音整合，改為單一模組 GPIO 14；避免 Wrover PSRAM 邊界，也避免第一篇 GPIO 13 按鈕殘留接線衝突。
- 光敏 AO 從舊版 GPIO 36 改為 ADC1 的 GPIO 33，模組使用 3.3V；`analogRead()` 只稱為原始值，不稱 lux。
- Core 3.3.11 在 ADC pin 首次配置前不使用 per-pin attenuation，單一 ADC 範例改用 `analogSetAttenuation(ADC_11db)`；依據為該版本的 [ADC 實作](https://github.com/espressif/arduino-esp32/blob/3.3.11/cores/esp32/esp32-hal-adc.c)。
- PIR 使用 60 秒可見暖機與 150 ms 穩定判定；事件數不是人數。光敏預設 `UNCAL`，本機校正後才允許顯示明暗分級。

## 圖像決策

AB143 第四版只有低解析 OLED 商品小圖，沒有符合目前 CLI、Core 3.3.11、Wrover GPIO 21／22 與 OLED 優先診斷流程的操作圖，因此前三章不直接匯入舊 OLED 圖。

本篇使用目前腳位重畫的接線圖、依程式版型產生且明確標示的預期畫面，以及 Commit `9934db9`、`dad0084` 的真實 GitHub Actions 編譯截圖。第 4～6 章另匯入三張相容的 PIR／光敏單圖並建立 `docs/assets/ab143/part2/SOURCES.md`；舊 WROOM 接線與 Serial-only 畫面不沿用。後續進入 1602 LCD 時，再評估 AB143 第四版第 6 章的 I²C 背板、行列座標與完成畫面單圖。

不納入舊 Arduino IDE、Library Manager、Serial Monitor、WROOM 板型、5V I²C 直連及來源不明商品圖。各教學圖的來源、用途與雜湊值記錄在圖檔同目錄的 `SOURCES.md`。
