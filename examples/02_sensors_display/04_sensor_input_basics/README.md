# 04 Sensor Input Basics

同時讀取 GPIO 14 的數位輸入與 GPIO 33 的 12-bit ADC 原始值，並將結果直接顯示在 OLED。這是接上 PIR、光敏、MQ-2 等模組前的輸入診斷範例。

## 腳位

| 功能 | GPIO |
|---|---:|
| OLED SDA | 21 |
| OLED SCL | 22 |
| 數位輸入 | 14 |
| 類比輸入（ADC1） | 33 |
| OLED 失敗備援燈號 | 2 |

所有訊號輸入必須在 `0～3.3V` 範圍內。GPIO 33 顯示的是未校正 ADC 原始值，不是照度、溫度或其他物理量。

## 畫面

- `D14 HIGH/LOW`：數位輸入狀態。
- `A33 0..4095`：GPIO 33 的 12-bit ADC 原始值。
- `OPEN = UNDEFINED`：GPIO 33 未接安全的 3.3V 類比來源時，浮動數字沒有意義。
- OLED 找不到時，GPIO 2 重複閃 2 次；OLED 初始化失敗時重複閃 3 次。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/04_sensor_input_basics
```

GPIO 33 只在接上已確認為 0～3.3V 的類比來源後才進行驗收；不可用浮接數字判定成功。GPIO 14 使用內建下拉，未接來源時的 `LOW` 也不是接線成功證據。CI 編譯不代表實體輸入腳位、接線或 OLED 已經驗證。
