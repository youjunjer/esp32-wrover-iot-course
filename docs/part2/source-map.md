# 第二篇來源與遷移記錄

## 來源基準

- AB143 第三版印刷 P79：I²C 位址掃描概念。
- AB143 第四版第 3 章 P35：PIR 外觀、腳位與調整旋鈕。
- AB143 第四版第 4 章 P43：四線式光敏模組的 AO／DO 外觀。
- AB143 第四版第 6 章：DHT11、顯示器比較、I²C 與 1602 LCD；保留到後續相符章節再使用。
- AB143 第三版印刷 P51～57：MQ-2 外觀、AO／DO 概念、暖機與相對值觀察；新版不搬 GPIO 36、RGB LED、`analogWrite()` 或 Serial-only 流程。
- AB143 第三版印刷 P61～68：HC-SR04、蜂鳴器及倒車雷達概念；新版不搬 Echo 5V 直連、GPIO 12／17 或舊 PWM API。
- AB143 第三版印刷 P72～83：DHT11、1602 LCD 與整合顯示；新版不搬 GPIO 4、`SimpleDHT`、5V I²C 直連或舊 IDE／Library Manager 畫面。
- AB143 第三版歷史範例 `P55`、`P56`、`P63`、`P67`、`P68`、`P75`、`P82-1`、`P82-2`：只作學習目標與行為比對，程式依目前工具鏈重寫。
- `esp32-mqtt-energy-meter/04_PIR`：GPIO 14 的既有 PIR 一般模式基準；新版移除紅綠燈並加入 OLED 暖機與診斷。
- `esp32-mqtt-energy-meter/06_oled_photo` 與專案硬體表：光敏 GPIO 33 ADC 基準。
- `esp32-mqtt-energy-meter/14_ENERGY_OLED`：Adafruit SSD1306、GPIO 21／22 與課程板旋轉方向。
- `esp32-mqtt-energy-meter/20_oled_show`：目前能源專案的 OLED 狀態與頁面設計。
- `docs/oled-status-standard.md`：OLED 優先、錯誤保留、資料新鮮度與敏感資訊邊界。

## 本次現代化決策

- 使用 `Adafruit SSD1306` 與 `Adafruit GFX`，對齊能源專案主程式，不直接搬用早期 U8g2 教學範例。
- 一般模式固定 SDA GPIO 21、SCL GPIO 22；相機模式另案實測，不宣稱可直接共用。
- 先檢查 `0x3C`、`0x3D`，不以 Serial Monitor 作為唯一掃描結果。
- OLED 無法初始化時使用 GPIO 2 閃爍碼，解決「螢幕壞了卻只能在螢幕顯示錯誤」的循環問題。
- 診斷版型的固定狀態全部標示 `DEMO`，不冒充真實感測器、網路或硬體結果。
- 前六個範例先保持自足；等指定課程板完成第一次 OLED 實測後，再決定共用狀態函式介面。
- PIR 從舊版 GPIO 16／17 與錄放音整合，改為單一模組 GPIO 14；避免 Wrover PSRAM 邊界，也避免第一篇 GPIO 13 按鈕殘留接線衝突。
- 光敏 AO 從舊版 GPIO 36 改為 ADC1 的 GPIO 33，模組使用 3.3V；`analogRead()` 只稱為原始值，不稱 lux。
- Core 3.3.11 在 ADC pin 首次配置前不使用 per-pin attenuation，單一 ADC 範例改用 `analogSetAttenuation(ADC_11db)`；依據為該版本的 [ADC 實作](https://github.com/espressif/arduino-esp32/blob/3.3.11/cores/esp32/esp32-hal-adc.c)。
- PIR 使用 60 秒可見暖機與 150 ms 穩定判定；事件數不是人數。光敏預設 `UNCAL`，本機校正後才允許顯示明暗分級。
- MQ-2 使用 GPIO 33 ADC1；5V 加熱器與 ESP32 共地，AO 先經 10 kΩ／12 kΩ 分壓再進 GPIO 33，DO 不接。即使分壓計算約為 2.73V，仍必須用電表確認；未經標準氣體校正只顯示 `RAW`、`BASELINE`、`REL WARN`／`REL HIGH`，不宣稱 ppm、瓦斯種類、安全或危險濃度。
- HC-SR04 使用 TRIG GPIO 25、ECHO GPIO 14；5V Echo 以 1% 10 kΩ／15 kΩ 分壓至名目 3.00V，或使用合適位準轉換器，接板前仍須量測。`pulseIn()` 設 30 ms 上限，逾時不保留成有效距離。
- 帶驅動級、3.3V HIGH 訊號控制的三線無源蜂鳴器模組使用 GPIO 13，採 ESP32 Core 3.3.11 [LEDC API](https://github.com/espressif/arduino-esp32/blob/3.3.11/docs/en/api/ledc.rst) 的 `ledcAttach()`、`ledcWriteTone()` 與 `ledcWrite(pin, 0)`；開機、超音波逾時或資料無效時都先靜音，必要時以外部下拉確保重設期間不短鳴。裸蜂鳴器或高電流負載不得直接由 GPIO 驅動。
- DHT11 使用 GPIO 14 與 [Adafruit DHT 1.4.7](https://github.com/adafruit/DHT-sensor-library/releases/tag/1.4.7)，讀取間隔至少 2 秒；`NaN` 顯示 `READ ERR`／`NO DATA`，最後成功資料超過時限後顯示 `STALE`，不得假裝仍是即時值。
- 1602 LCD 使用 [`LiquidCrystal_PCF8574` 2.3.0](https://github.com/mathertel/LiquidCrystal_PCF8574/blob/2.3.0/src/LiquidCrystal_PCF8574.h)，先掃描 `0x27`／`0x3F`。OLED 保留為啟動與錯誤介面；LCD 無回應時 OLED 顯示 `LCD NO ACK`。只允許 3.3V 相容背板，或以雙向 I²C 位準轉換器隔離常見 5V 上拉；共用匯流排依 ESP32 Core 3.3.11 [I²C API](https://github.com/espressif/arduino-esp32/blob/3.3.11/docs/en/api/i2c.rst) 設定。
- 多感測整合只合併 DHT11 GPIO 14、光敏 GPIO 33 與 GPIO 21／22 上的 OLED／1602，不把 MQ-2、PIR、超音波與蜂鳴器全部同時留在同一接線。

## 圖像決策

AB143 第四版只有低解析 OLED 商品小圖，沒有符合目前 CLI、Core 3.3.11、Wrover GPIO 21／22 與 OLED 優先診斷流程的操作圖，因此前三章不直接匯入舊 OLED 圖。

本篇使用目前腳位重畫的接線圖、依程式版型產生且明確標示的預期畫面，以及 Commit `9934db9`、`dad0084` 的真實 GitHub Actions 編譯截圖。第 4～6 章另匯入三張相容的 PIR／光敏單圖並建立 `docs/assets/ab143/part2/SOURCES.md`；舊 WROOM 接線與 Serial-only 畫面不沿用。

第 7～12 章重新繪製電壓安全與 OLED-first 圖，而不直接匯入課本中的 MQ-2／HC-SR04 5V 舊接線、舊 IDE 截圖及 1602 5V 直連表。元件商品照、資料表曲線及來源權利不明的圖也不納入公開 Repository；未來若補入課本單圖，必須先確認權利、裁成單一相容圖並記錄頁碼與 SHA-256。

不納入舊 Arduino IDE、Library Manager、Serial Monitor、WROOM 板型、5V I²C 直連及來源不明商品圖。各教學圖的來源、用途與雜湊值記錄在圖檔同目錄的 `SOURCES.md`。
