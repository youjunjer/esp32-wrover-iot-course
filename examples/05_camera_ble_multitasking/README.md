# 第五篇：相機、Bluetooth／BLE 與多工

正式教材：[8 章課文與範例索引](../../docs/part5/README.md)。本篇包含 9 個 Sketch，仍統一使用 `esp32:esp32:esp32wrover`；相機程式指定 `CAMERA_MODEL_AI_THINKER`。

一般模式使用 OLED GPIO 21／22；相機模式需經指定課程板實測的替代 I²C 設定。相機範例預設鎖定，先完成第 6 章預檢；不能只改旗標就宣稱相機與 OLED 共存已驗證。

下載時保留整個 Repository，Sketch 會引用同篇 `support/`。編譯、燒錄、接線、OLED 訊息和錯誤驗收都在逐例 README 與課文；[驗證紀錄](../../docs/part5/verification.md)區分軟體編譯與硬體待辦。
