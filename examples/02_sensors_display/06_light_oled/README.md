# 06 Light OLED

使用 GPIO 33（ADC1）讀取光敏模組的 AO，取 8 次樣本平均後，在 OLED 顯示原始值、明暗分級與本次開機觀察到的最小／最大值。

## 腳位

| 功能 | 接點 |
|---|---|
| 光敏 VCC | 3.3V |
| 光敏 AO | GPIO 33 |
| 光敏 GND | ESP32 GND |
| 光敏 DO | 本範例不接 |
| OLED | 3.3V、GND、SDA 21、SCL 22 |

光敏模組必須使用 3.3V 供電，避免 AO 超過 ESP32 的輸入範圍。顯示的 `RAW` 是 ADC 原始值，不是 lux。

## 校正參數

```cpp
constexpr bool CALIBRATION_READY = false;
constexpr bool DARK_IS_HIGH = true;
constexpr int BRIGHT_THRESHOLD = 1200;
constexpr int DARK_THRESHOLD = 2800;
```

預設 `CALIBRATION_READY = false`，OLED 只顯示 `UNCAL`，不會用未實測門檻宣稱明暗。先遮住與照亮手邊模組，觀察 `MIN`、`MAX` 與數值方向，再修改門檻並將它改為 `true`。不同模組若越亮數值越高，將 `DARK_IS_HIGH` 改為 `false`，並設定 `BRIGHT_THRESHOLD > DARK_THRESHOLD`；越暗數值越高時則維持 `BRIGHT_THRESHOLD < DARK_THRESHOLD`。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/06_light_oled
```

CI 編譯不代表門檻、AO 方向、實體亮度或接線已經驗證。
