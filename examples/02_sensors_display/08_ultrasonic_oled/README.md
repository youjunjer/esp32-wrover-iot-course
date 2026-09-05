# 08 Ultrasonic OLED

使用 HC-SR04、TRIG GPIO 25 與 ECHO GPIO 14 測量距離。所有有效距離、`TIMEOUT`、`RANGE ERR` 與重試資訊都直接顯示在 OLED，不依賴 Serial Monitor。

## 腳位與 Echo 降壓

| 功能 | 接點 |
|---|---|
| HC-SR04 VCC／GND | 5V／GND |
| TRIG | GPIO 25 |
| ECHO | 使用 1% 電阻：Echo → 10kΩ → GPIO 14，GPIO 14 → 15kΩ → GND；5.0V 名目輸出約分壓為 3.00V，仍須量測，或使用合適位準轉換器 |
| OLED | 3.3V、GND、SDA 21、SCL 22 |
| OLED 失敗備援燈號 | GPIO 2 |

Echo 不可直接 ESP32。改動接線前先斷開 USB 與外部電源，並確認所有元件共地。

GPIO 14 在 PIR 範例也是輸入腳；本範例不與 PIR 同時接線，切換前應先斷電拆除 PIR。

## OLED 畫面

- `OK`：本次距離在 2～400 cm 的課程範圍。
- `TIMEOUT`：`pulseIn()` 在 30 ms 內沒有收到 Echo，不顯示 0 cm 或舊值；`INVALID` 是連續無效讀值次數。
- `RANGE ERR`：有 Echo，但換算距離超出範圍。
- GPIO 2 閃 2 下：找不到 OLED。閃 3 下：OLED 初始化失敗。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/02_sensors_display/08_ultrasonic_oled
```

Echo 電壓、距離、視角、干擾、接線與 OLED 畫面尚未在指定課程板驗證；CI 編譯不是實機成功證據。完整教學見 [`docs/part2/08_ultrasonic_distance.md`](../../../docs/part2/08_ultrasonic_distance.md)。
