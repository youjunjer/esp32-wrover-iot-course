# 05 PIR OLED

使用 HC-SR501 類 PIR 模組從 GPIO 14 讀取移動狀態。程式先在 OLED 顯示 60 秒 `WARMUP` 倒數，再以 150 ms 穩定判斷顯示 `CLEAR` 或 `MOTION`，並計算 LOW 到 HIGH 的事件次數。

## 腳位

| 功能 | 接點 |
|---|---|
| PIR VCC | 課程模組的 5V 供電端 |
| PIR OUT | GPIO 14 |
| PIR GND | ESP32 GND |
| OLED | 3.3V、GND、SDA 21、SCL 22 |

不同 PIR 模組的供電範圍與腳位順序可能不同。接線前先依手邊模組標示確認，並確認 OUT 高電位不超過 ESP32 的 3.3V 輸入範圍。

## 畫面

- `WARMUP`：模組穩定倒數，這段時間不判定有人或無人。
- `CLEAR`：已穩定確認的狀態為 LOW。
- `MOTION`：已穩定確認的狀態為 HIGH。
- `RAW HIGH/LOW`：最新一次 GPIO 14 讀值；150 ms 確認期間可能短暫與大字狀態不同。
- `EVENTS n`：偵測到的 LOW→HIGH 次數，不等於現場實際人數。
- OLED 找不到時，GPIO 2 重複閃 2 次；OLED 初始化失敗時重複閃 3 次。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/05_pir_oled
```

CI 編譯不代表 PIR 暖機時間、靈敏度、延遲旋鈕或實體接線已經驗證。
