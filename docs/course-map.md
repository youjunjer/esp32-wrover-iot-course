# 課程地圖

本教材以 AB143 為基礎，整合 ESP32 Wrover、雲端、MQTT、Node-RED、相機、Bluetooth、多工與能源監測範例。完整課文會依本地圖逐章整理，不直接複製舊版課本。

## 共通原則

- 所有章節使用 `ESP32 Wrover Module`，FQBN 為 `esp32:esp32:esp32wrover`。
- 第二篇先完成 OLED 啟動自測與可視化診斷，再開始感測器。
- 從 OLED 章開始，每個 ESP32 執行階段訊息都必須在 OLED 顯示；序列輸出只能作為同步副本。
- 相機章仍選 Wrover，僅在程式內使用 `CAMERA_MODEL_AI_THINKER`。
- 舊 API、已停止服務及含憑證的舊範例不直接移植。

## 第一篇：ESP32 與 Arduino C 基礎

1. 認識物聯網與 ESP32 Wrover：架構、GPIO、一般模式與相機模式。
2. 開發環境與第一支程式：Arduino IDE、ESP32 Core、Arduino CLI、板型、序列埠、編譯與燒錄。
3. Arduino C 程式結構：`setup()`、`loop()`、註解、可見硬體輸出與執行順序；Serial 只作輔助概念。
4. 變數、資料型態與運算：整數、浮點數、布林、字串、常數與運算子。
5. 判斷、迴圈與函式：`if`、`switch`、`for`、`while`、函式與陣列。
6. 數位輸入與輸出：LED、按鈕、輸入防彈跳與紅綠燈。
7. PWM 與全彩 LED：新版 LEDC PWM、RGB LED 與 WS2812；需要數值的類比感測移到第二篇 OLED 之後。

## 第二篇：OLED、感測器與顯示器

1. OLED 啟動自測：I²C、SDA／SCL、自動尋址、初始化與 LED 備援錯誤碼。
2. OLED 基礎顯示：偵測位址、字型、文字、圖形與畫面更新。
3. OLED 可視化診斷：`BOOT`、`INIT`、`OK`、`ERR`、錯誤碼與即時數值版型。
4. 感測器接線基礎：電壓、共地、上拉電阻及數位／類比訊號。
5. PIR 人體感測器：來客報知、狀態顯示與誤觸發處理。
6. 光敏電阻：亮度量測、校正、明暗分級與 OLED 顯示。
7. MQ-2 氣體感測器：暖機、門檻、警示狀態與 OLED 顯示。
8. 超音波測距：Trigger、Echo、距離換算、逾時及 OLED 顯示。
9. 蜂鳴器與倒車雷達：音調、距離分級及畫面／聲音警示。
10. DHT11 溫濕度：讀值間隔、`NaN`、錯誤處理與 OLED 顯示。
11. 1602 LCD：I²C 轉接板、文字顯示、自訂字元及 DHT11 整合。
12. 多感測顯示整合：DHT11、光敏、OLED 多頁資訊與 1602 LCD 對照。

## 第三篇：Wi-Fi、雲端與 MQTT

1. Wi-Fi 掃描與連線：訊號強度、連線狀態、重連與憑證分離。
2. HTTPClient：HTTP GET、狀態碼、回應內容與錯誤處理。
3. JSON 與公開資料 API：ArduinoJson、欄位解析與空氣品質資料。
4. NTP 網路校時：時區、日期時間、同步狀態與 OLED 時鐘。
5. ESP32 網頁伺服器：顯示感測資料並控制 LED 或設備。
6. ThingSpeak：資料上傳、Channel、Field、更新頻率與圖表。
7. Google Sheets：Apps Script Web App、資料新增、讀取及 OLED 顯示。
8. MQTT 基礎：Broker、Client、Topic、QoS、Retain 與 Client ID。
9. MQTT Publish／Subscribe：發布感測資料、接收命令與斷線重連。
10. MQTT JSON Payload：封裝溫度、濕度、亮度與設備狀態。
11. MQTT 遠端控制：紅黃綠燈、繼電器與 SG90，每組使用獨立 Topic。

## 第四篇：Node-RED

1. Node-RED 環境與資料目錄：啟動、專案目錄、節點與版本管理。
2. Flow、Node、Wire 與 Message：`msg.payload`、流向與基本除錯。
3. 基礎節點：Inject、Debug、Change、Switch 與 Function。
4. Node-RED 與 MQTT：MQTT In／Out、Broker、Topic 與 Client ID。
5. JSON 與資料處理：解析、欄位轉換、驗證與異常值過濾。
6. Dashboard：即時數值、儀表、狀態燈與歷史曲線。
7. 雙向控制與維護：LED、繼電器、SG90、Flow 備份及憑證移除。

## 第五篇：相機、Bluetooth 與多工

1. Bluetooth Classic：文字傳輸、手機連線與序列通訊。
2. Bluetooth 感測與控制：溫濕度、雙向指令、LED 與 SG90。
3. BLE 與 Beacon：掃描、裝置資訊、距離判斷與點名應用。
4. FreeRTOS Task：建立工作、延遲、優先權與共用資料。
5. ESP32 雙核心：顯示執行核心、工作分配及單／雙核心比較。
6. AI Thinker 相機腳位：Wrover 板型、相機模式與腳位衝突。
7. CameraWebServer 與拍照：串流、解析度、拍照及 Base64。
8. 相機整合：PIR 觸發、MQTT 影像及相機／感測／網路多工。

## 第六篇：智慧能源監測專題

1. 市電安全與 PZEM-004T：原理、低壓端接線與斷電操作。
2. UART2 與 PZEM 通訊：腳位、鮑率、逾時與通訊診斷。
3. 能源資料解析與驗證：電壓、電流、功率、電量、頻率、功率因數、CRC 與資料新鮮度。
4. PZEM 與 OLED：能源資料、連線狀態及多頁顯示。
5. 能源 MQTT：JSON Payload、獨立 Topic、Client ID 與資料發布。
6. 繼電器與安全啟動：低電位觸發、預設關閉及無負載測試。
7. SG90 與環境感測：伺服馬達、DHT11、光敏電阻與 NTP。
8. Node-RED 能源 Dashboard：即時儀表、歷史曲線與設備控制。
9. 完整整合與驗收：OLED、MQTT、感測、控制、安全及異常復原。
