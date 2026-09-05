# AB143 ESP32 Wrover 物聯網與能源監測教材

這是一套以 AB143 教材為基礎、`ESP32 Wrover Module` 為統一開發板、Arduino CLI 為主要工具的 ESP32 物聯網課程範例庫。

本專案正在整併早期 AB143 教材、NMK99/Wrover 教學範例與 `esp32-mqtt-energy-meter` 能源監測專題。目前先建立可下載、可重複編譯、不含憑證的 GitHub 教材結構；完整課本文字尚未公開放入。

## 統一技術基準

- 開發板：`ESP32 Wrover Module`
- Arduino CLI FQBN：`esp32:esp32:esp32wrover`
- ESP32 Core：穩定版，目前驗證基準為 `3.3.11`
- 序列監控鮑率：`115200`
- 相機範例：開發板仍選 Wrover，程式內使用 `CAMERA_MODEL_AI_THINKER`
- 開發主流程：Arduino CLI
- 輔助工具：Arduino IDE 2.x（編輯、序列監控與人工排錯）

## 教材架構

1. `01_basics`：Arduino C、GPIO、PWM 與全彩 LED
2. `02_sensors_display`：I²C、OLED 可視化診斷、感測器與 1602 LCD
3. `03_network_cloud_mqtt`：Wi-Fi、HTTP、JSON、NTP、ThingSpeak、Google Sheets 與 MQTT
4. `04_nodered`：Node-RED、MQTT Flow、Dashboard 與資料處理
5. `05_camera_ble_multitasking`：相機、Bluetooth、BLE 與 FreeRTOS 多工
6. `06_energy_monitoring`：PZEM-004T、OLED、MQTT、繼電器、SG90 與能源監測整合

## 快速開始

安裝 Arduino CLI 後，在 Repository 根目錄安裝鎖定的 ESP32 Core 與函式庫：

```bash
arduino-cli version
./scripts/install-toolchain.sh
./scripts/install-libraries.sh
```

編譯第一個範例：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello
```

燒錄與可見驗證：

```bash
arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello
```

重啟後 GPIO 2 狀態 LED 會先快速閃爍三次，再每兩秒顯示一次短心跳。如果指定課程板沒有 GPIO 2 板載 LED，請依第一篇首頁的課程接線方式接上 LED。

第一篇正式教材與驗收表請見 [docs/part1/README.md](docs/part1/README.md)；第二篇 OLED 起始教材請見 [docs/part2/README.md](docs/part2/README.md)；完整章節順序請見 [docs/course-map.md](docs/course-map.md)；環境說明請見 [docs/environment-cli.md](docs/environment-cli.md)；一般模式與相機模式的腳位邊界請見 [docs/hardware-pin-modes.md](docs/hardware-pin-modes.md)。

## 可視化診斷原則

第一篇完成基礎燒錄後，第二篇先建立 OLED 啟動自測與顯示，再進入各種感測器。從這裡開始，每一個 ESP32 執行階段訊息都必須先呈現在 OLED，包括啟動階段、感測器無資料、Wi-Fi／NTP／HTTP／MQTT 連線失敗、重試次數、即時數值、資料逾時與錯誤碼。序列輸出只能同步複製，不可作為唯一的除錯管道。這讓使用者可以用 OLED 畫面照片協助 Codex 判斷實機狀態。

訊息格式與錯誤碼規則請見 [docs/oled-status-standard.md](docs/oled-status-standard.md)。

## 憑證與公開安全

不得將 Wi-Fi 密碼、API Key、Google Apps Script URL、MQTT 帳密或 Token 提交到 GitHub。需要憑證的範例會附 `secrets.example.h`，使用者必須在本機複製為 `secrets.h`。

## 目前進度

- [x] 建立教材結構與統一技術基準
- [x] 完成第一篇教材、8 個可見輸出範例與自動編譯驗收
- [x] 建立第二篇 OLED 自動尋址、基礎顯示與診斷版型範例
- [x] 完成 GPIO 14／33 輸入診斷、PIR 與光敏 ADC 校正教材；CI 已編譯全部 14 個 Sketch
- [x] 完成第二篇 MQ-2、超音波、蜂鳴器、DHT11、1602 LCD 與多感測顯示課文及範例；CI 已編譯全部 20 個 Sketch
- [ ] 建立 Wrover 一般模式／AI Thinker 相機模式腳位對照
- [ ] 整併 ThingSpeak、Google Sheets 與 MQTT 範例
- [ ] 整併 Node-RED Flow
- [ ] 整併相機、BLE 與多工範例
- [ ] 整併能源監測專題
- [ ] 對所有範例完成實體板驗證；目前 CI 編譯不等於燒錄或硬體成功

## 來源邊界

本 Repository 收錄經過整理、可公開與已驗證的教學範例。AB143 完整出版原稿、Word/PDF、歷史憑證、尚未驗證程式與第三方函式庫原始碼不直接納入。

## 授權

程式碼與教材文字的正式授權尚未確定。未加入 `LICENSE` 前，請不要假設本專案已授權任意重製或商業使用。
