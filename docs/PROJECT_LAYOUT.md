# Project Layout

```text
esp32-wrover-iot-course/
├── AGENTS.md
├── README.md
├── arduino-cli.yaml
├── docs/
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

- `docs/`：安裝、腳位、安全、排錯與交接文件。
- `examples/`：依教材六篇分類的 Arduino Sketch。
- `nodered/`：可匯入的 Flow 與 Dashboard 相關說明。
- `scripts/`：Arduino CLI 安裝、編譯與驗證輔助工具。

`secrets.h`、`.env`、Token、API Key、本機編譯輸出與原始出版檔不得提交。
