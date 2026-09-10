# 03 經過 CA 驗證的 HTTPS GET OLED

本範例自足完成 Wi-Fi 連線與 NTP 校時，再以 ESP32 Core 3.3.11 的 `NetworkClientSecure` 驗證 Open-Meteo HTTPS 憑證鏈。程式不使用 `setInsecure()`，也不依賴 Serial Monitor；HTTP、TLS、body 大小、等待與重試均顯示在 OLED。

## OLED 接線

| OLED | ESP32 Wrover |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

OLED 使用 rotation `2`。找不到 OLED 時 GPIO 2 重複閃 2 下；OLED 初始化失敗時重複閃 3 下。

## Wi-Fi 設定

macOS／Linux：

```bash
cp examples/03_network_cloud_mqtt/03_https_get_oled/secrets.example.h \
  examples/03_network_cloud_mqtt/03_https_get_oled/secrets.h
```

Windows PowerShell：

```powershell
Copy-Item examples/03_network_cloud_mqtt/03_https_get_oled/secrets.example.h `
  examples/03_network_cloud_mqtt/03_https_get_oled/secrets.h
```

只在本機 `secrets.h` 填入 `WIFI_SSID` 與 `WIFI_PASSWORD`，不要提交、截圖或貼到 Issue。缺少此檔或仍為 `REPLACE_WITH_...` 佔位字時，程式只顯示 `CONFIG ERR`，不嘗試連線。

## TLS 與 body 邊界

- `open_meteo_root_ca.h` 來自 [Let's Encrypt 官方 ISRG Root X1](https://letsencrypt.org/certificates/)。若服務端日後更換憑證鏈，應重新查證官方來源後更新，不能改用 `setInsecure()`。
- HTTPS 開始前必須先有已同步且落在合理範圍、足供憑證日期檢查的 NTP 時間；一般 SNTP 並非密碼學認證時間。
- 同步 `HTTPClient::GET()` 設有 8 秒逾時，呼叫前 OLED 會先顯示最長等待。
- HTTP 200 body 必須為 1–2048 bytes，每次主迴圈最多讀 128 bytes；空 body、實收長度短於 `Content-Length`，或無進度 5 秒都判定失敗。
- 成功畫面同時保留 HTTP code、TLS error code 與實際 body bytes；不把 body 或私人網路資料輸出到 Serial。

範例端點來自 [Open-Meteo Air Quality API](https://open-meteo.com/en/docs/air-quality-api)，其資料採 Open-Meteo／Copernicus CAMS 模式產品，不是本地實測；本章只計算回應長度。免費 open-access API 無 SLA，且依 [當前官方條款](https://open-meteo.com/en/terms) 限非商業使用；付費課程、公司或正式服務用途需先確認條款與合適方案。

教室多板同 NAT 時，首次請求加 0–15 秒 jitter；成功後至少 15 分鐘再加 0–30 秒 jitter；失敗由 60 秒退避到 15 分鐘並加 jitter，HTTP 429 直接採 15 分鐘。Wi-Fi 重連不會繞過既有期限。

## 主要 OLED 狀態

| 狀態 | 意義 |
|---|---|
| `CONFIG ERR` | `secrets.h` 未完成 |
| `WIFI CONNECT` / `OFFLINE` | Wi-Fi 連線或退避中 |
| `NTP SYNC` / `TIMEOUT` | 等待已同步時間或校時失敗 |
| `JITTER` / `START IN Ns` | 分散同一 NAT 下的首次請求 |
| `TLS VERIFY` | 已載入 CA，準備驗證伺服器 |
| `GET` / `MAX WAIT 8s` | 有限時的同步 HTTPS GET |
| `HTTP 200` / `BODY n/2048 B` | 主迴圈分段讀取 body |
| `TLS VERIFIED` | HTTPS、HTTP 與 body 讀取完成 |
| `TLS/NET ERR` / `HTTP ERR` | 傳輸層或 HTTP 狀態失敗 |
| `BODY EMPTY` / `BODY SHORT` / `BODY TOO LARGE` / `BODY TIMEOUT` | 回應為空、短於宣告長度、超限或停止前進 |

## Arduino CLI

從 Repository 根目錄執行：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/03_https_get_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/03_https_get_oled
```

## 驗收與證據邊界

實機應依序顯示 Wi-Fi、NTP、`JITTER`、`TLS VERIFY`、`GET`、`HTTP 200`，最後同時顯示 `TLS VERIFIED`、HTTP 200、TLS 0 與 1–2048 bytes 的 body 大小。斷網後應離開舊成功畫面並進入可見重試，但重連不得提前觸發 API。

CI 只能證明佔位憑證路徑可編譯、ESP32 Core 3.3.11 API 相容，及靜態程式沒有使用 `setInsecure()`；它不能證明實機時鐘、DNS、Internet、伺服器憑證鏈、HTTP 回應或 OLED 顯示已通過。實機回報請提供 OLED 正面照、完整低壓接線照、板型、Core 版本與燒錄結果，並遮蔽所有網路資訊。
