# BLE 掃描、Beacon 與訊號觀察

用兩片 Wrover 進行發送與掃描，或以能顯示廣播原始資料的 BLE 手機工具觀察 Beacon。兩個 Sketch 分開燒錄；本章不把 Classic SPP 與 BLE 同時塞進同一支程式。

## 實驗步驟

1. OLED 仍接 SDA 21／SCL 22，先將 `04_ble_beacon` 燒入發送板。`ADV STARTED` 只表示 API 啟動成功；收到廣播仍須另一台裝置確認。
2. 將 `03_ble_scan` 燒入接收板。每輪被動掃描 2 秒，顯示結果 3 秒，再清理結果開始新一輪。
3. 接收板應顯示 `LAB SEEN`、符合課堂格式的封包數及最後 RSSI。關閉發送板，下一輪應變成 `NO DATA`。
4. 改變距離、板子方向及中間遮蔽物；每種條件記錄至少 10 輪 RSSI。不能直接把某個 dBm 門檻寫成固定公尺數。

## 廣播格式

這個教學 Beacon 使用 iBeacon 格式的 manufacturer data：Apple company ID `0x004c`（線上 little endian 為 `4c 00`）、`02 15`、16 bytes UUID、big endian Major／Minor 與 1 byte measured power。

公開課堂 UUID 為 `71473d62-a152-4c66-8e20-6c6162303031`，Major 1、Minor 1。`-59 dBm` 的 measured power 是**未校正的教學占位值**，不是這片板在一公尺實測值，不用它推算距離。這些值只是教材識別，沒有廠商認證或身分驗證意義。

接收程式檢查完整 25 bytes 長度、格式、UUID、Major 與 Minor；不列印 MAC、附近裝置名稱或其他人的 Beacon。顯示的是符合條件的**封包次數**，不是人數，也不是唯一裝置數。

## 新版 API 與記憶體

Core 3.3.11 的同步 `BLEScan::start()` 回傳 `BLEScanResults *`，而且啟動失敗也可能取得非空指標，不能用空指標判斷成敗。本例採用非同步、回傳 bool 的 overload 檢查啟動，主迴圈輪詢完成狀態，超過 4 秒則停止並顯示 `SCAN TIMEOUT`。callback 只更新受短 critical section 保護的計數／RSSI；OLED 由 `loop()` 更新。每輪 `clearResults()` 釋放掃描結果，避免長時間掃描持續累積。

`setActiveScan(false)` 不額外向附近裝置要求 scan response。Interval 100 ms、Window 60 ms；兩者分別控制掃描週期與週期內開啟接收的時間。

## 應用討論與驗收

`SCAN ERR` 是掃描 API 失敗，`NO DATA` 是該輪沒有匹配廣播，不能合併為同一種故障。廣播 UUID 可被複製、RSSI 會受人體和環境影響；點名系統還需要登入綁定、同意、去重與防重播設計。本章只示範訊號觀察，不產生出席紀錄或追蹤陌生人。

驗收至少涵蓋發送、接收、關閉發送板、錯誤 UUID 被忽略，以及持續掃描後 OLED 仍可更新。

## 編譯、燒錄與回報

範例：[`03_ble_scan`](../../examples/05_camera_ble_multitasking/03_ble_scan/README.md)。

範例：[`04_ble_beacon`](../../examples/05_camera_ble_multitasking/04_ble_beacon/README.md)。

在 Repository 根目錄使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11 與鎖定函式庫。請先看[第五篇首頁](README.md)的命令與驗證邊界，燒錄後保留 OLED 正面、完整接線和板上操作結果。OLED 初始化失敗的 GPIO 2 閃爍碼見首頁。
