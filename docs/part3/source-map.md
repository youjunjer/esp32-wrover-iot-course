# 第三篇來源與遷移記錄

## 來源範圍

本篇以 AB143 第三版的 Wi-Fi、HTTP、JSON、ThingSpeak、Google Sheets、網頁伺服器與 MQTT 教學脈絡為基礎，參考 NMK99 的 `WebServer`／AP、ThingSpeak 與 MQTT 實驗，以及 `esp32-mqtt-energy-meter` 的 OLED、Wi-Fi、NTP、MQTT、感測資料與控制整合經驗。舊程式只用來辨識學習目標、既有服務合約與常見錯誤，不直接複製。

## 逐章來源對照

| 新章節 | 可沿用來源 | 保留的學習目標 | 必須淘汰或重寫 |
|---|---|---|---|
| 1. Wi-Fi 掃描與連線 | AB143 三版印刷 P106～108、歷史範例 `P107(WiFi掃描).txt`；能源專案 `10_mqtt_oled`／`11_mqttgoio` 的 Wi-Fi 狀態 | STA／AP 概念、掃描數量、SSID、RSSI、加密狀態、連線與重試 | 舊 IDE 截圖、Serial-only、顯示真實 SSID、永久等待、未釋放掃描結果，以及課本表格中的舊誤寫 |
| 2. NTP 網路校時 | 能源專案 `08_oled_dht_ntp` 的 `setupTime()`、`configTzTime()` 與 `getLocalTime()`；`20_oled_show` 的時間表頭 | 時區、同步、格式化時間、失敗回傳與資料年齡 | 假定固定時間永遠有效、只顯示 `--:--` 而不說原因、未顯示同步／重試進度，以及以編譯成功當成 NTP 實測 |
| 3. verified HTTPS | AB143 三版印刷 P109～112、歷史範例 `P110(WiFi抓取PM2.5).txt` 的 GET 流程；能源專案 `09_oled_npt_page` 的 `HTTPClient` 結構 | DNS／TLS／HTTP 分層、`begin`、`GET`、狀態碼、回應長度與 `end` | 明文 HTTP、固定 API Key、`setInsecure()`、把完整 payload 或私人 URL 顯示到 OLED／Serial，以及無上限等待 |
| 4. ArduinoJson 與 Open-Meteo AQI | AB143 三版印刷 P113～118、歷史範例 `P116(WiFi抓取並JSON解析).txt`；能源專案 `09_oled_npt_page` 的 AQI 頁面 | JSON object／array、欄位選取、空氣品質資料與更新時間 | ArduinoJson 6 舊截圖、`DynamicJsonDocument` 舊式教法、`i <= size()` 越界、手工字串切 JSON、忽略 `DeserializationError`、舊或帶金鑰端點及未標示 AQI 制度 |
| 5. ESP32 WebServer | AB143 三版印刷 P172～184、歷史範例 `P174`／`P177`／`P183`；NMK99 `進階範例/14_WiFiAP` | 區域網路服務、分組 `.local` 主機名、HTTP route、網頁顯示、設備控制與 404 | `WiFiServer` 手工搜尋 request 字串、Serial-only、舊 GPIO 接線、明文保存／列印 Wi-Fi 密碼、無驗證的公網暴露與永久 AP 迴圈 |
| 6. ThingSpeak | AB143 歷史範例 `P130(ThingSpeak溫濕度圖表).txt`；NMK99 `積木程式範例/09_dht_thingspeak`、`10_dht_thingspeak_oled` | Channel、Field、Write API Key、更新間隔、溫濕度／光敏資料與圖表 | 明文 HTTP、程式內嵌 Key、把 Key 放進完整 GET URL、10 秒連發、Serial-only、永久 Wi-Fi 等待，以及只看 HTTP 200 而不檢查 Entry ID |
| 7. Google Sheets | AB143 歷史範例 `GoogleSheet紀錄溫溼度.txt` | 沿用教師已開放 GAS `/exec` 與 `type=insert`、`dateInclude`、`sheetId`、`sheetTag`、`data` query 合約，寫入 URL-encoded 溫度／濕度 CSV | 不新增 `Code.gs`、不建立或重新部署 GAS；淘汰 GPIO 23、SimpleDHT、Serial-only、永久 Wi-Fi 等待、`setInsecure()`、不讀回應及 10 秒連發 |
| 8. MQTT 基礎 | AB143 `P273(MQTT直播).txt`；能源專案 `10_mqtt_oled`、`11_mqttgoio`、`11_mqttgovip`與 `13_mqtt` | Broker、Client、Topic、Client ID、連線狀態與 OLED 錯誤 | 未加密 1883、固定共用 Topic／Client ID、內嵌帳密、無 NTP／CA 驗證與已停止維護的 PubSubClient 依賴 |
| 9. MQTT Publish／Subscribe | AB143 `P212(MQTT 溫濕度及綠燈控制).txt`、`P273(MQTT直播).txt`；能源專案 `10_mqtt_oled`、`11_mqttgoio`與 `13_mqtt` | Publish／Subscribe 方向、主題命名、訊息流向、斷線重連與可見狀態 | 用萬用字元接收控制、對命令使用 Retain、在 callback 內再 publish／subscribe、無長度上限、多組共用 Topic 與只靠 Serial 驗收 |
| 10. MQTT JSON Payload | 能源專案 `13_mqtt`、`16_energy_mqtt`、`17_energy_mqtt_relay`、`18_energy_mqtt_relay_sg90`、`19_energy_mqtt_relay_sg90_sensor`與 `20_oled_show` | `temp`、`humi`、`light` 等感測欄位、發布週期與 OLED 狀態 | 無 schema 版本、無裝置／時間／序號、無 Payload 大小上限、無 `NaN`／範圍／新鮮度檢查，以及將舊值持續當作新資料發布 |
| 11. MQTT 遠端控制 | AB143 `P212(MQTT 溫濕度及綠燈控制).txt`、`P226(MQTT SG90).txt`；能源專案 `17_energy_mqtt_relay`、`18_energy_mqtt_relay_sg90`、`19_energy_mqtt_relay_sg90_sensor`與 `20_oled_show` | 紅黃綠燈、低電位觸發繼電器、SG90、命令接收與狀態回報 | 繼電器不安全啟動、無對應裝置命名空間、無命令 schema／時效／重播防護，失去網路後仍保持負載，以及未隔離伺服電源、遠端文字引導市電操作 |

