# Conversation Handoff

最後更新：2026-08-31（Asia/Taipei）

## 目前原則

本專案正在建立 AB143 與 ESP32 能源監測教材的公開 GitHub 版本。這是範例與操作文件庫，不是完整出版課本的鏡像。

## 已確定的決策

- 以 AB143 的課程骨架為基礎。
- 所有開發板設定統一為 `ESP32 Wrover Module`。
- Arduino CLI FQBN 統一為 `esp32:esp32:esp32wrover`。
- 相機單元仍使用 Wrover FQBN，程式內選擇 `CAMERA_MODEL_AI_THINKER`。
- 第二篇包含 1602 LCD 與 OLED。
- ThingSpeak、Google Sheets 與 MQTT 都放在第三篇。
- Node-RED 獨立為第四篇。
- 相機、Bluetooth/BLE 與多工整合為第五篇。
- 能源監測為第六篇整合專題。

## 來源

- AB143 課本與歷史範例：Google Drive `1_ESP32教學資料/AB143：ESP32使用C`
- Wrover/NMK99 教學範例：Google Drive `1_ESP32教學資料/NMK99`
- 能源監測來源：`esp32-mqtt-energy-meter`

上述來源不可整批直接推送到公開 GitHub。每個範例必須通過支援性、授權、憑證與編譯檢查後才能匯入。

## 目前狀態

- Repository 骨架：建立中
- GitHub Repository 目標：`https://github.com/youjunjer/esp32-wrover-iot-course`
- ESP32 Core 驗證基準：`3.3.11`
- 實體板驗證：尚未進行
- 憑證狀態：不得提交真實憑證

## 下一步

1. 建立並推送第一版 GitHub Repository。
2. 建立 Wrover 一般／相機模式腳位對照。
3. 從第一篇開始逐例匯入並以 CLI 編譯。
4. 再依序整合顯示器、雲端、MQTT、Node-RED、相機與能源監測。

## 已知驗證關卡

- 原能源專案使用 GPIO 16/17 作為 PZEM UART2；必須先以指定課程板確認 PSRAM 與實際腳位狀態。
- AI Thinker 相機腳位會與 OLED、WS2812、光敏電阻、SG90 與繼電器的部分舊接線衝突，一般模式與相機模式不宣稱可全部同時運作。
- 本機目前沒有 Arduino CLI 與 ESP32 Core，第一次完整編譯由 GitHub Actions 執行，之後再補本機與實體板驗證。
