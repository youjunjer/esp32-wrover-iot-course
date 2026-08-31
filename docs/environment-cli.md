# Arduino IDE、CLI 與 ESP32 開發環境

本課程以 Arduino CLI 作為可重複的編譯與燒錄主流程，Arduino IDE 2.x 用於編輯程式、選擇連接埠與人工操作。GitHub Actions 會在沒有本機 CLI 的情況下編譯全部正式範例；但要從學生電腦燒錄實體開發板，仍需要本機 IDE 或 CLI。

## 統一版本

```text
Arduino CLI: 1.5.1
ESP32 Core:  3.3.11
Board:       ESP32 Wrover Module
FQBN:        esp32:esp32:esp32wrover
```

`config/toolchain.lock` 是正式基準。日後可評估更新的穩定版，但必須先將所有 Sketch 重新編譯並完成實體板驗證，不在課堂中臨時混用版本。

## 1. 安裝 Arduino IDE 2.x

從 [Arduino 官方軟體頁](https://www.arduino.cc/en/software) 下載最新穩定版 IDE 2，不使用 Nightly：

- Windows：執行 64-bit 安裝程式，依導引完成。
- macOS：開啟 `.dmg`，將 Arduino IDE 拖到「應用程式」。
- Linux：使用官方 AppImage 或 ZIP；AppImage 需先設為可執行。

IDE 是課程輔助工具，不要用「IDE 剛好能編譯」取代 CLI 與 CI 的版本驗證。

## 2. 安裝 Arduino CLI

官方安裝方法見 [Arduino CLI Installation](https://arduino.github.io/arduino-cli/dev/installation/)。

### Windows

1. 從官方頁下載 Windows 64-bit `arduino-cli` 壓縮檔或 MSI，選用本教材鎖定的 1.5.1。
2. 將 `arduino-cli.exe` 所在目錄加入 `PATH`。
3. 重新開啟 PowerShell，執行 `arduino-cli version`。

### macOS

可使用 Homebrew：

```bash
brew update
brew install arduino-cli
arduino-cli version
```

若 Homebrew 版本不是 1.5.1，請改用官方頁的對應歷史版本壓縮檔，不要直接改動教材鎖定檔迴避版本差異。

### Linux

從官方頁下載與 CPU 架構對應的 1.5.1 壓縮檔，將 `arduino-cli` 放入已列在 `PATH` 的目錄，再執行：

```bash
arduino-cli version
```

## 3. 確認 USB 線、驅動與連接埠

先使用「可傳輸資料」的 USB 線；只能充電的線不會出現連接埠。開發板常見 USB-to-UART 晶片為 CP210x 或 CH340／CH341，只在作業系統沒有自動辨識時，依板上實際晶片安裝 [Silicon Labs CP210x](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers) 或 [WCH CH340／CH341](https://www.wch-ic.com/downloads/CH341SER_ZIP.html) 官方驅動。

- Windows：在「裝置管理員 → 連接埠 (COM 和 LPT)」確認新出現的 `COM3`、`COM4` 等連接埠。黃色驚嘆號通常代表驅動尚未正確安裝。
- macOS：連接埠常見為 `/dev/cu.usbserial-*`、`/dev/cu.SLAB_USBtoUART` 或 `/dev/cu.wchusbserial-*`。新安裝的驅動若被系統阻擋，到「系統設定 → 隱私權與安全性」確認。
- Linux：連接埠常見為 `/dev/ttyUSB0` 或 `/dev/ttyACM0`。出現 `Permission denied` 時，可執行 `sudo usermod -a -G dialout YOUR_USERNAME`，然後登出再登入；不要以長期使用 `sudo arduino-cli` 作為替代方案。

不確定連接埠時，先執行 `arduino-cli board list`，再拔掉、插回開發板比較清單。

## 4. 安裝 ESP32 Core

Repository 已提供 `arduino-cli.yaml`，在專案根目錄執行：

```bash
arduino-cli --config-file arduino-cli.yaml core update-index
arduino-cli --config-file arduino-cli.yaml core install esp32:esp32@3.3.11
arduino-cli --config-file arduino-cli.yaml core list
```

Arduino IDE 使用者則在設定的「Additional Boards Manager URLs」加入：

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

開啟開發板管理員，安裝 Espressif Systems 的 `esp32` 3.3.11，開發板選擇 `ESP32 Wrover Module`。安裝流程亦可對照 [Espressif Arduino-ESP32 官方文件](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 5. 編譯、燒錄與可見驗收

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

將 `YOUR_SERIAL_PORT` 替換為 `board list` 顯示的實際值，例如 Windows 的 `COM4`、macOS 的 `/dev/cu.usbserial-0001` 或 Linux 的 `/dev/ttyUSB0`。不要在 Shell 指令中使用尖括號當占位字，否則可能被解讀為輸入重導。

完成條件：

1. `arduino-cli version` 與 `config/toolchain.lock` 一致。
2. `core list` 顯示 `esp32:esp32` 3.3.11。
3. `compile` 與 `upload` 無錯誤完成，開發板重新啟動。
4. GPIO 2 狀態 LED 先快速閃爍三次，再每兩秒顯示一次心跳。

若卡在 `Connecting...`，先確認板型、連接埠、資料 USB 線，並關閉佔用連接埠的其他程式；必要時在開始連線時按住 BOOT，進入寫入後放開。

第一篇使用可直接觀察的 LED 動作，不保留 Serial-only 訊息。從第二篇 OLED 章開始，感測、網路、雲端、MQTT、相機與能源範例的所有執行階段訊息都必須優先顯示於 OLED，Serial 只能作為同步副本。
