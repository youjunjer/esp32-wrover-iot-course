# 第二篇：OLED、感測器與顯示器

第二篇先建立不依賴序列監控器的 OLED 狀態介面，再逐項加入感測器與 1602 LCD。從本篇開始，啟動、正常、失敗、重試、無資料與資料過期都必須優先顯示於 OLED。

## 目前完成範圍

1. [I²C 與 OLED 啟動自測](01_oled_self_test.md)
2. [OLED 基礎顯示](02_oled_basics.md)
3. [OLED 可視化診斷](03_oled_diagnostics.md)

感測器與 1602 LCD 會在上述三個 OLED 範例完成指定課程板實機驗證後繼續加入。現在先保留課程地圖中的順序，不將舊 AB143 的 WROOM 接線、5V I²C、舊 IDE 或 Serial-only 畫面直接搬入。

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

## 完成條件

- 三個 OLED Sketch 使用鎖定版本通過 GitHub Actions 編譯。
- 每個範例都有接線、畫面、錯誤碼、預期結果與失敗回報格式。
- 真實 CI 截圖需記錄 Run、Commit、版本與 `CI compile only` 邊界。
- 實體 OLED、GPIO 2 閃爍碼及完整接線仍需另行拍照驗證。

教材來源與現代化決策見 [第二篇來源與遷移記錄](source-map.md)，共通訊息規格見 [OLED 執行狀態與除錯訊息規範](../oled-status-standard.md)。
