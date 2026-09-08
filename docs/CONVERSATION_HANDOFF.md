# Conversation Handoff

最後更新：2026-09-08（Asia/Taipei）

## 目前原則

本專案正在建立 AB143 與 ESP32 能源監測教材的公開 GitHub 版本。這是範例與操作文件庫，不是完整出版課本的鏡像。

## 已確定的決策

- 以 AB143 的課程骨架為基礎。
- 所有開發板設定統一為 `ESP32 Wrover Module`。
- Arduino CLI FQBN 統一為 `esp32:esp32:esp32wrover`。
- 相機單元仍使用 Wrover FQBN，程式內選擇 `CAMERA_MODEL_AI_THINKER`。
- 第二篇調整為 OLED 先行：OLED／I²C 啟動自測 → OLED 基礎 → OLED 診斷畫面 → 感測器 → 1602 LCD → 整合顯示。
- 從 OLED 章開始，每一個 ESP32 執行階段訊息都必須先顯示於 OLED。感測器無資料、網路／雲端／MQTT 失敗、重試、逾時與控制錯誤不得只輸出到 Serial。
- 相機模式仍要使用 OLED；GPIO 21/22 已被相機占用，因此必須先實測替代 I²C 腳位，未驗證前不宣稱相機與 OLED 可同時運作。
- ThingSpeak、Google Sheets 與 MQTT 都放在第三篇。
- Node-RED 獨立為第四篇。
- 相機、Bluetooth/BLE 與多工整合為第五篇。
- 能源監測為第六篇整合專題。
- 第一篇不安排需要觀察數值的感測器；只使用 LED、按鈕、PWM、RGB 與 WS2812 等可直接看見的驗收結果。
- LED 接線注意事項只在第一篇首頁統一說明；後續章節、範例、接線表與排錯內容直接採課程接法，不再重複。
- AB143 圖像只匯入與新版內容相容的單張圖並保留來源記錄；舊工具畫面、錯誤腳位、整頁課本與來源不明圖片不公開。
- 教學操作必須保留真實截圖；預期 OLED 畫面與 DEMO 資料必須標示不是實機結果，不製造假的燒錄、位址或感測成功畫面。

## 來源

- AB143 課本與歷史範例：Google Drive `1_ESP32教學資料/AB143：ESP32使用C`
- Wrover/NMK99 教學範例：Google Drive `1_ESP32教學資料/NMK99`
- 能源監測來源：`esp32-mqtt-energy-meter`

上述來源不可整批直接推送到公開 GitHub。每個範例必須通過支援性、授權、憑證與編譯檢查後才能匯入。

## 目前狀態

