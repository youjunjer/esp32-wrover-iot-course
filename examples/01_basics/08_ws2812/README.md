# 08 WS2812 全彩 LED

本範例延續 AB143 舊教材與既有能源監測專案的 WS2812 實驗，使用一條資料線依序顯示白、紅、綠、藍與熄滅。燈光順序就是可見執行狀態，程式沒有 Serial-only 訊息。

## 函式庫

Arduino Library Manager 精確名稱：

```text
Adafruit NeoPixel
```

本教材建議鎖定版本：

```text
Adafruit NeoPixel@1.15.5
```

正式編譯會依 `config/libraries.lock` 安裝這個精確版本，避免課堂與 CI 因函式庫漂移而出現不同結果。

## 接線

| WS2812 | ESP32／電源 |
|---|---|
| DIN | GPIO 32，建議串聯 `330Ω`～`470Ω` 電阻 |
| VCC | 依模組規格接 5V |
| GND | GND，必須與 ESP32 共地 |

- 若指定課程板已內建 GPIO 32 的 WS2812，請以板上接線為準，不要重複外接。
- 程式把全域亮度限制為 `32/255`，降低一顆燈珠教學時的亮度與耗電。
- 外接多顆燈珠時不可由 ESP32 3.3V 腳供電，應使用容量足夠的 5V 電源。未確認燈珠模組可穩定接受 3.3V DIN 時，以 74AHCT 類準位轉換器將資料訊號轉為 5V，不把「剛好會亮」當作穩定接法。
- GPIO 32 在 AI Thinker 相機模式中是相機的 `PWDN`。本範例只適用一般模式，不能和相機同時使用。

改動接線前先斷開 USB 與外部電源。

## 可見結果

1. 啟動後白燈短閃三次，表示程式已開始送出 WS2812 資料。
2. 依序顯示白、紅、綠、藍與熄滅，每個狀態維持 `500 ms`。
3. 完整循環後重複。

WS2812 是單向資料裝置，程式無法讀回燈珠是否真的收到資料。若完全沒有亮燈，請依序檢查 VCC、共地、DIN／DOUT 是否接反、GPIO 32、資料線串聯電阻與函式庫版本；不要把「程式已送出資料」誤判為硬體一定正常。

## 編譯目標

```text
FQBN: esp32:esp32:esp32wrover
ESP32 Core: 3.3.11
外部函式庫: Adafruit NeoPixel@1.15.5
```

版本參考：[Adafruit NeoPixel 官方 Releases](https://github.com/adafruit/Adafruit_NeoPixel/releases)
