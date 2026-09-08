# 第三篇真實操作截圖來源

本資料夾只保存可追溯的真實操作證據。預期 OLED 畫面另存於 `../guides/`，不可把示意圖誤標為實機結果。

## `github-actions-part3-foundations-success.jpg`

- 來源頁面：[GitHub Actions Run 34241431791](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/34241431791)
- Repository：`youjunjer/esp32-wrover-iot-course`（公開）
- 對應 Commit：`d53bbcaa0168b98e3c8a2e5df2ab688abfdb6135`
- 擷取時間：2026-09-08 23:16 CST（Asia/Taipei）
- 擷取方式：Codex in-app browser；保存瀏覽器回傳的原始 JPEG 位元資料，未重繪或變造頁面內容
- 畫面可見證據：Workflow `Compile Arduino examples`、Run `#14`、狀態 `Success`、總時間 `7m 16s`、`compile` Job `7m 12s`
- 工作流程記錄：Arduino CLI 1.5.1、ESP32 Core 3.3.11、FQBN `esp32:esp32:esp32wrover`，並依 `config/libraries.lock` 安裝函式庫
- SHA-256：`326a8b37790b766085382aa98808df3de40fed3fe966cb646cb90d4ed1868bd8`

證據邊界：本圖只證明該 Commit 的 25 個 Sketch 通過 CI 編譯。它不能證明已燒錄至實體板，也不能證明 OLED、Wi-Fi、NTP、TLS、Open-Meteo、WebServer、瀏覽器控制或 GPIO 輸出已完成實測。
