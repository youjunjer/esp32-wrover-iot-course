# AGENTS.md

本檔案是未來 Codex 或其他開發者進入專案時的首要操作規則。

## 工作前必讀

1. `README.md`
2. `docs/CONVERSATION_HANDOFF.md`
3. `docs/PROJECT_LAYOUT.md`
4. `docs/environment-cli.md`
5. `docs/hardware-pin-modes.md`
6. `docs/wiring-safety.md`

## 技術原則

- 所有一般與相機範例的 Arduino CLI FQBN 都是 `esp32:esp32:esp32wrover`。
- 相機範例不更換開發板，只在程式中使用 `CAMERA_MODEL_AI_THINKER` 腳位定義。
- Arduino CLI 是正式編譯、燒錄與驗證工具；Arduino IDE 是輔助工具。
- 不得把尚未在指定 Core 版本編譯通過的範例標示為已完成。
- 舊服務與舊 API 不直接搬運；先確認仍可用，再以新版作法重寫。

## 公開安全

- 不得列印、提交或回覆 Wi-Fi 密碼、API Key、Token、私人 MQTT 帳密或 Google Apps Script URL。
- 憑證使用 `secrets.h`，Repository 只提交 `secrets.example.h`。
- 不得提交 `.env`、私鑰、本機編譯輸出、原始課本工作檔或未授權第三方內容。

## 硬體安全

- LED 必須使用適當的限流電阻。
- SG90 應使用適當的獨立 5V 電源並與 ESP32 共地。
- 繼電器先在無市電負載狀態測試，並定義安全的啟動狀態。
- PZEM-004T 市電側不以遠端文字引導學生帶電操作。

## Git 規則

- 修改前先檢查 `git status --short --branch`。
- 不回退或覆蓋使用者的無關修改。
- 提交前檢查 diff、憑證與大型檔案。
- 修改完成且驗證後，除非使用者要求不推送，否則提交並同步至 GitHub。