## AB143 可沿用與不沿用內容

### 可沿用

- 印刷 P106～108：Wi-Fi 模式、掃描、RSSI、頻道與加密概念。
- 印刷 P109～112：HTTPClient 的請求生命週期及 HTTP 狀態碼概念。
- 印刷 P113～114：JSON object、array 與巢狀欄位的概念，可重新繪製為 Repository 原生圖。
- `P130(ThingSpeak溫濕度圖表).txt`：Channel、Field、感測資料上傳與圖表對照的學習目標。
- `GoogleSheet紀錄溫溼度.txt`：教師既有 GAS 的 query 欄位名稱、溫濕度 CSV 與 URL encoding 合約。
- 印刷 P172～175：ESP32 在區域網路提供頁面，以及「route 對應設備動作」的教學關係，可重新繪製資料流圖。
- `P212(MQTT 溫濕度及綠燈控制).txt`、`P226(MQTT SG90).txt` 與 `P273(MQTT直播).txt`：Broker、Topic、Publish／Subscribe、感測資料與遠端控制的學習順序。

### 不直接沿用

- P107、P114 的舊 Arduino IDE／Library Manager 畫面；目前課程以 CLI 為主，操作截圖必須重新實際擷取。
- P108、P117、P174 等 Serial Monitor 畫面；本課程從 OLED 章起以 OLED 為主要可觀測介面。
- P109～111、P116～117 中的既有端點與金鑰，不論是否仍有效都不複製、不測試、不公開。
- `DynamicJsonDocument`、未檢查反序列化結果及 `i <= records.size()` 等舊寫法。
- P172 的舊板型／GPIO 接線、P174～178 的私人區網 IP 截圖及用字串包含判斷直接控制 GPIO 的流程。
- ThingSpeak 與 MQTT 舊範例中內嵌的 Wi-Fi、API Key、Broker 帳密、固定 Topic 與明文端點。
- Google Sheets 範例的 GAS URL 與 Sheet ID 不進入公開 Repository；本章只保留已有 query 合約，不由本教材新增 `Code.gs` 或提供重新部署步驟。

