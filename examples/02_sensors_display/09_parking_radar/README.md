# 09 Parking Radar

將 HC-SR04 距離分成 `SAFE`、`CAUTION`、`WARNING`與 `DANGER`，並使用 GPIO 13 的無源蜂鳴器輸出不同頻率。OLED 是主要狀態介面；啟動、逾時、範圍錯誤或 LEDC 失敗時都會顯示 `SAFE OFF` 並保持靜音。

## 腳位

| 功能 | 接點 |
|---|---|
| HC-SR04 VCC／GND | 5V／GND |
| TRIG | GPIO 25 |
| ECHO | 使用 1% 電阻：Echo → 10kΩ → GPIO 14，GPIO 14 → 15kΩ → GND；5.0V 名目輸出約分壓為 3.00V，仍須量測，或使用合適位準轉換器 |
| 三線無源蜂鳴器模組 SIG | GPIO 13；3.3V 邏輯、高阻抗、HIGH 發聲 |
| 蜂鳴器模組 VCC／GND | 依模組規格供電／ESP32 GND，不從 GPIO 供電 |
| OLED | 3.3V、GND、SDA 21、SCL 22 |
| OLED 失敗備援燈號 | GPIO 2 |

Echo 不可直接 ESP32。本範例只支援帶驅動級的三線無源蜂鳴器模組；裸元件、高電流、LOW 觸發或有源蜂鳴器不可直接套用。若模組在 SIG 浮接時可能發聲，另加 10 kΩ 下拉到 GND，並以冷開機測試確認沒有短鳴。

GPIO 14 的 PIR 與 GPIO 13 的第一篇按鈕都必須先斷電拆除，才能依本範例改接 HC-SR04 與蜂鳴器。

## 畫面與安全狀態

- `SAFE`（100～400 cm）：靜音。
- `CAUTION`（50～小於 100 cm）：500 Hz。
- `WARNING`（15～小於 50 cm）：1000 Hz。
- `DANGER`（2～小於 15 cm）：2000 Hz。
- `TIMEOUT`、`RANGE ERR`或 `BUZZER ERR`：一律 `SAFE OFF`並靜音。
- GPIO 2 閃 2 下：找不到 OLED。閃 3 下：OLED 初始化失敗；兩種狀態下蜂鳴器仍靜音。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/09_parking_radar
```

本範例使用 ESP32 Core 3.x 的 `ledcAttach()`、`ledcWriteTone()` 與 `ledcWrite(pin, 0)`，不使用舊 LEDC API。蜂鳴器規格、重設期間關閉偏壓、音量、距離分級、Echo 電壓、接線與 OLED 畫面尚未在指定課程板驗證；CI 編譯不是實機成功證據。完整教學見 [`docs/part2/09_parking_radar.md`](../../../docs/part2/09_parking_radar.md)。
