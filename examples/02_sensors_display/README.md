# 第二篇：OLED、感測器與顯示器

Codex 輔助實機開發時不一定能直接取得序列監控畫面，因此本篇先完成 OLED，讓後續感測器都有可拍照、可辨識的狀態輸出。

正式教材：

1. [OLED 啟動自測與 I²C 自動尋址](../../docs/part2/01_oled_self_test.md)－已加入可編譯範例
2. [OLED 基礎顯示](../../docs/part2/02_oled_basics.md)－已加入可編譯範例
3. [OLED 可視化診斷畫面](../../docs/part2/03_oled_diagnostics.md)－已加入 DEMO 版型範例
4. [感測器接線與輸入診斷](../../docs/part2/04_sensor_wiring_basics.md)－GPIO 14 數位輸入與 GPIO 33 ADC
5. [PIR 人體感測器與 OLED](../../docs/part2/05_pir_sensor.md)－60 秒暖機、穩定判定與事件計數
6. [光敏電阻、ADC 校正與 OLED](../../docs/part2/06_light_sensor.md)－預設 `UNCAL`，實測後才啟用明暗分級
7. [MQ-2 氣體感測器](../../docs/part2/07_mq2_gas_sensor.md)－5V 加熱、AO 安全分壓、暖機與相對門檻
8. [HC-SR04 超音波測距](../../docs/part2/08_ultrasonic_distance.md)－Echo 限壓、30 ms 逾時與距離狀態
9. [蜂鳴器與倒車雷達](../../docs/part2/09_parking_radar.md)－Core 3.x LEDC、距離分級與失效靜音
10. [DHT11 溫濕度](../../docs/part2/10_dht11_sensor.md)－2 秒讀取、`NaN` 與資料過期
11. [1602 LCD 與 DHT11](../../docs/part2/11_lcd1602.md)－I²C 位址、電壓安全與 OLED 診斷
12. [多感測顯示整合](../../docs/part2/12_multisensor_display.md)－DHT11、光敏電阻、OLED 多頁與 1602 LCD 對照

從 OLED 章開始，每一個 ESP32 執行階段訊息都必須先呈現在 OLED，包括暖機、未校正、原始值、連線失敗、重試、資料逾時與錯誤碼；Serial 只能同步複製。前六個範例已由 [GitHub Actions Run 33478348736](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33478348736) 以鎖定工具鏈驗證編譯，連同第一篇共 14 個 Sketch。第 7～12 個範例已納入編譯清單，但本批 CI 尚待執行。所有實體接線、OLED 畫面與感測結果仍需另行拍照驗證。
