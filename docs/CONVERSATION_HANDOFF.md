# Conversation Handoff

最後更新：2026-09-01（Asia/Taipei）

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

## 來源

- AB143 課本與歷史範例：Google Drive `1_ESP32教學資料/AB143：ESP32使用C`
- Wrover/NMK99 教學範例：Google Drive `1_ESP32教學資料/NMK99`
- 能源監測來源：`esp32-mqtt-energy-meter`

上述來源不可整批直接推送到公開 GitHub。每個範例必須通過支援性、授權、憑證與編譯檢查後才能匯入。

## 目前狀態

- Repository 骨架：已建立並推送
- GitHub Repository：`https://github.com/youjunjer/esp32-wrover-iot-course`（公開）
- ESP32 Core 驗證基準：`3.3.11`
- 實體板驗證：尚未進行
- 憑證狀態：不得提交真實憑證
- 首次 GitHub Actions：已使用 Arduino CLI 編譯通過兩個基礎範例
- 第一篇：7 章課文、8 個範例、每例接線與可見驗收已完成；[GitHub Actions #33413746238](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33413746238) 已實際編譯通過全部 8 個 Sketch

## 下一步

1. 建立第二篇 I²C、OLED 基礎與 OLED 診斷畫面範例。
2. 以 OLED 可視化狀態為基準，再逐項整合感測器與 1602 LCD。
3. 再依序整合雲端、MQTT、Node-RED、相機與能源監測。

## 已知驗證關卡

- 原能源專案使用 GPIO 16/17 作為 PZEM UART2；必須先以指定課程板確認 PSRAM 與實際腳位狀態。
- AI Thinker 相機腳位會與 OLED、WS2812、光敏電阻、SG90 與繼電器的部分舊接線衝突，一般模式與相機模式不宣稱可全部同時運作。
- 本機目前沒有 Arduino CLI 與 ESP32 Core，第一次完整編譯由 GitHub Actions 執行，之後再補本機與實體板驗證。
