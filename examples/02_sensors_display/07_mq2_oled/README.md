# 07 MQ-2 OLED

使用 GPIO 33（ADC1）讀取 MQ-2 模組的分壓 AO，並在 OLED 顯示暖機、未校正、原始值及相對警示。本範例不輸出 ppm，不是安全認證的氣體警報器。

## 腳位與分壓

| 功能 | 接點 |
|---|---|
| MQ-2 VCC／GND | 指定課程模組的 5V／GND |
| MQ-2 AO | AO → 10kΩ → GPIO 33，GPIO 33 → 12kΩ → GND |
| MQ-2 DO | 不接 |
| OLED | 3.3V、GND、SDA 21、SCL 22 |
| OLED 失敗備援燈號 | GPIO 2 |

AO 不得直接 ESP32。實際電壓必須確認不超過 3.3V，所有電源必須共地。

## OLED 畫面

- `WARMUP`：60 秒課堂啟動觀察倒數；不代表完整預熱。
- `ADC CHECK/ADC ERR`：ADC 接近 0 或 4095 時暫停分級；連續 3 筆才鎖定錯誤，並要恢復 3 筆有效值才解除。
- `UNCAL`：只顯示 `RAW/MIN/MAX`，不宣稱氣體濃度或安全狀態。
- `BASELINE/REL WARN/REL HIGH`：只在完成本機基準校正並將 `CALIBRATION_READY` 改為 `true` 後使用；這些都是相對變化，不表示環境安全。
- GPIO 2 閃 2 下：找不到 OLED。閃 3 下：OLED 初始化失敗。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/07_mq2_oled
```

接線、校正、MQ-2 暖機與 OLED 實體畫面尚未在指定課程板驗證；即使 CI 編譯通過，也不能宣稱實機或氣體警示已驗證。完整教學見 [`docs/part2/07_mq2_gas_sensor.md`](../../../docs/part2/07_mq2_gas_sensor.md)。
