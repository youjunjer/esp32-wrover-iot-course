# OLED 執行狀態與除錯訊息規範

Codex 輔助開發時不保證能直接取得序列監控內容，因此 OLED 是本教材的主要執行狀態介面。從第二篇 OLED 章開始，所有 ESP32 範例都必須遵守本規範。

## 核心規則

1. 所有執行階段訊息先顯示在 OLED，Serial 只能同步複製。
2. 不得在 Wi-Fi、NTP、HTTP、MQTT 或感測器重試期間保持空白畫面。
3. 感測資料失效時不得繼續把舊值當成即時值顯示；應改為 `NO DATA`、`TIMEOUT`、`CRC ERR` 或 `STALE`。
4. 錯誤訊息應保留到狀態改變或使用者確認，不可只閃現幾毫秒。
5. OLED 不顯示密碼、Token、完整 API Key、私人 URL 或其他憑證。
6. 唯一無法使用 OLED 的情況是 OLED 本身初始化失敗；此時使用 GPIO 2 狀態 LED 閃爍碼作為備援，不得只輸出到 Serial。是否為板載 LED 必須依指定課程板確認。

## 建議畫面欄位

128×64 OLED 優先使用四個資訊區：

1. 模組或工作名稱，例如 `WIFI`、`DHT11`、`MQTT`、`PZEM`。
2. 階段或狀態，例如 `BOOT`、`INIT`、`OK`、`RETRY`、`ERR`。
3. 數值、錯誤原因或回應碼。
4. 重試次數、最後更新時間或資料年齡。

## 必須涵蓋的狀態

| 類別 | OLED 訊息範例 |
|---|---|
| 系統 | `BOOT`、`INIT`、`READY`、`ERR` |
| 感測器 | `NO DATA`、`TIMEOUT`、`RANGE ERR`、`STALE` |
| Wi-Fi | `SCAN`、`CONNECT`、`NO AP`、`AUTH ERR`、`RETRY 3`、`ONLINE` |
| NTP | `SYNC`、`TIMEOUT`、`TIME OK` |
| HTTP／雲端 | `SEND`、`HTTP 200`、`HTTP 404`、`HTTP ERR` |
| MQTT | `CONNECT`、`BROKER ERR`、`PUB OK`、`PUB ERR`、`SUB OK` |
| 控制 | `SAFE OFF`、`CMD OK`、`CMD ERR`、`RETAIN BLOCK` |
| PZEM | `UART ERR`、`CRC ERR`、`NO DATA`、`STALE 12s` |

訊息可以縮寫，但每個範例 README 必須列出所使用的代碼與意義。

後續程式應統一透過共用 OLED 狀態函式顯示訊息，再由該函式選擇是否同步複製到 Serial，避免每個範例各自產生不同格式或遺漏 OLED 訊息。共用函式與 OLED 函式庫版本需在第一個 OLED 範例完成實機驗證後加入。

## OLED 啟動自測

OLED 第一個範例應自動尋找常見 I²C 位址並完成初始化，成功後直接顯示偵測到的位址。若找不到 OLED，使用已確認接線的 GPIO 2 狀態 LED 固定閃爍碼表示 `OLED INIT ERR`，避免形成「必須先看 Serial 才知道 OLED 為何沒有畫面」的循環依賴。

## 相機模式

AI Thinker 相機會占用一般 OLED 常用的 GPIO 21/22。相機章仍必須提供 OLED 狀態畫面，但要使用指定課程板上已實測、不與相機衝突的替代 I²C 腳位。完成實機驗證前，不得在教材中宣稱相機與 OLED 可同時使用。

## 提供 Codex 的診斷資料

發生問題時回傳：

1. OLED 正面清晰照片，保留完整訊息與錯誤碼。
2. ESP32、OLED、感測器與電源的完整接線照片。
3. 使用的章節、範例名稱、開發板與 Core 版本。
4. 編譯或燒錄結果；序列輸出若能取得可以附上，但不是必要條件。
