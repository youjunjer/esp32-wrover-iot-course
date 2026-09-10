# 07 Google Sheets／既有 GAS OLED

本章**不重新設計或部署 Google Apps Script**。程式沿用教師原始範例已開放的 GAS 服務與既有查詢格式：`type=insert`、`dateInclude`、`sheetId`、`sheetTag`、`data`；新版只把 ESP32 端更新為 Wrover、DHT11 GPIO 14、OLED-first、有限時連線與 CA 驗證 HTTPS。

## 使用既有教材設定

1. 從教師提供的原始 Google Sheets 範例取得既有 GAS `/exec` URL、Sheet ID 與工作表名稱；不要建立新的 GAS。
2. 複製 `secrets.example.h` 為未追蹤的 `secrets.h`，把上述三個值與 Wi-Fi 設定填入。
3. Repository、OLED、截圖、Issue 與學生提問都不得出現完整 GAS URL、Sheet ID 或 Wi-Fi 資訊。

既有 GAS 接受 URL query 並由 Sheet ID／工作表名稱決定寫入位置，且原合約沒有裝置驗證 Token、事件 ID 或去重機制。隱藏 URL 不是身分驗證；GAS URL 加 Sheet ID 本身就是可能被濫用的存取能力。因此它只適用於受控課堂與專用、非敏感的教學 Sheet，不作正式系統安全範例。外界若取得設定可能送入資料；課後是否撤銷部署或更換 URL 由教師端決定，不要求學生重設計 GAS。若某次請求已寫入但回應在途中遺失，裝置無法確定是否重複，OLED 會顯示 `RESULT UNKNOWN`，必須到 Sheet 核對，不宣稱自動防重複。

## 資料格式

本章維持既有 GAS 合約，`data` 是 URL 編碼後的 `溫度,濕度` CSV。裝置只產生通過範圍檢查的數值，不讓任意字串進入 Sheet。正常間隔至少 60 秒並加入抖動，DHT11 無效時顯示 `NO DATA` 且不送出。

Apps Script Content Service 可能把回應導向 `script.googleusercontent.com`。程式只接受這個官方 HTTPS 主機、只跟隨一次，兩段都以 GTS Root R4 驗證；不使用 `setInsecure()`。根憑證與服務鏈會更新，上課前須依 Google PKI 重新核對。

## 接線

| 模組腳位 | ESP32 Wrover |
|---|---|
| OLED VCC／GND | 3.3V／GND |
| OLED SDA／SCL | GPIO 21／22 |
| DHT11 VCC／GND | 3.3V／GND |
| DHT11 DATA | GPIO 14 |

接線或拔線前先關閉 USB 與外部電源，不熱插拔感測器。

## OLED 狀態

| 畫面 | 意義 |
|---|---|
| `BOOT`／`READY` | OLED 已啟動，或資料有效且等待下次傳送 |
| `CONFIG ERR` | 尚未填入既有教材設定 |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線、退避等待或逾時 |
| `TIME WAIT`／`TIME OK`／`TIMEOUT` | NTP 同步中、已取得足供憑證日期檢查的時間，或同步逾時 |
| `SHEET SEND`／`EXISTING GAS` | 依既有格式發送，URL 不顯示 |
| `REDIRECT`／`HOST VERIFIED` | 只跟隨一次允許的官方回應主機 |
| `HTTP OK`／`CODE 200` | HTTP 傳輸成功；仍須到 Sheet 確認新列 |
| `SEND ERR`／`RESULT UNKNOWN` | 無法判定是否已寫入，先人工核對 |
| `NO DATA` | DHT11 資料無效，沒有送出 |

若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗。

## Arduino CLI

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/07_google_sheets_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/07_google_sheets_oled
```

完成條件：OLED 顯示 `HTTP OK CODE 200`，並在指定 Sheet 人工確認同一時間的新列與溫濕度正確。CI 只證明編譯，不會呼叫教師 GAS，也不能證明 Google 帳號權限、服務回應、Sheet 寫入或實機感測成功。

官方 TLS／Redirect 背景資料：[Apps Script Content Service](https://developers.google.com/apps-script/guides/content)、[Google Trust Services](https://pki.goog/repository/)。
