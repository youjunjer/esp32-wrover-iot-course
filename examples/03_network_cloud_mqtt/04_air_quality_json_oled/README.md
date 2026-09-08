# 04 Open-Meteo 空氣品質 JSON OLED

本範例自足完成 Wi-Fi、NTP、CA 驗證 HTTPS 與有界 body 接收，再使用 ArduinoJson 7.4.3 的 `JsonDocument`、`Filter` 及 `NestingLimit(4)` 解析 Open-Meteo 空氣品質資料。所有進度與錯誤都顯示在 OLED，不依賴 Serial Monitor。

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
cp examples/03_network_cloud_mqtt/04_air_quality_json_oled/secrets.example.h \
  examples/03_network_cloud_mqtt/04_air_quality_json_oled/secrets.h
```

Windows PowerShell：

```powershell
Copy-Item examples/03_network_cloud_mqtt/04_air_quality_json_oled/secrets.example.h `
  examples/03_network_cloud_mqtt/04_air_quality_json_oled/secrets.h
```

只在本機 `secrets.h` 填入 `WIFI_SSID` 與 `WIFI_PASSWORD`。缺檔或仍為 `REPLACE_WITH_...` 佔位字時只會顯示 `CONFIG ERR`。不要提交 `secrets.h`，也不要在照片、Issue 或課堂訊息公開 Wi-Fi 資訊。

## 資料與解析界線

- 資料來自 [Open-Meteo Air Quality API](https://open-meteo.com/en/docs/air-quality-api)，其空氣品質產品採用 Copernicus CAMS 模型資料；這是區域模型估算，不是開發板旁的感測器實測。展示或衍生教材必須保留 Open-Meteo 與 CAMS attribution。
- Open-Meteo 免費 open-access API 無 SLA，且依 [官方條款](https://open-meteo.com/en/terms)，限非商業使用。付費課程、公司用途或正式服務須先確認當下條款，並採合適的商業方案或經核准的資料源。
- TLS 使用官方 ISRG Root X1 CA；禁止 `setInsecure()`，HTTPS 前必須先完成 NTP。
- JSON body 最多 2048 bytes；實收長度短於已宣告的 `Content-Length`，或讀取停止前進 5 秒，都立即失敗。
- ArduinoJson 7.4.3 只保留 `current`／`current_units` 需要的欄位，並限制巢狀深度為 4。
- 程式驗證物件存在、欄位型別、API 單位、有限數值、PM 0–5000 µg/m³、European AQI 0–1000、interval 1–86400 秒與來源時間；來源不得超前可信時間 5 分鐘以上或老於 2 小時。這些是防禦性資料界線，不是健康或法規標準。
- 同一 NAT 下的每台裝置成功後都至少等待 15 分鐘，再加 0–30 秒 jitter；失敗採 60 秒至 15 分鐘 backoff，HTTP 429 直接拉長為 15 分鐘。

## 主要 OLED 狀態

| 狀態 | 意義 |
|---|---|
| `CONFIG ERR` | 本機 Wi-Fi 設定未完成 |
| `WIFI` / `NTP` | 前置連線或可信時間未完成 |
| `TLS VERIFY` / `HTTPS GET` | CA 驗證及有限時 GET |
| `JSON READ` | 接收不超過 2048 bytes 的 body |
| `JSON PARSE` | 套用 Filter 與 NestingLimit |
| `JSON ERR` | JSON 語法、深度或內容不完整 |
| `FIELD/RANGE ERR` | 欄位、型別、時間或範圍驗證失敗 |
| `UNIT ERR` | API 單位不是 `μg/m³`／`EAQI` |
| `SOURCE FUTURE` / `SOURCE STALE` | 來源時間超前或過舊 |
| `OPEN-METEO/CAMS` | 新資料通過驗證，顯示 PM、EAQI、SRC、AGE |
| `AQ DATA STALE` | 最近更新失敗，目前畫面是舊資料 |

## Arduino CLI

從 Repository 根目錄執行：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/04_air_quality_json_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/04_air_quality_json_oled
```

## 驗收與證據邊界

實機正常流程應依序通過 Wi-Fi、NTP、TLS、HTTP、body 與 JSON 驗證，最後顯示 `OPEN-METEO/CAMS`、EAQI、PM2.5、PM10、來源時間、資料年齡及下次輪詢倒數。斷網後舊資料必須標成 `AQ DATA STALE`，不能繼續冒充新資料。

CI 可以證明 Sketch 在 ESP32 Core 3.3.11 與 ArduinoJson 7.4.3 下可編譯，也可靜態檢查 CA、解析選項、欄位驗證及時間常數；它不能證明實機 Wi-Fi、NTP、TLS、Open-Meteo 當下可用性、CAMS 數值、jitter/backoff 行為或 OLED 顯示。實機回報請附 OLED 正面照、完整低壓接線照、板型、Core／ArduinoJson 版本與燒錄結果，並遮蔽網路資訊。
