# Bluetooth Classic 文字傳輸

先用手機與 ESP32 建立 SPP 連線，再送出一行文字並收到回應。這個實驗不需要 Wi-Fi、雲端帳號或 Serial Monitor。

## 接線與操作

使用一般模式：OLED SDA 21、SCL 22，依第二篇的 3.3V 接線。相機不啟用。

1. 編譯、燒錄並重新開機；OLED 應顯示 `WAIT CLIENT`。
2. 在支援 Bluetooth Classic SPP 的 Android 手機或電腦配對 `WROVER-CLASS-TEXT`，再用 SPP 終端建立連線。系統顯示「已配對」不等於 SPP 通道已開啟；OLED 要出現 `CONNECTED`。
3. 終端選擇 LF 換行，送出 `HELLO`，收到 `HELLO CLASS`；送出 `PING`，收到 `PONG`。
4. 中斷終端連線，OLED 顯示 `LINK LOST`。重新連線後輸入緩衝已清空。

iPhone／iPad 的一般 BLE 終端不能直接連這個 SPP Sketch；改做第 3 章的 BLE 掃描／Beacon 實驗。Bluetooth Classic 與 BLE 是不同服務，名稱出現在搜尋結果不代表 App 支援它的協定。

## 程式重點

`BluetoothSerial` 來自鎖定的 ESP32 Core。`enableSSP(false, false)` 使用 Just Works 配對，沒有雙端數字比對；適用現場文字練習，不能當成辨識操作者的憑證。Core 3.x 不沿用舊的四位固定 PIN 流程。3.3.11 的標頭也會產生 deprecated 提示，說明 4.0.0 預設不再支援 BluetoothSerial；本章限定在鎖定的 3.3.11，不隱藏此提示，也不承諾未來 4.x 相容。

程式每輪最多讀 64 bytes，接受 ASCII 一行 1～64 bytes；CR 被略過、LF 結束。任一非法字元或超長都捨棄整行，避免長字串尾端變成第二個有效指令。未在 2 秒內完成的半行顯示 `RX TIMEOUT`。

OLED 只顯示指令結果與固定回應，不回顯任意文字，以免把使用者貼上的私人資料拍進照片。學生可修改固定的 `HELLO` 回應，練習雙向傳輸。

## 驗收與排錯

| 操作 | OLED／終端結果 |
|---|---|
| 啟動失敗 | `BT INIT ERR`，先核對 Wrover 與 Core 版本 |
| `PING` 加 LF | `RX OK`／`PONG` |
| 未知指令 | `CMD ERR`／`USE HELLO OR PING` |
| 65 字元一行或非法字元 | `RX ERR`／`INVALID LINE` |
| 半行超過 2 秒 | `RX TIMEOUT`／`LINE TIMEOUT` |
| 關閉 SPP 通道 | `LINK LOST`，重連後不得接續舊半行 |

練習：依序送三次 `PING`，再關閉終端；用 OLED 照片與終端固定回應確認連線和配對的差別。

## 編譯、燒錄與回報

範例：[`01_bluetooth_text`](../../examples/05_camera_ble_multitasking/01_bluetooth_text/README.md)。

在 Repository 根目錄使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11 與鎖定函式庫。請先看[第五篇首頁](README.md)的命令與驗證邊界，燒錄後保留 OLED 正面、完整接線和板上操作結果。OLED 初始化失敗的 GPIO 2 閃爍碼見首頁。
