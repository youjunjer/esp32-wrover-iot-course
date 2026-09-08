# 第三篇：Wi-Fi、網路時間、HTTPS、JSON 與網頁伺服器

第三篇把第二篇建立的 OLED 可視化診斷延伸到網路。學生先學會辨識 Wi-Fi 掃描、連線與重試狀態，再取得可信任的網路時間；完成校時後才進入憑證驗證的 HTTPS、ArduinoJson 與公開空氣品質資料，最後由 ESP32 在區域網路提供網頁服務。

本篇不以 Serial Monitor 作為主要觀察工具。Wi-Fi、NTP、TLS／網路、HTTP、JSON 與 WebServer 的啟動、成功、失敗、重試及資料過期狀態，都必須先顯示在 OLED；序列輸出只能是同步副本。若 Core 只回報合併的底層錯誤，畫面應如實標為 `TLS/NET ERR`，不猜測是 DNS 或其他單一原因。

## 章節規劃

1. [Wi-Fi 掃描、連線與重試](01_wifi_scan_connect.md)
2. [NTP 網路校時與資料新鮮度](02_ntp_clock.md)
3. [經過憑證驗證的 HTTPS GET](03_https_get.md)
4. [ArduinoJson 與 Open-Meteo 空氣品質資料](04_air_quality_json.md)
5. [ESP32 WebServer 與區域網路控制](05_webserver.md)

章序不可任意對調。HTTPS 憑證具有有效期間，因此先取得可信任時間，再驗證 TLS；JSON 章則建立在已能分辨 DNS、TLS 與 HTTP 錯誤的基礎上，避免學生把所有失敗都誤認成 JSON 問題。

## 統一硬體與工具基準

- 開發板：ESP32 Wrover Module
- FQBN：`esp32:esp32:esp32wrover`
- OLED：SSD1306 128×64，SDA GPIO 21、SCL GPIO 22
- OLED 安裝方向基準：`setRotation(2)`；不同模組依實物調整
- 網路：ESP32 可使用的 2.4 GHz Wi-Fi；實際 AP 相容性必須由學生現場確認
- 編譯與燒錄：Arduino CLI 為正式流程，Arduino IDE 為輔助工具
- 函式庫與 Core：以 Repository 鎖定檔與 GitHub Actions 當次紀錄為準

## 憑證與隱私規則

- Wi-Fi SSID、密碼及任何私人端點放在 `secrets.h`；Repository 只提交 `secrets.example.h`。
- OLED、Serial、截圖、Issue 與課堂回報都不得顯示密碼、Token、API Key、完整私人 URL、MAC 位址或可識別裝置的 Client ID。
- Wi-Fi 掃描結果可能暴露附近住戶或機構名稱。OLED 只顯示 AP 數量、目標是否出現及 RSSI，不顯示 SSID 或完整 IP；學生回報照片前也要遮蔽可識別網路資訊。
- 第 3 章禁止使用 `setInsecure()` 當作完成方式；必須驗證伺服器憑證鏈，並把前置時間、TLS／網路與 HTTP 錯誤分層顯示。
- Open-Meteo 範例不需要把 API Key 寫進程式；座標仍可能透露位置，公開教材只用教學座標或模糊區域，不提交私人住址。
- WebServer 範例只定位為受信任區域網路內的教學服務，不宣稱可安全直接暴露到公網。

## OLED 統一狀態

