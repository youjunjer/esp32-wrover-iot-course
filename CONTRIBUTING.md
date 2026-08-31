# Contributing

## 範例規格

- 每個 Arduino Sketch 使用獨立資料夾。
- 資料夾名稱必須與主 `.ino` 檔名相同。
- 正式基準是 `esp32:esp32:esp32wrover`。
- 相機範例才能使用 `CAMERA_MODEL_AI_THINKER`。
- 範例必須加入 `config/sketches.txt` 才視為受編譯檢查保護。
- 使用外部函式庫時，在 `config/libraries.lock` 記錄名稱與已驗證版本。
- 從第二篇 OLED 章開始，所有 ESP32 執行階段訊息必須先顯示於 OLED；新增 `Serial.print*()` 訊息時要同時加入對應 OLED 訊息。
- 每個範例 README 必須列出 OLED 狀態碼、預期畫面與錯誤畫面。
- OLED 畫面不得顯示憑證；相機範例必須使用經實測、不與相機衝突的 OLED I²C 腳位。

## 憑證

- 程式只可引用 `secrets.h`。
- Repository 只可包含無真實值的 `secrets.example.h`。
- 不可提交 Wi-Fi 密碼、API Key、Token、私人 Broker 帳密或 Google Apps Script URL。

## 提交前

```bash
./scripts/check-policy.sh
./scripts/compile-all.sh
```

需要實體硬體的範例，必須在文件中另外記錄燒錄與實機驗證結果。
