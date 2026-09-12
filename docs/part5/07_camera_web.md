# 區網 CameraWebServer、拍照與 Base64

完成上一章實機預檢後，才進行此章。這是依官方相機 API 重寫的精簡教學伺服器，並未搬入原範例的整包網頁、Serial-only 狀態或無限等待迴圈。

## 準備與操作

1. 複製本章 `camera_config.example.h` 為 `camera_config.h`，填入上一章實際驗證過的設定。
2. 複製 `secrets.example.h` 為 `secrets.h`，設定 Wi-Fi 和獨立 Web 使用者／密碼；密碼至少 12 字元。真值不寫在指令列、README 或照片中。
3. 使用 Wrover FQBN 編譯燒錄。在受信任的教室區網等待 OLED `WEB READY`，同一區網的瀏覽器開啟 `http://wrover-camera.local/` 並輸入帳密。一次只測一片，避免 mDNS 名稱重複。
4. 若 mDNS 不被裝置支援，從自己的路由器裝置清單確認 IP；不把完整 IP 或網路資訊放進公開截圖。`MDNS ERR` 不等於 WebServer 已停止。
5. 依序測試 JPEG 拍照、Base64、短串流，以及 QVGA／VGA 解析度切換。只拍攝已同意的現場或無個資物件。

## 路由與資源限制

| 路由 | 行為 |
|---|---|
| `/` | 驗證身分後顯示操作連結與解析度表單 |
| `/capture` | JPEG bytes，Content-Length 與實際大小相同 |
| `/base64` | 同一類 JPEG 的 Base64 文字，不附 data URL 前綴 |
| `/stream` | multipart MJPEG，最多 20 張／10 秒，再次點擊才重新開始 |
| POST `/resolution` | 僅接受 QVGA／VGA，驗證每次開機生成的 CSRF 值 |

所有影像與首頁需要 Digest 驗證；回應 `Cache-Control: no-store`，程式不把影像存到板上檔案。HTTP 本身沒有加密影像，不能轉發到公網；Digest 也不是 HTTPS。

每張影像上限 48 KiB、網路寫入期限 10 秒，單次寫入以小區塊進行並設定 socket timeout。這些是應用層上限，底層呼叫仍可能多花一個 socket timeout；不是硬即時保證。此同步 WebServer 同時服務一個請求，串流期間其他操作可能等待，故刻意只做短串流。

Base64 每 3 bytes 變 4 字元，大小約增加三分之一，**不會加密、壓縮或匿名化照片**。程式每次編碼 384 bytes，避免再配置整張 Base64 字串。`esp_camera_fb_return()` 會在所有完成／失敗路徑歸還影格。

## OLED 排錯與驗收

Wi-Fi 最長等待 15 秒，逾時保留 `WIFI TIMEOUT` 10 秒後重試並顯示次數；斷線顯示 `LINK LOST` 並停用舊 server／mDNS，重連再啟動。未填設定則停在 `CONFIG ERR`。

驗證未登入請求得到認證要求；正常登入可見 `PAGE SENT`、`SEND OK`、`STREAM DONE`。超過大小限制顯示 `FRAME SIZE ERR`，先改回 QVGA；`SEND TIMEOUT`／`STREAM ERR` 檢查連線、相機與供電。伺服器送出 bytes 並不保證瀏覽器已成功解碼，需實際開啟 JPEG 驗收。

驗收包含兩種尺寸、Base64 解碼後可開啟、短串流結束、未登入拒絕、錯誤 CSRF 拒絕、Wi-Fi 中斷與恢復。公開回報以測試物為主，不夾帶帳密或現場人物影像。

## 編譯、燒錄與回報

範例：[`08_camera_web`](../../examples/05_camera_ble_multitasking/08_camera_web/README.md)。

在 Repository 根目錄使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11 與鎖定函式庫。請先看[第五篇首頁](README.md)的命令與驗證邊界，燒錄後保留 OLED 正面、完整接線和板上操作結果。OLED 初始化失敗的 GPIO 2 閃爍碼見首頁。
