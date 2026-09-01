# AGENTS.md

本檔案是未來 Codex 或其他開發者進入專案時的首要操作規則。

## 工作前必讀

1. `README.md`
2. `docs/CONVERSATION_HANDOFF.md`
3. `docs/course-map.md`
4. `docs/PROJECT_LAYOUT.md`
5. `docs/environment-cli.md`
6. `docs/hardware-pin-modes.md`
7. `docs/wiring-safety.md`
8. `docs/oled-status-standard.md`

## 技術原則

- 所有一般與相機範例的 Arduino CLI FQBN 都是 `esp32:esp32:esp32wrover`。
- 相機範例不更換開發板，只在程式中使用 `CAMERA_MODEL_AI_THINKER` 腳位定義。
- Arduino CLI 是正式編譯、燒錄與驗證工具；Arduino IDE 是輔助工具。
- 不得把尚未在指定 Core 版本編譯通過的範例標示為已完成。
- 舊服務與舊 API 不直接搬運；先確認仍可用，再以新版作法重寫。

## Codex 可觀測性

- 第二篇必須先完成 OLED 啟動自測、基礎顯示與診斷畫面，才開始感測器範例。
- 從 OLED 章開始，每一個 ESP32 執行階段訊息都必須先顯示在 OLED；感測器無資料、Wi-Fi／NTP／HTTP／MQTT 失敗、重試、資料逾時及控制錯誤都不能只寫入 Serial。
- 新增或修改 `Serial.print*()` 訊息時，必須同時提供對應的 OLED 訊息。序列輸出只是可選的同步副本。
- 不可在等待連線或重試的迴圈中保持空白畫面；OLED 必須顯示目前階段、錯誤原因與重試進度。
- OLED 不得顯示密碼、Token、完整 API Key、私人 URL 或其他憑證。
- 請使用者回傳 OLED 正面照片、完整接線照片與編譯結果，讓 Codex 能對照可見狀態排錯。
- OLED 尚未初始化成功時，可以使用 GPIO 2 狀態 LED 閃爍碼作為最低限度的替代訊號；是否為板載 LED 必須依指定課程板確認。
- 相機模式仍必須提供 OLED 狀態，但不得使用已被 AI Thinker 相機占用的 GPIO 21/22；替代 I²C 腳位必須先完成指定課程板實測，未驗證前不得標示完成。

## 公開安全

- 不得列印、提交或回覆 Wi-Fi 密碼、API Key、Token、私人 MQTT 帳密或 Google Apps Script URL。
- 憑證使用 `secrets.h`，Repository 只提交 `secrets.example.h`。
- 不得提交 `.env`、私鑰、本機編譯輸出、原始課本工作檔或未授權第三方內容。

## 教材圖像

- AB143 課本圖只取與目前教材相符的單張圖，不提交整頁課本、原始 DOCX／PDF 或出版社版面。
- 每張圖必須有語意化檔名、替代文字、圖說，並記錄在同目錄的 `SOURCES.md`。
- 舊 IDE、Serial-only、錯誤腳位、不同板型或來源不明的第三方圖片不得直接沿用。

## 硬體安全

- LED 範例接線依第一篇首頁的統一說明與指定課程板配置，不在各範例重複說明。
- SG90 應使用適當的獨立 5V 電源並與 ESP32 共地。
- 繼電器先在無市電負載狀態測試，並定義安全的啟動狀態。
- PZEM-004T 市電側不以遠端文字引導學生帶電操作。

## Git 規則

- 修改前先檢查 `git status --short --branch`。
- 不回退或覆蓋使用者的無關修改。
- 提交前檢查 diff、憑證與大型檔案。
- 修改完成且驗證後，除非使用者要求不推送，否則提交並同步至 GitHub。
