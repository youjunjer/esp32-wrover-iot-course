# AI Thinker 相機腳位與 OLED 預檢

這一章的程式有意預設鎖住相機。**指定課程板的替代 OLED 腳位與相機共存尚未實測，不能直接照猜測腳位接線。**

所有編譯仍用 `esp32:esp32:esp32wrover`；Sketch 宣告 `CAMERA_MODEL_AI_THINKER`。相機對照見[硬體模式表](../hardware-pin-modes.md)，不能改選另一個 FQBN 迴避腳位問題。

## 為什麼不能沿用 21／22

相機的資料腳包含 GPIO 21，像素時鐘為 GPIO 22，兩者已被使用。程式另外排除 Flash 6～11、Wrover PSRAM 16／17、UART 1／3、GPIO 2 備援 LED、輸入限定腳與所有相機腳位；這個邏輯檢查只會排除已知衝突，不能證明剩下的腳在課程板上有引出或沒有外接電路。

## 分階段實測流程

1. 斷電，取得指定板實物與電路圖，核對可引出的兩個 I²C 腳位、3.3V 上拉、啟動電位、供電與其他元件。此步要由持有板子的教師完成。
2. 在該 Sketch 資料夾複製 `camera_config.example.h` 為 `camera_config.h`。只填入經電路核對的 `CAMERA_OLED_SDA`／`CAMERA_OLED_SCL`，兩個旗標維持 `false`；不更動 secrets 或 Repository 範例檔。
3. 用這個預檢 Sketch 測試替代 OLED。此時相機不初始化，OLED 應出現 `CAM LOCKED`。確認畫面、接線和多次上電都正常，才能把 `OLED_PINS_VERIFIED` 改為 `true`。
4. 確認相機線序與電源後，將 `ENABLE_CAMERA_TEST` 改為 `true`，編譯燒錄做**共存測試**。這個開關只表示要進入測試，不能當作共存已驗證的證據。
5. `FRAME OK` 每 5 秒顯示尺寸與 JPEG bytes。確認 OLED 連續更新、相機有影格、供電穩定，再補照片與實測表，之後才把同一份本機設定用在第 7／8 章。

所有 `camera_config.h` 都由 gitignore 排除，不公開未經驗證的接線推薦。CI 使用的 13／14／33 是**只為讓程式分支進入編譯的合成值**，不得拿來當實機接線表或燒錄設定。

## OLED 與最低限度錯誤訊號

| 訊號 | 意義 |
|---|---|
| GPIO 2 每組 4 閃 | 尚無有效替代 I²C 設定，OLED 無法初始化；相機未啟動 |
| GPIO 2 每組 2 閃 | 在設定匯流排找不到 0x3C／0x3D OLED |
| GPIO 2 每組 3 閃 | I²C／OLED 初始化失敗 |
| `CAM LOCKED` | OLED 可顯示，相機測試條件尚未打開 |
| `PSRAM ERR` | 未發現 PSRAM，不開始相機 |
| `CAM INIT ERR` + code | 相機初始化失敗，檢查相機／電源／腳位 |
| `FRAME ERR`／`FORMAT ERR` | 無影格或非 JPEG，5 秒後再試 |

GPIO 2 必須有已確認接線的狀態 LED，不能假設所有板都有。初始化失敗不改用 Serial-only，也不自動回退到 21／22。

## 記憶體與驗收

相機先以 VGA 配置緩衝，再切 QVGA 擷取，讓下一章能練習兩種解析度。使用單一 PSRAM framebuffer；每次取得影格後，成功或失敗路徑都歸還，避免耗盡緩衝。

本章驗收紀錄須包含板版本、相機模組、替代腳位、上拉電壓、供電、Core／FQBN、OLED 正面與全接線照片。沒有這些結果，後兩章維持「程式可編譯、硬體待驗證」。

## 編譯、燒錄與回報

範例：[`07_camera_preflight`](../../examples/05_camera_ble_multitasking/07_camera_preflight/README.md)。

在 Repository 根目錄使用 Arduino CLI 1.5.1、ESP32 Core 3.3.11 與鎖定函式庫。請先看[第五篇首頁](README.md)的命令與驗證邊界，燒錄後保留 OLED 正面、完整接線和板上操作結果。OLED 初始化失敗的 GPIO 2 閃爍碼見首頁。