## NMK99 可沿用與不沿用內容

### 可沿用

- `進階範例/14_WiFiAP/02_wifiAP.ino` 的 `WebServer server(80)`、`server.on()`、GET／POST handler 與 AP／STA 模式切換概念。
- `進階範例/14_WiFiAP/03_spiffs.ino` 的「設定需持久保存」問題意識，但不沿用其明文格式。
- `積木程式範例/09_dht_thingspeak`、`10_dht_thingspeak_oled` 的 DHT11 資料上傳與「網路結果也要在 OLED 顯示」方向。
- `積木程式範例/12_dht_mqtt` 的溫濕度與 MQTT 學習主題；只保留教學目標，不複製其固定網路或 Broker 設定。

### 不直接沿用

- HTML 密碼欄使用一般文字輸入、在 Serial 列印 SSID／密碼、以純文字存入 SPIFFS，以及程式內建的示範憑證。
- `SPIFFS.begin(true)` 在掛載失敗時可能格式化、連線失敗直接重新啟動，以及 `while (true) server.handleClient()` 永久佔用流程。
- Blocklyduino 產生的固定 Wi-Fi／ThingSpeak／MQTT 值、明文 HTTP、永久連線等待與版本綁定程式；這些只做歷史比較。
- NMK99 的 Google Docs／Slides 指標檔不是可直接提交的程式或截圖；若日後引用，必須另行檢查內容、權利與敏感資料。

## 能源監測專案可沿用與不沿用內容

### 可沿用

- `08_oled_dht_ntp/08_oled_dht_ntp.ino`：`connectWiFi()`、`configTzTime()`、`getLocalTime()` 與時間格式化的基本結構。
- `09_oled_npt_page/09_oled_npt_page.ino`：HTTPS／AQI／多頁 OLED 的整合目標，以及把更新週期與畫面頁面分離的方向。
- `10_mqtt_oled`、`11_mqttgoio`：`WIFI CONNECT`、`WIFI OK`、`WIFI FAIL`、重試間隔與 OLED 狀態切換。
- `13_mqtt`、`16_energy_mqtt`：裝置發布感測／能源資料與區分資料主題的整合目標。
- `17_energy_mqtt_relay`、`18_energy_mqtt_relay_sg90`、`19_energy_mqtt_relay_sg90_sensor`：繼電器、SG90、DHT11 與光敏控制的分階段整合順序。
- `20_oled_show/20_oled_show.ino`：在連線前先顯示狀態、連線後顯示 IP，以及表頭持續呈現 Wi-Fi 與時間狀態。

### 不直接沿用