| 層級 | 啟動／等待 | 成功 | 失敗或資料過期 |
|---|---|---|---|
| Wi-Fi | `WIFI OLED`、`SCAN`、`CONNECT` | `ONLINE`、`IP ASSIGNED`、RSSI | `NO AP`、`CONN ERR`、`LINK LOST`、`RETRY` |
| NTP | `NTP CLOCK UTC+8`、`NTP SYNC` | `TIME OK`、日期時間、`SYNC Ns NET ON` | `NTP T/O`、`NTP STALE`、`NET LOST`、`RETRY` |
| HTTPS | `TLS VERIFY`、`GET`、`HTTP 200`、`BODY n/2048 B` | `TLS VERIFIED`、HTTP 狀態碼、TLS 錯誤碼、body bytes | `TIMEOUT`、`TLS/NET ERR`、`HTTP ERR`、`BODY EMPTY`、`BODY SHORT`、`BODY TOO LARGE` |
| JSON／AQI | `AIR QUALITY`、`HTTPS GET`、`JSON READ`、`JSON PARSE` | `OPEN-METEO/CAMS`、`EAQI`、`PM2.5`、`PM10`、`SRC`、`AGE` | `JSON ERR`、`FIELD/RANGE ERR`、`UNIT ERR`、`SOURCE FUTURE`、`SOURCE STALE` |
| WebServer | `WEB GPIO13`、`WIFI CONNECT`、`TIME LEFT` | `WEB READY`、`.local` 主機名、`CMD OK` | `WIFI TIMEOUT`、`NET LOST`、`RETRY`、`SAFE OFF` |

錯誤畫面必須保留到狀態改變或使用者確認，不能一閃而過。重試畫面要顯示次數或倒數；有最後成功資料時仍要顯示 `AGE`，超過章節定義時限後改標 `STALE`，不能只留下舊數字。

## 預期畫面與證據邊界

- [Wi-Fi OLED 預期畫面](../assets/part3/guides/wifi-expected.svg)
- [NTP OLED 預期畫面](../assets/part3/guides/ntp-expected.svg)
- [verified HTTPS OLED 預期畫面](../assets/part3/guides/https-expected.svg)
- [Open-Meteo AQI OLED 預期畫面](../assets/part3/guides/air-quality-expected.svg)
- [WebServer OLED 預期畫面](../assets/part3/guides/webserver-expected.svg)

這五張圖都是 Repository 原生 SVG，直接標示「非實機／非實測資料」。它們只能說明狀態名稱、欄位與版面，不能證明 Wi-Fi、NTP、TLS、公開 API 或 WebServer 已在實體板上成功。

## GitHub Actions 編譯證據

![第三篇前五章 GitHub Actions 編譯成功](../assets/part3/captures/github-actions-part3-foundations-success.jpg)

上圖是公開 Repository 的真實 [GitHub Actions Run 34241431791](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/34241431791) 畫面，對應 Commit `d53bbcaa0168b98e3c8a2e5df2ab688abfdb6135`。該次工作流程使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11、FQBN `esp32:esp32:esp32wrover` 與 `config/libraries.lock` 的函式庫版本，編譯通過清單內全部 25 個 Sketch；其中包含第三篇前五章的 5 個 Sketch。

這張圖只證明上述 Commit 能在 CI 以鎖定工具鏈完成編譯。它不代表程式已燒錄，也不證明 OLED、Wi-Fi、NTP、TLS、Open-Meteo、瀏覽器操作或 GPIO 13 已在實體板上通過。截圖來源、時間與 SHA-256 見 [第三篇真實操作截圖來源](../assets/part3/captures/SOURCES.md)。

## 本篇完成條件

- 五章課文、五個自足式 Sketch、各章 README、接線表／資料流說明與 OLED 預期畫面齊全。
- `secrets.h` 不進版控；`secrets.example.h` 只包含明確的佔位值。
- 每個等待操作都有上限、可見進度及重試間隔，不使用永久阻塞連線迴圈。
- HTTPS 驗證憑證，不以 `setInsecure()` 或明文 HTTP 取代。
- JSON 解析檢查反序列化錯誤、缺少物件或欄位、錯誤型別、非數字值、單位與資料時間。
- WebServer 使用明確 route、404 回應及安全的輸出啟動狀態。
- 所有 Sketch 使用鎖定工具鏈通過 GitHub Actions；真實 Run 截圖需記錄 Commit、版本與 `CI compile only` 邊界。
- 實體板另行驗證 Wi-Fi 相容性、OLED 畫面、NTP 時間、TLS 憑證、API 內容與 WebServer 操作；CI 不代替硬體或外部服務驗收。

教材依據與淘汰項目見 [第三篇來源與遷移記錄](source-map.md)，共通畫面規範見 [OLED 執行狀態與除錯訊息規範](../oled-status-standard.md)。
