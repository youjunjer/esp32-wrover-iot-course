# 10 DHT11 OLED

使用 GPIO 14 每 2 秒讀取一次 DHT11，並由 OLED 顯示 `WAIT`、`OK`、`NO DATA`、`READ ERR` 或 `STALE`。上一筆成功資料若保留在畫面，會明確標示為 `LAST` 並附上年齡。

## 腳位

| 功能 | 接點 |
|---|---|
| DHT11 VCC | 3.3V |
| DHT11 DATA | GPIO 14 |
| DHT11 GND | ESP32 GND |
| OLED | 3.3V、GND、SDA 21、SCL 22 |

裸 DHT11 的 DATA 通常需要 4.7 kΩ～10 kΩ 上拉到 3.3V；模組版先確認是否已有上拉。GPIO 14 與 PIR／超音波範例共用，切換章節前先斷電並拆除上一個模組。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `WAIT` | 尚未到第一次 2 秒讀取時間 |
| `OK` | 本次溫度與溼度皆有效 |
| `NO DATA` | 開機後從未成功讀取 |
| `READ ERR` | 本次失敗，但仍有上一筆成功資料 |
| `STALE` | 最後成功資料已超過 10 秒 |
| `LAST` / `AGE` | 畫面是舊值，並顯示資料年齡 |

`AGE` 與失敗次數顯示上限為 `9999`。任何改線都要先斷開 USB 與外部電源；運轉中斷線測試只能使用教師預先安裝的電氣隔離治具，學生禁止熱插拔。

## 函式庫

```text
DHT sensor library@1.4.7
Adafruit Unified Sensor@1.1.15
```

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/10_dht11_oled
```

CI 編譯不代表 DHT11 接線、上拉、讀值準確度或 OLED 畫面已完成實測。
