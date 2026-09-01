# Project Layout

```text
esp32-wrover-iot-course/
├── AGENTS.md
├── README.md
├── arduino-cli.yaml
├── docs/
│   ├── assets/
│   │   └── ab143/
│   ├── part1/
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

## 本機路徑

```bash
cd "/Users/youjunjer/Library/CloudStorage/GoogleDrive-youjunjer@gmail.com/我的雲端硬碟/E_Codex/ESP32_Develop/esp32-wrover-iot-course"
```

## 目錄邊界

- `docs/course-map.md`：正式章節順序與教材範圍。
- `docs/assets/ab143/`：由 AB143 課本取出的教材圖像及來源記錄，不放整頁課本或原始出版檔。
- `docs/part1/`：第一篇正式課文、驗收與 AB143 遷移記錄。
- `docs/oled-status-standard.md`：所有 ESP32 執行階段訊息的 OLED 顯示規範。
- `docs/` 其他文件：環境、腳位、安全、排錯與交接。
- `examples/`：依教材六篇分類的 Arduino Sketch。
- `nodered/`：可匯入的 Flow 與 Dashboard 相關說明。
- `scripts/`：ESP32 Core、函式庫、編譯與驗證輔助工具。

`secrets.h`、`.env`、Token、API Key、本機編譯輸出與原始出版檔不得提交。
