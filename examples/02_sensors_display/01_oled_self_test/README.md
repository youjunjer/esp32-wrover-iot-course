# 01 OLED 啟動自測

這是第二篇第一個範例。程式先以 GPIO 21／22 啟動 I²C，自動檢查常見的 `0x3C` 與 `0x3D` 位址，再初始化 128×64 SSD1306 OLED。

## 接線

| OLED | ESP32 Wrover |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

本範例是一般模式接線；進入 AI Thinker 相機章後，GPIO 21／22 會發生衝突，不能直接沿用。

若指定課程板沒有已確認的 GPIO 2 板載 LED，請依第一篇的課程接法使用外接狀態 LED。OLED 初始化失敗時不可能在 OLED 上顯示原因，因此 GPIO 2 是最低限度的備援訊號。

## CLI

```bash
./scripts/install-libraries.sh
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/01_oled_self_test
arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/01_oled_self_test
```

## 可見驗收

- 成功：OLED 持續顯示 `INIT OK`、偵測位址及遞增的 `UP` 秒數。
- GPIO 2 連續閃 2 下再停頓：找不到 `0x3C` 或 `0x3D`，先檢查供電、共地、SDA 與 SCL。
- GPIO 2 連續閃 3 下再停頓：找到 I²C 位址，但顯示緩衝區或驅動初始化失敗。

完整教學、圖像與失敗回報格式見 [`docs/part2/01_oled_self_test.md`](../../../docs/part2/01_oled_self_test.md)。
