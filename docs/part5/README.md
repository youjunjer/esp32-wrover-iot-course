# 第五篇：相機、Bluetooth／BLE 與多工

本篇接續已完成軟體驗證的第四篇，先練習 Bluetooth Classic、BLE 與 FreeRTOS，再進入有硬體預檢條件的相機章。共 8 章、9 個 Sketch；編譯與主機測試證據見[驗證紀錄](verification.md)。

**相機與替代 OLED 共存仍待指定課程板實測。** 程式提供預檢、拍照／短串流、Base64 與 PIR／MQTT 整合；預設不啟動相機，不提供猜測的替代腳位，也不把 CI 成功當作硬體通過。

## 章節

| 章 | 主題 | Sketch |
|---|---|---|
| 1 | [Bluetooth Classic 文字](01_bluetooth_classic.md) | `01_bluetooth_text` |
| 2 | [溫溼度、LED／SG90 控制](02_bluetooth_control.md) | `02_bluetooth_control` |
| 3 | [BLE 掃描與 Beacon](03_ble_beacon.md) | `03_ble_scan`、`04_ble_beacon` |
| 4 | [FreeRTOS Task 與共享資料](04_freertos_tasks.md) | `05_freertos_tasks` |
| 5 | [雙核心比較](05_dual_core.md) | `06_dual_core` |
| 6 | [AI Thinker 相機與 OLED 預檢](06_camera_pins.md) | `07_camera_preflight` |
| 7 | [CameraWebServer、拍照、Base64](07_camera_web.md) | `08_camera_web` |
| 8 | [PIR／相機／MQTT 多工](08_camera_mqtt.md) | `09_camera_mqtt` |

## 工具與接線基準

- Arduino CLI 1.5.1、ESP32 Core 3.3.11；所有範例 FQBN 都是 `esp32:esp32:esp32wrover`。
- 藍牙、BLE、FreeRTOS、相機 driver 與 mbedTLS 使用 Core 內附版本；其他函式庫沿用 `config/libraries.lock`，不另外安裝舊 ESP32 BLE Arduino 庫。
- 一般模式 OLED SDA 21／SCL 22，128×64 SSD1306、旋轉方向 2，依第二篇接線。相機模式絕不使用 21／22。
- 只由主迴圈操作 OLED；callback／背景工作回報資料或受鎖保護的短狀態，畫面涵蓋等待、錯誤、重試與資料過期。無新增 Serial-only 訊息。
- GPIO 2 備援 LED 必須依指定板確認。每組 2 閃是找不到 OLED；3 閃為初始化失敗；相機範例的 4 閃為沒有有效替代 I²C 設定。
- GPIO 4／5／13／14 在不同章有不同用途，依[硬體模式表](../hardware-pin-modes.md)逐章斷電換線，不一次全部連接。

## 命令與驗證

在 Repository 根目錄先安裝鎖定工具鏈及函式庫，再依各例 README 編譯與燒錄。不要只下載單一 `.ino`，它依賴同篇的 `support/` 純邏輯與相機 driver。

```bash
./scripts/install-toolchain.sh
./scripts/install-libraries.sh
./scripts/check-policy.sh
./scripts/compile-all.sh
./scripts/compile-camera-branches.sh
```

`check-policy.sh` 需要 Node.js 24、C++17 編譯器、ripgrep、jq。`compile-camera-branches.sh` 另外用暫存的合成設定編譯 3 個相機啟用分支，**不連 Wi-Fi、不上傳到板子**，避免只編譯到預設鎖定的空殼。CI 與主機測試使用的值不代表合法實體接線或可用憑證。

## 證據與來源

- [驗證紀錄與實機待辦](verification.md)
- [來源、API 更新與舊教材遷移](source-map.md)
- [相機 MQTT 接收器](../../scripts/camera-receiver.mjs)

BLE UUID／RSSI 不用來判定真人身分或出席。Base64 與 CRC 分別負責編碼和完整性檢查，不提供加密或認證。這些界線直接影響學生如何判讀實驗結果。
