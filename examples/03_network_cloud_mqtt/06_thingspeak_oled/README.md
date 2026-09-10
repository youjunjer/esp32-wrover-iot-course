# 06 ThingSpeak OLED

本章把 DHT11 溫度、濕度與 GPIO 33 光敏原始值，透過 CA 驗證的 HTTPS POST 寫入 ThingSpeak。OLED 會顯示 Wi-Fi、NTP、TLS、HTTP、Entry ID、資料錯誤與重試倒數；不需要 Serial Monitor。

## ThingSpeak 準備

1. 建立一個 Channel，啟用 Field 1 `temp`、Field 2 `humi`、Field 3 `light`。
2. 從 Channel 的 API Keys 頁複製 **Write API Key**。
3. 複製 `secrets.example.h` 為未追蹤的 `secrets.h`，填入 Wi-Fi 與 Write API Key。
4. 不要把 Read／Write API Key、私人 Channel URL 或 `secrets.h` 放入 Git、OLED、截圖或提問內容。

ThingSpeak 官方 REST API 以 `https://api.thingspeak.com` 提供服務。免費方案的 Channel 更新頻率不得快於每 15 秒；本範例正常成功後等待至少 30 秒並加入隨機抖動，遇到 HTTP 429 則退避 15 分鐘。方案與限制可能改變，上課前應重新查看官方文件。

## 接線

| 模組腳位 | ESP32 Wrover |
|---|---|
| OLED VCC／GND | 3.3V／GND |
| OLED SDA／SCL | GPIO 21／22 |
| DHT11 VCC／GND | 3.3V／GND |
| DHT11 DATA | GPIO 14 |
| 光敏模組 VCC／GND | 3.3V／GND |
| 光敏模組 AO | GPIO 33 |
| 光敏模組 DO | 不接 |

本章沿用第二篇規定的低壓接法，但仍須在指定課程板完成實機驗證。DHT11 或 ADC 無效時顯示 `NO DATA` 並阻止上傳，不以舊值冒充新資料。接線或拔線前先關閉 USB 與外部電源，不熱插拔感測器。

## OLED 狀態

| 畫面 | 意義 |
|---|---|
| `BOOT`／`READY` | OLED 已啟動，或感測資料有效且等待下次上傳 |
| `CONFIG ERR` | 缺少本機設定；不連線 |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線、退避等待或連線逾時 |
| `TIME WAIT`／`TIME OK`／`TIMEOUT` | NTP 同步中、已取得足供憑證日期檢查的時間，或同步逾時 |
| `SEND`／`TLS VERIFY` | 正在執行 CA 驗證 HTTPS POST |
| `HTTP 200`／`ENTRY n` | ThingSpeak 接受資料並回傳正整數 Entry ID |
| `TLS/NET ERR`／`UPLOAD ERR` | TLS／網路／HTTP／回應內容不符合預期 |
| `RATE LIMIT` | 收到 HTTP 429，延長退避 |
| `NO DATA` | 感測資料無效，不送出 |

若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗。

## Arduino CLI

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/06_thingspeak_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/06_thingspeak_oled
```

完成條件是 OLED 顯示 `HTTP 200` 與正整數 `ENTRY`，且 ThingSpeak 圖表的新資料時間及三個欄位與 OLED 當次讀值相符。CI 只證明程式可編譯，不能證明 Wi-Fi、TLS、Channel、API Key、外部服務或感測器已實測。

官方資料：[ThingSpeak REST API](https://www.mathworks.com/help/thingspeak/rest-api.html)、[Channel API Keys 與更新限制](https://www.mathworks.com/help/thingspeak/channel-control.html)、[DigiCert Global Root G2](https://knowledge.digicert.com/general-information/digicert-trusted-root-authority-certificates)。