- `09_oled_npt_page/09_oled_npt_page.ino` 曾含嵌入程式的敏感端點參數；本記錄刻意不重現其值。建立新教材前應移除公開值並由專案負責人判斷是否需要撤銷或輪替。
- 同檔案使用 `setInsecure()` 且以字串搜尋方式解析 JSON；兩者都不能成為 verified HTTPS／ArduinoJson 章的完成版本。
- 舊範例只把部分 Wi-Fi／HTTP 錯誤寫到 Serial、保留預設感測數字，且沒有完整 `AGE`／`STALE` 判定。
- MQTT 舊範例使用已停止維護的 PubSubClient、固定主題與共用 Client ID，且曾將網路與 Broker 設定直接寫入程式；新範例不沿用這些實作。
- 舊感測 Payload 沒有 schema 版本、NTP 校時後的系統時間、序號、長度上限與資料新鮮度；新版改以 ArduinoJson 7 建立有型別與邊界的 Payload。
- 繼電器與伺服馬達舊控制流程缺少安全啟動、指令時效、開機識別、單調命令序號與輸出租期；新章不把舊控制邏輯直接搬入。
- 部分 U8g2 範例使用 GPIO 13／14 作 I²C；第三篇一般模式仍採 SSD1306 GPIO 21／22，不能直接搬腳位。
- 相機範例的 `app_httpd.cpp` 串流框架太複雜，不用作初學者 WebServer 章來源。

## 現代化決策

### Wi-Fi

- 畫面固定以 `WIFI OLED` 為表頭，依序顯示 `SCAN`、`CONNECT` 與 `ONLINE`；完成掃描後只顯示 AP 數量、目標有無與 RSSI，不在 OLED 顯示 SSID 或完整 IP。課堂照片仍必須檢查是否含可識別網路資訊。
- 連線採有限等待與退避重試，不在 `setup()` 永久卡住。斷線顯示 `LINK LOST`，沒有連線時不啟動後續網路工作。
- 掃描資料用完後釋放結果；每次掃描、連線及重試都有 OLED 進度。

### NTP

- 使用 `configTzTime()` 設定時區與多個 NTP 來源，以 `getLocalTime()` 明確判斷同步是否成功。
- 畫面固定以 `NTP CLOCK UTC+8` 為表頭，顯示 `NTP SYNC`、`TIME OK` 或 `NTP STALE`，並以 `SYNC Ns NET ON` 呈現最後成功同步距今秒數；裝置啟動秒數不能冒充真實日期時間。
- HTTPS 只有在 NTP 系統校時成功後才開始；NTP 未成功時顯示 `TIMEOUT`／`TLS NOT STARTED` 與重試倒數，不跳過憑證檢查。這只是讓憑證有效期檢查可使用合理系統時間，不把一般 SNTP 宣稱為密碼學可信時間。

### verified HTTPS

- 使用 `NetworkClientSecure` 與受信任根憑證／憑證套件驗證伺服器，不使用 `setInsecure()`；`WiFiClientSecure` 只視為相容別名，不作新版教材主名稱。
- OLED 依序顯示 Wi-Fi、NTP、`TLS VERIFY`、HTTP 狀態與 body 長度；Core 將 DNS／TLS／網路底層失敗合併回報時，畫面明確標為 `TLS/NET ERR` 而不猜測單一原因。畫面不顯示完整 URL 或查詢參數。
- 對網路工作設定連線與讀取逾時；任何分支都呼叫 `http.end()` 並保留可診斷錯誤。

### ArduinoJson 與 Open-Meteo

- 依鎖定版本使用 ArduinoJson 7 的 `JsonDocument`，檢查 `DeserializationError` 後才讀欄位。
- 採 Open-Meteo Air Quality API 作無金鑰公開資料練習；請求參數只取所需欄位，座標使用公開教學位置或模糊區域。
- 本章解析 `european_aqi` 並在畫面明確標示 `EAQI`，不能只寫模糊的 `AQI`。
- 檢查必要物件與欄位、型別、單位、有限數值、合理範圍、資料時間及 `AGE`；錯誤時顯示 `JSON ERR`、`FIELD/RANGE ERR`、`UNIT ERR` 或 `SOURCE FUTURE/STALE`，舊資料會明確標為 `AQ DATA STALE`。

### WebServer

