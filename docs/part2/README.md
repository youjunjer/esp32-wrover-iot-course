# 第二篇：OLED、感測器與顯示器

第二篇先建立不依賴序列監控器的 OLED 狀態介面，再逐項加入感測器與 1602 LCD。從本篇開始，啟動、正常、失敗、重試、無資料與資料過期都必須優先顯示於 OLED。

## 目前教材範圍

1. [I²C 與 OLED 啟動自測](01_oled_self_test.md)
2. [OLED 基礎顯示](02_oled_basics.md)
3. [OLED 可視化診斷](03_oled_diagnostics.md)
4. [感測器接線與輸入診斷](04_sensor_wiring_basics.md)
5. [PIR 人體感測器與 OLED](05_pir_sensor.md)
6. [光敏電阻、ADC 校正與 OLED](06_light_sensor.md)

第 4～6 章教材與自足式 Sketch 可以先完成 CI；實際上課仍應先讓第 1～3 章在指定課程板完成 OLED 實機驗收，再接入感測器。1602 LCD 與其他感測器依課程地圖後續加入，不將舊 AB143 的 WROOM 接線、5V I²C、GPIO 16／17、舊 IDE 或 Serial-only 畫面直接搬入。

## 統一硬體基準

- 開發板：ESP32 Wrover Module
- FQBN：`esp32:esp32:esp32wrover`
- OLED：SSD1306，128×64，I²C
- 一般模式 SDA：GPIO 21
- 一般模式 SCL：GPIO 22
- 優先供電：3.3V
- 常見位址：`0x3C`、`0x3D`
- 課程板安裝方向基準：`setRotation(2)`；若學生模組方向不同，再依實物調整

## 教學圖像的證據層級

| 圖像類型 | 可以證明 | 不可以證明 |
|---|---|---|
| 接線／預期畫面示意圖 | 目前教材的腳位與預期版型 | 實體接線或 OLED 已成功 |
| 真實 GitHub Actions／CLI 截圖 | 指定 Commit 已編譯通過 | 已燒錄、已接線或硬體可用 |
| 實體接線與 OLED 照片 | 照片所記錄硬體工作階段的可見結果 | 其他板子或其他接線也必然相同 |

預期畫面示意圖會直接標示不是實機照片；實機照片尚未取得時，教材保留「待實機截圖」，不製造假的燒錄、位址或感測數據。

## 本階段完成條件

- 前六個 Sketch 使用鎖定版本通過 GitHub Actions 編譯。
- 每個範例都有接線、畫面、錯誤碼、預期結果與失敗回報格式。
- 真實 CI 截圖需記錄 Run、Commit、版本與 `CI compile only` 邊界。
- 實體 OLED、GPIO 2 閃爍碼、GPIO 14 PIR、GPIO 33 ADC 與完整接線仍需另行拍照驗證。

## 目前驗證狀態

- 2026-09-01：[GitHub Actions Run 33478348736](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33478348736) 使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11、`esp32:esp32:esp32wrover`、Adafruit BusIO 1.17.4、GFX 1.12.6 與 SSD1306 2.5.17，編譯通過第一篇 8 個與第二篇 6 個，共 14 個 Sketch。
- 真實 Run Summary 截圖已放入第一章與第四章；其證據範圍只到 CI 編譯成功。
- 實體板燒錄、OLED 位址、畫面方向、接線、PIR／光敏數值與 GPIO 2 閃爍碼尚未驗證。

教材來源與現代化決策見 [第二篇來源與遷移記錄](source-map.md)，共通訊息規格見 [OLED 執行狀態與除錯訊息規範](../oled-status-standard.md)。