- Repository 骨架：已建立並推送
- GitHub Repository：`https://github.com/youjunjer/esp32-wrover-iot-course`（公開）
- ESP32 Core 驗證基準：`3.3.11`
- 2026-09-08 再次核對 Espressif 正式 Release：`3.3.11` 仍是最新穩定版；`4.0.0-alpha1` 不納入正式教材
- 實體板驗證：尚未進行
- 憑證狀態：不得提交真實憑證
- 首次 GitHub Actions：已使用 Arduino CLI 編譯通過兩個基礎範例
- 第一篇：7 章課文、8 個範例、每例接線與可見驗收已完成；[GitHub Actions #33413746238](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33413746238) 已實際編譯通過全部 8 個 Sketch
- 第一篇圖像：已加入 6 張 AB143 課本圖，涵蓋板載 LED、數位訊號、麵包板、LED 極性、PWM 波形與 RGB 腳位；來源記錄位於 `docs/assets/ab143/part1/SOURCES.md`
- 第二篇 OLED 基礎層：已建立自動尋址、基礎繪圖與診斷 DEMO 三章及三個 Sketch；[GitHub Actions Run 33474648376](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33474648376) 已編譯通過全部 11 個 Sketch，真實 Run Summary 截圖已加入第一章
- 第二篇感測器起始層：已新增輸入診斷、PIR 與光敏三章及三個 OLED-first Sketch，使用 GPIO 14／33；[GitHub Actions Run 33478348736](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33478348736) 已編譯通過全部 14 個 Sketch
- 第二篇圖像：已有 3 張 AB143 PIR／光敏單圖、22 張明確標示非實機／非實測的 Repository 原生接線與預期畫面圖，以及 3 張可追溯 Commit／Run 的真實 GitHub Actions 截圖
- 第二篇第 7～12 章：已整理 MQ-2、HC-SR04、停車雷達、DHT11、1602 LCD 與多感測顯示課文及 6 個 OLED-first Sketch；新增 DHT、Unified Sensor 與 LiquidCrystal_PCF8574 鎖定版本；[GitHub Actions Run 33950027529](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33950027529) 已在 Commit `08e2c84` 編譯通過全部 20 個 Sketch，真實 Run Summary 截圖已加入第 7 章
- 第二篇安全決策：MQ-2 AO 與 HC-SR04 Echo 不得將 5V 直接送入 GPIO；1602 I²C 背板不得以 5V 上拉直接連 ESP32；超音波資料無效時蜂鳴器必須靜音
- 第二篇實體驗證：尚未燒錄；OLED 正面、完整接線、GPIO 2 錯誤閃爍碼、各感測器讀值與校正、蜂鳴器失效靜音及 1602 I²C 電壓／顯示照片待補
- 第三篇前五章：正在整理 Wi-Fi 非同步掃描／連線、NTP、CA 驗證 HTTPS、ArduinoJson 7／Open-Meteo 空氣品質模式資料及區網 WebServer；章序調整為先校時再做 HTTPS，禁止以 `setInsecure()` 繞過驗證
- 第三篇安全基準：網路範例只提交 `secrets.example.h`；實機缺少 `secrets.h` 時固定顯示 `CONFIG ERR`。WebServer 開機與斷線預設 `SAFE OFF`，只允許經驗證的 POST 控制，且不得公開到 Internet 或控制市電
- 第三篇驗證狀態：本批程式尚待 GitHub Actions；不得在 CI 完成前標示已編譯通過，也不得把 CI 當成 Wi-Fi、NTP、TLS、API、瀏覽器或 GPIO 實測

## 下一步

1. 以指定課程板依序實測 OLED、GPIO 14 PIR／DHT11、GPIO 33 光敏／MQ-2、HC-SR04、蜂鳴器及 1602，補正面、接線、錯誤碼與校正照片。
2. 實測 OLED 基礎後再決定共用狀態介面；不同模組仍以逐章斷電換線方式驗證，不一次全部整合。
3. 完成第三篇前五章 GitHub Actions 與真實 Run 截圖，再以同一 OLED／憑證規則加入 ThingSpeak、Google Sheets 與 MQTT。

## 已知驗證關卡

- 原能源專案使用 GPIO 16/17 作為 PZEM UART2；必須先以指定課程板確認 PSRAM 與實際腳位狀態。
- AI Thinker 相機腳位會與 OLED、WS2812、SG90、繼電器及部分舊感測器接線衝突；新版光敏一般模式改用 GPIO 33，但仍不宣稱可與相機及 GPIO 21／22 OLED 同時運作。
- 本機目前沒有 Arduino CLI 與 ESP32 Core，第一次完整編譯由 GitHub Actions 執行，之後再補本機與實體板驗證。
- verified HTTPS 使用的根憑證是公開信任錨，不是秘密；但伺服器憑證鏈可能變更，若 OLED 顯示 TLS 錯誤，需先核對可信時間與服務端最新憑證鏈，不能改用 `setInsecure()`。
- Open-Meteo Air Quality API 是 CAMS 模式預報／估算資料，不是課程板或現地測站的即時量測；需保留來源標示、控制請求頻率，且不得用該值直接驅動安全關鍵設備。
- 教室 WebServer 使用明文 HTTP，只能放在隔離或可信任區網。Digest 與 CSRF 可降低誤操作，但不等於傳輸加密。
