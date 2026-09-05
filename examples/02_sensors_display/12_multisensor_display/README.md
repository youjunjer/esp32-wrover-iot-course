# 12 Multisensor Display

整合 DHT11 GPIO 14、光敏 AO GPIO 33、OLED 與 I²C 1602 LCD GPIO 21／22。OLED 輪播 `SUMMARY`、`DHT11`、`LIGHT`、`SYSTEM` 四頁，每頁都保留 LCD 連線狀態。

## 腳位

| 功能 | 接點 |
|---|---|
| DHT11 | 3.3V、GND、DATA 14 |
| 光敏模組 | 3.3V、GND、AO 33；DO 不接 |
| OLED | 3.3V、GND、SDA 21、SCL 22 |
| 1602 LCD | 已確認安全的電源／位準轉換、SDA 21、SCL 22 |

1602 背板不得將 SDA／SCL 拉到 5V。光敏預設保持 `CALIBRATION_READY = false`，只顯示 ADC 原始值及 `UNCAL`，不宣稱為 lux。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `WAIT` / `FIRST READ` | DHT11 尚未到首次 2 秒讀取時間 |
| `OK` | 本次 DHT11 資料有效 |
| `NO DATA` | DHT11 從未成功讀取 |
| `READ ERR` / `STALE` | 本次 DHT11 失敗／最後成功資料超過 10 秒 |
| `LAST` / `AGE` | DHT11 畫面是舊值及其年齡 |
| `UNCAL` | 光敏只有原始 ADC 值，尚未完成相對明暗校正 |
| `LCD ACK 0x..` | I²C 匯流排健康且 PCF8574 回應 |
| `LCD NO ACK R3s` | PCF8574 未回應，3 秒後再試 |

DHT11 進入 `NO DATA`、`READ ERR` 或 `STALE` 後，OLED 強制停在 DHT11 頁，直到狀態回復 `OK`。`AGE` 與 DHT 失敗次數最多顯示 `9999`。

`LCD ACK` 只是 PCF8574 ACK，不偵測 HD44780 或實際畫面內容；SDA／SCL 卡低時不保證 OLED 仍可運作。程式每 10 秒強制重寫 LCD 兩列，只降低單次傳輸漏寫。所有改線都先斷開 USB 與外部電源；運轉中模擬斷線只能使用教師預先安裝的電氣隔離治具，學生禁止熱插拔。

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
  examples/02_sensors_display/12_multisensor_display
```

CI 編譯不代表多模組供電、LCD 重連、DHT11、光敏校正或 OLED 多頁已完成實測。
