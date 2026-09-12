# AI Thinker 相機腳位與 OLED 預檢：07_camera_preflight

完整課文、接線、操作與 OLED 訊息意義：[第五篇課文](../../../docs/part5/06_camera_pins.md)。

此 Sketch 要連同上一層 `support/` 一起保留，請下載完整 Repository，不能只複製 `.ino`。

在 Repository 根目錄編譯：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/05_camera_ble_multitasking/07_camera_preflight
```

列出連接埠並將 `YOUR_SERIAL_PORT` 替換為實際值後燒錄：

```bash
arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT --fqbn esp32:esp32:esp32wrover \
  examples/05_camera_ble_multitasking/07_camera_preflight
```

相機範例預設不啟動：必須先依第 6 章核對指定實體板與替代 OLED；本機 `camera_config.h` 不提交。第 7／8 章另需本機 `secrets.h`。不得燒錄 CI 合成設定。

編譯與主機端測試不代表已完成實體板驗證。最新證據與待驗收項目見[驗證紀錄](../../../docs/part5/verification.md)。
