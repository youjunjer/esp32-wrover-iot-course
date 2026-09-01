# 第二篇：OLED、感測器與顯示器

Codex 輔助實機開發時不一定能直接取得序列監控畫面，因此本篇先完成 OLED，讓後續感測器都有可拍照、可辨識的狀態輸出。

正式教材：

1. [OLED 啟動自測與 I²C 自動尋址](../../docs/part2/01_oled_self_test.md)－已加入可編譯範例
2. [OLED 基礎顯示](../../docs/part2/02_oled_basics.md)－已加入可編譯範例
3. [OLED 可視化診斷畫面](../../docs/part2/03_oled_diagnostics.md)－已加入 DEMO 版型範例
4. [感測器接線與輸入診斷](../../docs/part2/04_sensor_wiring_basics.md)－GPIO 14 數位輸入與 GPIO 33 ADC
5. [PIR 人體感測器與 OLED](../../docs/part2/05_pir_sensor.md)－60 秒暖機、穩定判定與事件計數
6. [光敏電阻、ADC 校正與 OLED](../../docs/part2/06_light_sensor.md)－預設 `UNCAL`，實測後才啟用明暗分級

後續規劃：

7. MQ-2 氣體感測器
8. 超音波測距
9. 蜂鳴器與倒車雷達
10. DHT11 溫濕度
11. 1602 LCD 與 DHT11 整合
12. DHT11＋光敏電阻＋OLED 多頁顯示

從 OLED 章開始，每一個 ESP32 執行階段訊息都必須先呈現在 OLED，包括暖機、未校正、原始值、連線失敗、重試、資料逾時與錯誤碼；Serial 只能同步複製。前六個範例已由 [GitHub Actions Run 33478348736](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33478348736) 以鎖定工具鏈驗證編譯，連同第一篇共 14 個 Sketch。實體接線、OLED、PIR、光敏及 GPIO 2 備援閃爍碼仍需另行拍照驗證。
