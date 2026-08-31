# AB143 ESP32 Wrover 物聯網與能源監測教材

這是一套以 AB143 教材為基礎、`ESP32 Wrover Module` 為統一開發板、Arduino CLI 為主要工具的 ESP32 物聯網課程範例庫。

本專案正在整併早期 AB143 教材、NMK99/Wrover 教學範例與 `esp32-mqtt-energy-meter` 能源監測專題。目前先建立可下載、可重複編譯、不含憑證的 GitHub 教材結構；完整課本文字尚未公開放入。

## 統一技術基準

- 開發板：`ESP32 Wrover Module`
- Arduino CLI FQBN：`esp32:esp32:esp32wrover`
- ESP32 Core：穩定版，目前驗證基準為 `3.3.11`
- 序列監控鮮率：`115200`
- 相機範例：開發板仍選 Wrover，程式內使用 `CAMERA_MODEL_AI_THINKER`
- 開發主流程：Arduino CLI
- 輔助工具：Arduino IDE 2.x（編輯、序列監控與人工排錯）

## 教材架構

1. `01_basics`：Arduino C、GPIO、數位與類比基礎
2. `02_sensors_display`：感測器、1602 LCD 與 OLED
3. `03_network_cloud_mqtt`：Wi-Fi、HTTP、JSON、NTP、ThingSpeak、Google Sheets 與 MQTT
4. `04_nodered`：Node-RED、MQTT Flow、Dashboard 與資料處理
5. `05_camera_ble_multitasking`：相機、Bluetooth、BLE 與 FreeRTOS 多工
6. `06_energy_monitoring`：PZEM-004T、OLED、MQTT、繼電器、SG90 與能源監測整合

## 快速開始

安裝 Arduino CLI 後，先安裝指定的 ESP32 Core：

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.11
arduino-cli core list
```

編譯第一個範例：

```bash
arduino-cli compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello
```

燒錄與監控：

```bash
arduino-cli board list
arduino-cli upload \
  -p <SERIAL_PORT> \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello
arduino-cli monitor -p <SERIAL_PORT> -c baudrate=115200
```

完整環境說明請見 [docs/environment-cli.md](docs/environment-cli.md)；一般模式與相機模式的腳位邊界請見 [docs/hardware-pin-modes.md](docs/hardware-pin-modes.md)。

## 憑證與公開安全

不得將 Wi-Fi 密碼、API Key、Google Apps Script URL、MQTT 帳密或 Token 提交到 GitHub。需要憑證的範例會附 `secrets.example.h`，使用者必須在本機複製為 `secrets.h`。

## 目前進度

- [x] 建立教材結構與統一技術基準
- [x] 建立第一批無外部函式庫基礎範例
- [ ] 建立 Wrover 一般模式／AI Thinker 相機模式腳位對照
- [ ] 整併 1602 LCD、OLED 與感測器範例
- [ ] 整併 ThingSpeak、Google Sheets 與 MQTT 範例
- [ ] 整併 Node-RED Flow
- [ ] 整併相機、BLE 與多工範例
- [ ] 整併能源監測專題
- [ ] 對所有範例進行 CLI 編譯與實體板驗證

## 來源邊界

本 Repository 收錄經過整理、可公開與已驗證的教學範例。AB143 完整出版原稿、Word/PDF、歷史憑證、尚未驗證程式與第三方函式庫原始碼不直接納入。

## 授權

程式碼與教材文字的正式授權尚未確定。未加入 `LICENSE` 前，請不要假設本專案已授權任意重製或商業使用。
