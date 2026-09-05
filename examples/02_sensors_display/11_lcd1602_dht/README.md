# 11 LCD1602 DHT

OLED 與 PCF8574 I²C 1602 LCD 共用 SDA GPIO 21、SCL GPIO 22；DHT11 使用 GPIO 14。程式掃描 `0x27`／`0x3F`，PCF8574 離線時由 OLED 顯示 `LCD NO ACK Rns` 與下次重試倒數。

## 安全與腳位

| 功能 | 接點 |
|---|---|
| OLED | 3.3V、GND、SDA 21、SCL 22 |
| 1602 LCD | 已確認安全的電源／位準轉換、SDA 21、SCL 22 |
| DHT11 | 3.3V、GND、DATA 14 |

常見 5V PCF8574 背板會把 SDA／SCL 上拉到 5V，不可直接接 ESP32。只能使用已實測 3.3V 相容的背板，或加入雙向 I²C 位準轉換。只看到背光亮起不是安全或成功證據。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `LCD SCAN` | 正在尋找 `0x27` 或 `0x3F` |
| `LCD ACK 0x..` | I²C 匯流排健康且該 PCF8574 位址回應 |
| `LCD NO ACK R3s` | PCF8574 未回應，3 秒後再試 |
| `WAIT` / `OK` | DHT11 等待首次讀取／本次資料有效 |
| `NO DATA` | DHT11 從未成功讀取 |
| `READ ERR` / `STALE` | 本次讀取失敗／最後成功資料超過 10 秒 |
| `LAST` / `AGE` | 顯示的 DHT11 數值是舊值及其年齡 |

`LCD ACK` 不代表 HD44780、對比與實際文字已正常。SDA／SCL 卡低時，也不保證共用匯流排的 OLED 仍能運作。程式每 10 秒強制重寫兩列，只用來降低單次傳輸漏寫，不是 HD44780 故障偵測。`AGE` 與失敗次數上限為 `9999`。

所有改線先斷開 USB 與外部電源。只有教師預先安裝的電氣隔離治具可在運轉中模擬 NACK 與恢復 ACK；學生禁止熱插拔 LCD。

## 函式庫

```text
LiquidCrystal_PCF8574@2.3.0
DHT sensor library@1.4.7
Adafruit Unified Sensor@1.1.15
```

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/11_lcd1602_dht
```

CI 編譯不代表 I²C 電壓、LCD 位址／對比、DHT11 或兩個顯示器已完成實測。