- 使用 Core 3.3.11 內建 `WebServer.h`、明確 GET route、`onNotFound()` 與正確狀態碼，不手工搜尋整段 request。
- 設備輸出在伺服器啟動前先進入安全狀態；普通未知 route 只回傳 404，但控制 route 的未授權、缺少參數、錯誤 Token、未知值或不允許方法都會先強制 `SAFE OFF`。
- OLED 顯示 `WEB GPIO13`、每組唯一的 `.local` 主機名、最近 route 及回應狀態；Wi-Fi 斷線時顯示 `NET LOST` 並停止宣稱服務可用。
- 本章只在受信任區域網路教學，不提供連接埠轉送、公網 NAT 或未驗證遠端控制步驟。

### ThingSpeak

- 使用 `https://api.thingspeak.com/update` 與 DigiCert Global Root G2 進行 CA 驗證 HTTPS POST，Write API Key 只放本機 `secrets.h`。程式依該端點的純文字回應解析 Entry ID，不把 JSON 端點的回應格式混入本章。
- DHT11 或光敏資料無效時顯示 `NO DATA` 並阻止上傳。HTTP 200 且 body 是正整數 Entry ID 才認定當次寫入成功，圖表仍要人工核對。
- 正常間隔至少 30 秒並加入 jitter；HTTP 429 採較長退避。服務方案與限制會變動，上課前要以官方說明重新核對。

### Google Sheets 與既有 GAS

- 本章依使用者指定，只沿用 `GoogleSheet紀錄溫溼度.txt` 的既有 GAS `/exec` 及 `type=insert&dateInclude=1&sheetId=...&sheetTag=...&data=...` query 合約。
- 不在 Repository 新增 `Code.gs`，不指導學生建立、改寫或重新部署 GAS；完整 GAS URL、Sheet ID 與工作表名稱依然只存放於本機 `secrets.h`。
- 舊合約沒有裝置 Token、事件 ID 或去重機制，只適用於教師控制的課堂。HTTP 200 後還要到 Sheet 人工確認新列；傳輸中斷時以 `RESULT UNKNOWN` 提醒不能盲目重試。
- Apps Script Content Service 可能導向 `script.googleusercontent.com`；新 ESP32 程式只跟隨一次這個官方 HTTPS 主機，並以 GTS Root R4 驗證兩段 TLS，不使用 `setInsecure()`。

### MQTT TLS、Publish／Subscribe 與 JSON

- 舊範例使用的 PubSubClient 已由其官方 Repository 標示為停止維護；新章改用鎖定的 `MQTT` 2.5.3（arduino-mqtt）與 MQTT 3.1.1。這是現代化決策，不冒充能源專案原本實作。
- 只連接埠 8883 的 TLS Broker；Root CA 由 Broker 維運方或 CA 官方來源提供，上課前核對憑證鏈，不使用明文 1883 或 `setInsecure()`。
- 每組使用獨立 Topic root、Client ID 與 Broker ACL。先在第 8 章驗證 retained online／Last Will offline，第 9 章以不 Retain 的 heartbeat 與 PING／PONG 驗證 Publish／Subscribe，再於第 10 章發布感測 JSON。
- Callback 只收取精確 Topic 的有限長度訊息，實際判斷與回覆回到主迴圈處理。感測 JSON 有 schema 版本、非敏感裝置別名、序號、NTP 時間、型別／範圍與 384-byte 上限；無效或過期資料不發布。

### MQTT 安全控制

- 裝置啟動、網路／時間／TLS／MQTT／訂閱失敗、錯誤命令、ACK／status 發布失敗或連線中斷時，紅黃綠燈、SG90 與低電位觸發繼電器都回到 `SAFE OFF`。
- 控制端先從 retained status 取得本次 `boot_id`；命令還必須通過 schema、`cmd_seq` 單調遞增、NTP 校時後的系統時間與 5～30 秒 TTL，收到時至少要剩 2 秒。啟用輸出的 lease 只使用 `ts + ttl` 尚餘的時間且最長 30 秒，到期自動安全關閉。
- 簡化 callback 無法辨識收到訊息的 retained flag，因此連線後先清除本裝置精確 `cmd` Topic 的舊 retained 訊息，再訂閱並啟用 `boot_id`、TTL 與 `cmd_seq` 防護。控制端仍不得 Retain 命令。
- SG90 使用適當的獨立 5V 電源並與 ESP32 共地；停止 PWM 不代表機械結構已回到安全位置。繼電器只在無市電負載狀態驗證，本教材不以遠端文字引導帶電操作。

