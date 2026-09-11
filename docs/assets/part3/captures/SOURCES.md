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

## `github-actions-part3-complete-success.png`

- 來源頁面：[GitHub Actions Run 34484466016](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/34484466016)
- Repository：`youjunjer/esp32-wrover-iot-course`（公開）
- 對應 Commit：`01435e8fd49b3a63b5c68ce1b02d723aefc87c48`
- 擷取時間：2026-09-10 22:02 CST（Asia/Taipei）
- 擷取方式：Codex 使用本機 Google Chrome 的 Playwright headless 畫面擷取；1440×1100 PNG，未重繪頁面內容
- 畫面可見證據：Workflow `Compile Arduino examples`、Run `#17`、`compile succeeded`、總時間 `10m 13s`，且所有步驟顯示完成
- 工作流程記錄：Arduino CLI 1.5.1、ESP32 Core 3.3.11、FQBN `esp32:esp32:esp32wrover`、`config/libraries.lock` 鎖定函式庫；記錄中共有 31 行 `Compiling examples/`
- 處理：使用公開、未登入的 GitHub 頁面；畫面沒有帳密、內網資訊或本機路徑，因此未遮蔽與裁切
- SHA-256：`04d94fabc348e875185a28ac0c4acc3178a8bad2f39a81cdc306347617550fb9`

證據邊界：本圖與 Run 記錄只證明 Commit `01435e8` 的 31 個 Sketch 在 GitHub Actions 以鎖定工具鏈完成編譯。它不證明已燒錄到實體板，也不證明 ThingSpeak、教師既有 GAS、MQTT Broker／ACL／TLS、OLED、SG90 或繼電器已完成實測。
