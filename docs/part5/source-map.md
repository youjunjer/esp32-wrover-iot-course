# 第五篇來源與 API 核對

核對日期：2026-09-12。章序依本專案 `docs/course-map.md`；本次依官方公開 API 重新撰寫程式與課文，未匯入 AB143 完整頁面、舊專案憑證或第三方壓縮網頁。

| 主題 | 官方來源 | 本次處理 |
|---|---|---|
| Bluetooth Classic | [Core 3.3.11 BluetoothSerial.h](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/BluetoothSerial/src/BluetoothSerial.h)、[官方 API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/bluetooth.html) | `enableSSP()`、`begin()`、`hasClient()`；不搬固定四位 PIN 或 Serial-only echo |
| 手機相容性 | [Apple External Accessory](https://developer.apple.com/documentation/externalaccessory)、[Apple Core Bluetooth](https://developer.apple.com/documentation/corebluetooth) | SPP 與 BLE 分開教學，不宣稱一般 iOS BLE App 可連通 SPP |
| BLE 掃描／廣播 | [BLEScan.h 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/BLE/src/BLEScan.h)、[BLEAdvertising.h 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/BLE/src/BLEAdvertising.h)、[BLEBeacon.h 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/BLE/src/BLEBeacon.h) | 核對同步指標與非同步 bool overload，選擇可判斷啟動失敗的 API、清理結果；公開課堂資料格式，不列印陌生裝置識別 |
| FreeRTOS | [ESP-IDF 5.5 FreeRTOS 文件](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/system/freertos_idf.html) | Stack 以 bytes 表示；Queue／Event Group、核心 affinity、短 critical section |
| 相機 | [CameraWebServer 3.3.11](https://github.com/espressif/arduino-esp32/tree/3.3.11/libraries/ESP32/examples/Camera/CameraWebServer)、[esp32-camera](https://github.com/espressif/esp32-camera) | 核對 AI Thinker 腳位、`pin_sccb_*`、PSRAM、JPEG、framebuffer 歸還；新增 OLED 預檢 |
| 區網 HTTP | [WebServer.h 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/WebServer/src/WebServer.h) | Digest、長度限定與短串流；頁面重新撰寫，不提交官方壓縮資產 |
| MQTT | [arduino-mqtt v2.5.3](https://github.com/256dpi/arduino-mqtt/tree/v2.5.3) | 延續第三篇的 CA 驗證 TLS、QoS 1、非 retained；新增分段合約 |
| 接收 CLI | [Mosquitto 官方手冊](https://mosquitto.org/man/mosquitto_sub-1.html) | 使用 2.1+ `-o` 私人設定與 `%j` JSON envelope，接收器檢查 retain／Topic |

Core 的鎖定 Tag 是編譯基準；latest 文件可能持續變更。沒有更新 Core 或加入舊版 BLE／相機函式庫以迴避 API 問題。

## 驗證界線

主機測試的 Beacon、CRC、分段影像是合成資料；沒有實際手機配對、BLE 無線掃描、PIR 電位、相機影像或 MQTT Broker 成功紀錄。相機腳位程式檢查只排除已知衝突，板上走線、上拉、啟動電位、供電與共存必須現場驗證。