## 官方技術依據

- Espressif [Arduino-ESP32 3.3.11 Release](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11)：2026-09-08 核對時仍為最新穩定版；4.0.0 alpha 不作正式教材基準。
- ESP32 Core 3.3.11 [Wi-Fi API](https://github.com/espressif/arduino-esp32/blob/3.3.11/docs/en/api/wifi.rst)
- ESP32 Core 3.3.11 [SimpleTime 範例](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino)
- ESP32 Core 3.3.11 [`HTTPClient.h`](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/HTTPClient/src/HTTPClient.h)
- ESP32 Core 3.3.11 [`WebServer.h`](https://github.com/espressif/arduino-esp32/blob/3.3.11/libraries/WebServer/src/WebServer.h)
- ArduinoJson [7.4.3 Release](https://github.com/bblanchon/ArduinoJson/releases/tag/v7.4.3) 與官方 [version 6 升級到 version 7](https://arduinojson.org/v7/how-to/upgrade-from-v6/)
- Let's Encrypt 官方 [ISRG Root X1 PEM](https://letsencrypt.org/certs/isrgrootx1.pem)：本篇 HTTPS 範例的公開信任錨；它不是秘密，但服務端憑證鏈仍可能變更。
- Open-Meteo 官方 [Air Quality API 文件](https://open-meteo.com/en/docs/air-quality-api)
- MathWorks 官方 [ThingSpeak REST API](https://www.mathworks.com/help/thingspeak/rest-api.html) 與 [Channel API Keys 及更新限制](https://www.mathworks.com/help/thingspeak/channel-control.html)
- DigiCert 官方 [Trusted Root Authority Certificates](https://knowledge.digicert.com/general-information/digicert-trusted-root-authority-certificates)：ThingSpeak 範例使用的 DigiCert Global Root G2 來源
- Google 官方 [Apps Script Content Service](https://developers.google.com/apps-script/guides/content) 與 [Google Trust Services PKI](https://pki.goog/repository/)：只用來說明既有 GAS 回應導向與 TLS 信任錨，不是重新部署 GAS 指南
- OASIS [MQTT Version 3.1.1](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- PubSubClient 官方 [README](https://github.com/knolleary/pubsubclient/blob/master/README.md) 的停止維護說明，以及新教材鎖定的 [`MQTT` library 2.5.3](https://github.com/256dpi/arduino-mqtt/tree/v2.5.3)
- ESP32Servo 官方 [Repository](https://github.com/madhephaestus/ESP32Servo)：第 11 章伺服馬達 API 來源；實際版本以 `config/libraries.lock` 為準

## 圖像決策

AB143 的舊 IDE、Library Manager、Serial Monitor、含 API Key 的 URL、真實 SSID、私人區網 IP、舊板型與 Serial-only 畫面不納入公開 Repository。JSON object／array、HTTP route 與 Wi-Fi 模式等概念改以文字或 Repository 原生 SVG 重畫。

本篇十一張 OLED 圖只使用文字與幾何圖元，顯示預定狀態與示意數值，並直接標示「非實機／非實測資料」。它們不是 CLI、GitHub Actions、瀏覽器或實體 OLED 截圖；真實操作畫面只能在完成對應程式後重新擷取，並記錄工具版本、指令、Commit、證據邊界與 SHA-256。

圖檔來源、用途與雜湊記錄在 [`docs/assets/part3/guides/SOURCES.md`](../assets/part3/guides/SOURCES.md)。
