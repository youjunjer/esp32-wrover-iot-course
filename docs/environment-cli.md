# Arduino CLI 環境

## 安裝內容

本課程同時安裝：

- Arduino IDE 2.x：編輯、序列監控與人工排錯。
- Arduino CLI：安裝 Core、安裝函式庫、編譯、燒錄與自動驗證。
- Espressif Arduino-ESP32 Core：穩定版 `3.3.11`。

## 設定 ESP32 Core

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.11
arduino-cli core list
```

## 開發板

本課程的一般、網路、相機與能源監測範例都使用：

```text
esp32:esp32:esp32wrover
```

進入相機單元時不更換 FQBN，只在程式中選擇 AI Thinker 相機腳位。

## 標準驗證

```bash
arduino-cli compile --fqbn esp32:esp32:esp32wrover examples/01_basics/01_hello
arduino-cli board list
arduino-cli upload -p <SERIAL_PORT> --fqbn esp32:esp32:esp32wrover examples/01_basics/01_hello
```

完成條件：

1. Core 正確列在 `arduino-cli core list`。
2. `compile` 無錯誤完成。
3. `upload` 完成且開發板重新啟動。
4. GPIO 2 狀態 LED 先快速閃爍三次，再每兩秒顯示一次心跳；若板上沒有該 LED，依安全接線說明外接 LED 與限流電阻。

序列監控器並非 Codex 輔助開發的必要條件。OLED 建立前的基礎範例使用可直接觀察的 LED 動作，不保留 Serial-only 訊息；從 OLED 章開始，感測、網路、雲端、MQTT、相機與能源範例的所有執行階段訊息都必須顯示在 OLED，Serial 只能作為同步副本。

## 版本政策

教材開發可評估新的穩定版，但每一個正式教材版本必須記錄已驗證的 CLI、Core 與函式庫版本。不使用 RC、Alpha 或每日開發版作為課堂基準。
