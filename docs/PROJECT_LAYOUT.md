# Project Layout

```text
esp32-wrover-iot-course/
├── AGENTS.md
├── README.md
├── arduino-cli.yaml
├── docs/
│   ├── assets/
│   │   ├── ab143/
│   │   ├── part2/
│   │   ├── part3/
│   │   └── part4/
│   ├── part1/
│   ├── part2/
│   ├── part3/
│   ├── part4/
│   ├── part5/
│   └── ...
├── examples/
│   ├── 01_basics/
│   ├── 02_sensors_display/
│   ├── 03_network_cloud_mqtt/
│   ├── 04_nodered/
│   ├── 05_camera_ble_multitasking/
│   └── 06_energy_monitoring/
├── nodered/
└── scripts/
```

## 進入 Repository

```bash
cd /path/to/esp32-wrover-iot-course
```

## 目錄邊界

- `docs/course-map.md`：正式章節順序與教材範圍。
- `docs/assets/ab143/`：由 AB143 課本取出的教材圖像及來源記錄，不放整頁課本或原始出版檔。
- `docs/assets/part2/`：第二篇目前接線圖、預期畫面與真實操作截圖；預期圖和實測證據分開記錄。
- `docs/assets/part3/`：第三篇網路狀態預期畫面與真實 CI／實機證據；示意圖和操作截圖分開存放。
- `docs/part1/`：第一篇正式課文、驗收與 AB143 遷移記錄。
- `docs/part2/`：第二篇 OLED、感測器與顯示器課文。
- `docs/part3/`：第三篇 Wi-Fi、NTP、HTTPS、JSON／公開 API、WebServer、雲端與 MQTT 課文。
- `docs/oled-status-standard.md`：所有 ESP32 執行階段訊息的 OLED 顯示規範。
- `docs/` 其他文件：環境、腳位、安全、排錯與交接。
- `examples/`：依教材六篇分類的 Arduino Sketch。
- `docs/part4/`：Node-RED 7 章、來源與驗證紀錄；操作圖在 `docs/assets/part4/captures/`。
- `nodered/`：6 份可公開 Flow、套件版本與 lockfile；Runtime／credentials／context 留在專用 userDir。
- `docs/part5/`：Bluetooth／BLE、多工與有預檢條件的相機 8 章；9 個 Sketch、MQTT 影像接收器與軟體／實機驗收界線。
- `tests/`：純 C++ 指令與相機分段合約測試，不連接硬體或外部 Broker。
- `scripts/`：ESP32 Core、函式庫、編譯與驗證輔助工具。

`secrets.h`、`.env`、Token、API Key、本機編譯輸出與原始出版檔不得提交。
