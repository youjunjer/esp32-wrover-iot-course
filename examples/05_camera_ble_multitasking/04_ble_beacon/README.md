# BLE 掃描、Beacon 與訊號觀察：04_ble_beacon

完整課文、接線、操作與 OLED 訊息意義：[第五篇課文](../../../docs/part5/03_ble_beacon.md)。

此 Sketch 要連同上一層 `support/` 一起保留，請下載完整 Repository，不能只複製 `.ino`。

在 Repository 根目錄編譯：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/05_camera_ble_multitasking/04_ble_beacon
```

列出連接埠並將 `YOUR_SERIAL_PORT` 替換為實際值後燒錄：

```bash
arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT --fqbn esp32:esp32:esp32wrover \
  examples/05_camera_ble_multitasking/04_ble_beacon
```

一般模式 OLED SDA 21／SCL 22；依課文逐章斷電換線。

編譯與主機端測試不代表已完成實體板驗證。最新證據與待驗收項目見[驗證紀錄](../../../docs/part5/verification.md)。
