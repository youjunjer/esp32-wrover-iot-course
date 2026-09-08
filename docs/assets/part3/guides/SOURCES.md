# 第三篇 OLED 預期畫面來源

本目錄保存第三篇 Wi-Fi、NTP、verified HTTPS、ArduinoJson／Open-Meteo 空氣品質與 WebServer 的 Repository 原生 SVG。所有圖由 Codex 以純 SVG XML、文字與幾何圖元製作，沒有裁切或嵌入 AB143 頁面、舊 IDE、Serial Monitor、瀏覽器畫面、商品照片或第三方點陣素材。

圖中的網路名稱、IP、RSSI、時間、HTTP 狀態、資料長度、PM2.5、AQI、route 與計數都是版型示意，不是實機或即時服務資料。每張圖都直接標示「非實機／非實測資料」，不能當成編譯、燒錄、網路、TLS、API 或硬體成功證據。

| 圖檔 | 依據 | 證據邊界 | SHA-256 |
|---|---|---|---|
| `wifi-expected.svg` | `01_wifi_oled.ino` 的 `WIFI OLED` 表頭，以及 `SCAN`、`CONNECT`、`ONLINE` 與實際 footer 字串 | Repository 原生預期畫面；不顯示 SSID 或完整 IP，AP 數、RSSI、倒數及上線秒數均非實測 | `70f60a9d3b4a3f92a1bbca2a76f0ed0eaf038121a5ce9d6d448d72b4a5be5388` |
| `ntp-expected.svg` | `02_ntp_clock_oled.ino` 的 `NTP CLOCK UTC+8` 表頭，以及 `NTP SYNC`、`TIME OK`、`NTP STALE` 與實際 footer 字串 | Repository 原生預期畫面；日期、時間、倒數及同步秒數均非實測 | `998c4696892182cbcd0e824edf5a7c1a1dcb1d7a0232d423642c31f39636b0de` |
| `https-expected.svg` | verified HTTPS 的可信時間、ISRG Root X1 CA 驗證、HTTP 狀態、body 長度與錯誤分層 | Repository 原生預期畫面；未證明 DNS、憑證鏈、HTTP 或外部服務可用 | `2360d518240463954a346286c42a6323231f043d74815c5d26d8435dc0105abb` |
| `air-quality-expected.svg` | ArduinoJson 欄位與單位驗證、Open-Meteo CAMS `pm10`／`pm2_5`／`european_aqi` 與資料新鮮度 | Repository 原生預期畫面；PM2.5、PM10、EAQI、年齡與錯誤均非即時 API 資料 | `519947680e62a139632a3b53059404356dc8c0451c6da26445c21d1e3a0802a8` |
| `webserver-expected.svg` | `05_webserver_oled.ino` 的 `WEB GPIO13`、連線倒數、`.local` 主機名、合法命令與斷線安全關閉字串 | Repository 原生預期畫面；主機名、輸出、HTTP 狀態碼及重試秒數均非實機結果 | `142ea87d8a4fc71a1247f4008ff42ea8119909e39267e26cb35a3cd43467eea8` |

以上 SVG 已通過 XML 格式檢查，SHA-256 使用 `shasum -a 256` 計算。真實 CLI、GitHub Actions、瀏覽器與實體 OLED 證據日後存放於獨立 captures 目錄，並記錄工具版本、指令或 Run、Commit、處理方式與證據邊界。
