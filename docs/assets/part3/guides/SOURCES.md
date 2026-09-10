# 第三篇 OLED 預期畫面來源

本目錄保存第三篇 Wi-Fi、NTP、verified HTTPS、ArduinoJson／Open-Meteo 空氣品質、WebServer、ThingSpeak、Google Sheets 與 MQTT 的 Repository 原生 SVG。所有圖由 Codex 以純 SVG XML、文字與幾何圖元製作，沒有裁切或嵌入 AB143 頁面、舊 IDE、Serial Monitor、瀏覽器畫面、商品照片或第三方點陣素材。

圖中的網路名稱、IP、RSSI、時間、HTTP 狀態、資料長度、PM2.5、AQI、route 與計數都是版型示意，不是實機或即時服務資料。每張圖都直接標示「非實機／非實測資料」，不能當成編譯、燒錄、網路、TLS、API 或硬體成功證據。

| 圖檔 | 依據 | 證據邊界 | SHA-256 |
|---|---|---|---|
| `wifi-expected.svg` | `01_wifi_oled.ino` 的 `WIFI OLED` 表頭，以及 `SCAN`、`CONNECT`、`ONLINE` 與實際 footer 字串 | Repository 原生預期畫面；不顯示 SSID 或完整 IP，AP 數、RSSI、倒數及上線秒數均非實測 | `70f60a9d3b4a3f92a1bbca2a76f0ed0eaf038121a5ce9d6d448d72b4a5be5388` |
| `ntp-expected.svg` | `02_ntp_clock_oled.ino` 的 `NTP CLOCK UTC+8` 表頭，以及 `NTP SYNC`、`TIME OK`、`NTP STALE` 與實際 footer 字串 | Repository 原生預期畫面；日期、時間、倒數及同步秒數均非實測 | `998c4696892182cbcd0e824edf5a7c1a1dcb1d7a0232d423642c31f39636b0de` |
| `https-expected.svg` | verified HTTPS 的 NTP 校時前置、ISRG Root X1 CA 驗證、HTTP 狀態、body 長度與錯誤分層 | Repository 原生預期畫面；未證明 DNS、憑證鏈、HTTP 或外部服務可用 | `2360d518240463954a346286c42a6323231f043d74815c5d26d8435dc0105abb` |
| `air-quality-expected.svg` | ArduinoJson 欄位與單位驗證、Open-Meteo CAMS `pm10`／`pm2_5`／`european_aqi` 與資料新鮮度 | Repository 原生預期畫面；PM2.5、PM10、EAQI、年齡與錯誤均非即時 API 資料 | `519947680e62a139632a3b53059404356dc8c0451c6da26445c21d1e3a0802a8` |
| `webserver-expected.svg` | `05_webserver_oled.ino` 的 `WEB GPIO13`、連線倒數、`.local` 主機名、合法命令與斷線安全關閉字串 | Repository 原生預期畫面；主機名、輸出、HTTP 狀態碼及重試秒數均非實機結果 | `142ea87d8a4fc71a1247f4008ff42ea8119909e39267e26cb35a3cd43467eea8` |
| `thingspeak-expected.svg` | `06_thingspeak_oled.ino` 的 `SEND`、`TLS VERIFY`、`HTTP 200`、`ENTRY n` 與 `NO DATA` 狀態 | Repository 原生預期畫面；Entry ID、序號、倒數及感測值均為示意，未證明實際感測器、TLS、ThingSpeak 服務或圖表寫入 | `9eb8ac8aed56586880282d0305ff25289b26fe2069f6ad090f4717dd50372314` |
| `google-sheets-expected.svg` | `07_google_sheets_oled.ino` 沿用教師既有 GAS 合約時的 `SHEET SEND`、`REDIRECT`、`HTTP OK` 與人工核對提示 | Repository 原生預期畫面；未連線教師 GAS 或 Sheet，HTTP 狀態、列數與倒數皆非實測，也不能證明工作表已新增資料 | `6568190dfb085b4c49511c795ff3ea78f2f74d8f20d21c271f0499d1cc348060` |
| `mqtt-basics-expected.svg` | `08_mqtt_basics_oled.ino` 的 `TLS CONNECT`、`MQTT ONLINE`、QoS 1 retained status／Last Will 與 `TLS ERR` | Repository 原生預期畫面；未證明 Broker、憑證鏈、ACL、Last Will 或實際 MQTT 連線 | `8cbcd3a2942fe638b5a76cde7dc830afd8013a99a7e3e5fe75fe5753d199d01e` |
| `mqtt-pubsub-expected.svg` | `09_mqtt_pubsub_oled.ino` 的精確 Topic、`SUB OK`、`PUB OK`、`CMD REJECT`、QoS 1 與非 Retain 訊息 | Repository 原生預期畫面；Topic、ACL、序號與封包均為示意，未證明訂閱、發布或 PING／PONG 成功 | `22a30827d1a5b6ac20e322774cbcb7db87bbeca242117cf2adc3069e84a471bb` |
| `mqtt-json-expected.svg` | `10_mqtt_json_oled.ino` 的 schema v1、`MQTT ONLINE`、`PUB OK`、Payload bytes／sequence 與 `STALE` 阻擋 | Repository 原生預期畫面；溫度、濕度、亮度、Payload 大小與序號均為示意，未證明感測器讀值或 MQTT 發布 | `2c2b9556dedc0f8a2dcbecb50fbea0945d0eaf96c6fe40ac99745af12bda5e0e` |
| `mqtt-control-expected.svg` | `11_mqtt_control_oled.ino` 的安全啟動 `SAFE OFF`、`CONTROL READY`、`CMD REJECT`、過期拒絕與輸出關閉 | Repository 原生預期畫面；未證明 Broker、Topic、ACL、繼電器、SG90 或命令時效，也不是市電操作證據 | `03b3577ae8a4f7a46a5bf2853cf0b1a5d2d3f9c85de4d6f6e6a92f732047a547` |

以上 SVG 已通過 XML 格式檢查，SHA-256 使用 `shasum -a 256` 計算。真實 CLI、GitHub Actions、瀏覽器與實體 OLED 證據日後存放於獨立 captures 目錄，並記錄工具版本、指令或 Run、Commit、處理方式與證據邊界。
