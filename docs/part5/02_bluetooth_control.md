# 藍牙溫溼度與 LED／SG90 控制

延續文字通道，讓手機收到 DHT11 數值並下達有限的控制指令。所有狀態仍以 OLED 為主。

## 接線

| 裝置 | 一般模式腳位 |
|---|---|
| OLED | SDA 21、SCL 22 |
| DHT11 DATA | 14，電位相容與接線依第二篇 |
| LED | 4，依第一篇統一接法 |
| 現場允許按鈕 | 13 → 按鈕 → GND，使用 INPUT_PULLUP |
| SG90 訊號 | 5，適當獨立 5V 電源並共地 |

GPIO 5 的外接線不能在上電時強制錯誤啟動電位。初次測試卸除伺服負載；本章不接繼電器或市電。這些一般模式接線不能搬到相機模式。

## 操作

1. 開機確認 LED 關閉、SG90 尚未附接，OLED 為 `SAFE OFF`。
2. 用 SPP 終端連 `WROVER-CLASS-CONTROL`。約每 2 秒收到 `T=... ,H=...` 型式的當次有效讀值（實際字串沒有額外空格）。OLED 同步顯示溫溼度；失敗時回覆 `DHT NO DATA`。
3. 未按住 GPIO 13 時送 `LED ON` 或 `SERVO 90`，應顯示 `HOLD BTN13` 並保持關閉。
4. 按住按鈕送出指令；放開按鈕、斷線或最後有效控制指令超過 5 秒，都讓 LED 關閉、SG90 detach。
5. `OFF` 不需要按鈕，可隨時執行。

| 指令（結尾 LF） | 行為 |
|---|---|
| `LED ON`／`LED OFF` | 設定 GPIO 4，仍需按住按鈕 |
| `SERVO 0`～`SERVO 180` | 限定整數角度；禁止負值、小數、附加文字 |
| `OFF` | LED 關閉、停止輸出伺服 PWM |

`detach()` 只停止 PWM，不保證機械回到某個位置，也不切斷伺服電源。Just Works 不驗證人員身分；現場按鈕只是讓老師能控制這次低風險示範的允許時段。

## 錯誤與驗收

`CMD ERR`、`RX ERR`、`RX TIMEOUT` 都先關閉輸出。`SERVO ERR` 代表 PWM 附接失敗；`LEASE/BTN OFF` 代表逾時或按鈕放開。DHT 第一次尚無有效讀值為 `DHT NO DATA`，讀取失敗為 `DHT READ ERR`，最後有效值超過 10 秒為 `DHT STALE`；畫面不把失效數值當成新數據。

分別驗證放開按鈕、停止發送、手機斷線、非法角度和半行逾時。記錄 OLED 與實際 LED／伺服行為；只收到 `CMD OK` 不能取代輸出驗收。

## 編譯、燒錄與回報

範例：[`02_bluetooth_control`](../../examples/05_camera_ble_multitasking/02_bluetooth_control/README.md)。

在 Repository 根目錄使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11 與鎖定函式庫。請先看[第五篇首頁](README.md)的命令與驗證邊界，燒錄後保留 OLED 正面、完整接線和板上操作結果。OLED 初始化失敗的 GPIO 2 閃爍碼見首頁。
