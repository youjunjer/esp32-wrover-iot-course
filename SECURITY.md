# Security Policy

## 敏感資料

請不要在 Issue、Pull Request、範例、Node-RED Flow 或序列輸出中公開：

- Wi-Fi SSID 與密碼
- API Key、Token 與 OAuth 憑證
- Google Apps Script 私人部署 URL
- MQTT 帳號密碼與私人 Broker 連線資料
- 個人資料、內網 IP、私鑰或憑證檔

如果發現已提交的憑證，先廢止與更換憑證，不只是刪除最新版檔案。

## 硬體安全

涉及 PZEM-004T 市電側、繼電器負載或外部高電流供電時，請先閱讀 `docs/wiring-safety.md`。
