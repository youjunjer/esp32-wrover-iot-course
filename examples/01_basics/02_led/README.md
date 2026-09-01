# 02 LED 數位輸出

這個範例使用 GPIO 2 練習 `pinMode()`、`digitalWrite()` 與 `delay()`。LED 亮、暗各 1 秒，直接顯示程式的執行順序。

## 接線

| 元件 | ESP32 腳位 | 接法 |
|---|---:|---|
| LED | GPIO 2 | GPIO 2 接 LED 正極，LED 負極接 GND |

若課程板已有 GPIO 2 板載 LED，可不再外接。不確定板上 LED 腳位時，依表格外接會更容易觀察。更改接線前先斷電。

## 可見驗證

1. 重啟後 LED 預設熄滅。
2. LED 亮 1 秒、熄 1 秒，持續重複。
3. 將兩個 `delay()` 改為不同數值後，可直接從亮暗時間判斷改動是否生效。

若 LED 始終不亮，檢查 GPIO、LED 極性與共地；若始終全亮，檢查是否誤接到 3.3V。

## 編譯目標

```text
FQBN: esp32:esp32:esp32wrover
ESP32 Core: 3.3.11
外部函式庫: 無
```
