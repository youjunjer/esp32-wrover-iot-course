# 2. 開發工具與第一支程式

## 工具分工

- Arduino CLI：正式的 Core、函式庫、編譯、燒錄與自動驗證工具。
- Arduino IDE 2.x：編輯程式、選擇連接埠及人工操作的輔助工具。
- GitHub Actions：在沒有本機 Arduino CLI 的情況下，使用鎖定版本重新編譯所有正式範例。

本章不要求以序列監控器判斷程式是否正常；`01_hello` 使用 GPIO 2 狀態 LED 提供可見結果。

## 統一設定

```text
Board: ESP32 Wrover Module
FQBN:  esp32:esp32:esp32wrover
Core:  3.3.11
```

完整安裝與作業系統差異見 [`docs/environment-cli.md`](../environment-cli.md)。

## 編譯與燒錄

在 Repository 根目錄執行：

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello

arduino-cli board list

arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/01_basics/01_hello
```

## 可見驗收

1. 編譯與上傳都沒有錯誤。
2. ESP32 重啟後，GPIO 2 狀態 LED 快速閃爍三次。
3. 接著每兩秒出現一次短心跳。

若指定課程板沒有 GPIO 2 板載 LED，依第一篇首頁的課程接線方式將 LED 接到 GPIO 2。只看到編譯成功仍不等於硬體已驗證。

## 常見問題

- 找不到連接埠：先更換可傳輸資料的 USB 線，再確認 USB-to-Serial 驅動。
- 卡在 Connecting：確認連接埠與板型，必要時依板子說明使用 BOOT／EN。
- 上傳成功但 LED 不亮：確認 GPIO 2 是否真的接到 LED、極性與共地。
- LED 一直亮或一直滅：確認沒有把 LED 直接接到 3.3V，並重新檢查程式與接線。
