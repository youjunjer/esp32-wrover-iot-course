# 10 MQTT JSON Payload OLED

本章把能源專案既有的 `temp`、`humi`、`light` 欄位改為有版本、來源、順序與時間的 JSON schema，並以 ArduinoJson 7 產生有限長度 Payload：

```json
{"v":1,"device":"team01-device01","seq":7,"ts":1789000000,"temp":26.4,"humi":63.0,"light":2048,"status":"ok"}
```

示例數字只是格式說明，不是實測資料。`device` 必須是每組已設定的非敏感別名，不能使用完整 MAC。`ts` 是 NTP 校時後的 Unix time；`seq` 每次開機從 1 開始。Payload 上限 384 bytes，發布採 QoS 1、不 Retain。

DHT11 使用 GPIO 14、光敏 AO 使用 ADC1 GPIO 33，OLED 使用 GPIO 21／22。任何 `NaN`、超出防禦範圍或超過 10 秒未更新的資料會顯示 `NO DATA`／`STALE` 並阻止發布。JSON 也必須檢查 `measureJson()` 與實際寫入長度，不能用無界字串拼接。

最小 ACL：裝置只可 publish 自己的精確 `data` 與 `status`，觀察端只可 subscribe 這兩個 Topic。不要使用萬用控制訂閱或授權 `class702/#`。`MQTT_ROOT_CA` 只向 Broker 維運者或簽發 CA 官方來源取得，上課前核對憑證鏈、有效期及 SHA-256 指紋；禁止 `setInsecure()`。

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

接線或拔線前先關閉 USB 與外部電源，不熱插拔感測器。

## OLED 狀態

| 畫面 | 意義 |
|---|---|
| `BOOT`／`CONFIG ERR` | OLED 已啟動，或本機設定仍是佔位值 |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線中、退避等待或逾時 |
| `TIME WAIT`／`TIMEOUT`／`TIME STALE` | NTP 同步中、同步逾時，或系統時間已不適合發布 |
| `TLS CONNECT`／`TLS ERR` | 正在驗證 Broker 憑證，或 CA／日期／網路失敗 |
| `MQTT OFF`／`MQTT ERR` | 等待退避，或 CONNECT／CONNACK 失敗 |
| `MQTT ONLINE` | Broker 已連線，JSON schema v1 可開始工作 |
| `PUB OK`／`PUB ERR` | JSON 或 retained status 發布成功／失敗 |
| `JSON ERR`／`JSON TOO LARGE` | 序列化失敗或封包超過 384 bytes，阻止發布 |
| `NO DATA`／`STALE` | 感測值無效或過期，不把舊資料冒充新資料 |
| `LINK LOST` | MQTT loop 失敗並進入重連 |

若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗。

## 編譯、燒錄與驗收

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/10_mqtt_json_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/10_mqtt_json_oled
```

實機驗收：訂閱精確 `data` Topic，逐欄核對型別、值、遞增 sequence、已同步時間與 OLED 當次讀值；斷電後移除 DHT11 再重新上電，Topic 不能繼續發布舊值。CI 只驗證編譯，不證明感測器、網路、Broker、TLS、QoS 或資料時間。
