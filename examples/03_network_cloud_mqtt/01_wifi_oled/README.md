# 01 Wi-Fi 掃描、連線與 OLED

本範例先非同步掃描附近 AP，確認設定的目標存在後才開始連線。掃描、連線與重試都不使用無限 `while` 等待，OLED 會持續顯示進度。

## OLED 接線

| OLED | ESP32 Wrover |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

找不到 OLED 時 GPIO 2 重複閃 2 下；找到位址但初始化失敗時閃 3 下。GPIO 2 必須是指定課程板上已確認接線的狀態 LED。

## 憑證設定

先將範本複製為不會被 Git 追蹤的 `secrets.h`：

```bash
cp examples/03_network_cloud_mqtt/01_wifi_oled/secrets.example.h \
  examples/03_network_cloud_mqtt/01_wifi_oled/secrets.h
```

Windows PowerShell 可改用：

```powershell
Copy-Item examples/03_network_cloud_mqtt/01_wifi_oled/secrets.example.h `
  examples/03_network_cloud_mqtt/01_wifi_oled/secrets.h
```

只在本機 `secrets.h` 填寫 `WIFI_SSID` 與 `WIFI_PASSWORD`。若沒有此檔，CI 會用 `secrets.example.h` 的佔位字編譯；實機上因此只會固定顯示 `CONFIG ERR`，不會拿佔位字連線。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `CONFIG ERR` | 尚未建立實際 `secrets.h`，或仍為佔位字 |
| `SCAN` | 非同步掃描進行中 |
| `CONNECT` | 目標 AP 存在，15 秒連線截止計時中 |
| `ONLINE` | 已取得 IP；畫面只顯示 `IP ASSIGNED` 與 RSSI |
| `NO AP` | 掃描時找不到目標 SSID |
| `SCAN ERR` / `SCAN T/O` | 掃描失敗或超過 12 秒 |
| `CONN ERR` / `TIMEOUT` | 連線失敗或超過 15 秒 |
| `LINK LOST` | 連線曾成功，但後來中斷 |
| `RETRY Ns` | 依 3、6、12、30 秒上限退避後再試 |

`WL_CONNECT_FAILED` 可能有多種原因，本範例不會只根據此狀態就誤報為 `AUTH ERR`。OLED 不顯示 SSID、密碼或完整內網 IP。

## CLI

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/01_wifi_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/01_wifi_oled
```

## 可見驗收與證據邊界

1. 未建立 `secrets.h` 時，OLED 穩定保留 `CONFIG ERR`。
2. 有效設定下，畫面由 `SCAN` 進入 `CONNECT`，成功後顯示 `ONLINE`與合理 RSSI。
3. 關閉課堂 AP 後要看到 `LINK LOST` 或 `NO AP` 與重試倒數；恢復 AP 後能自動回到 `ONLINE`。
4. 回報問題時提供 OLED 正面照、完整接線照、板型、Core 版本與燒錄結果，但先遮蔽 SSID 與密碼。

CI 只能證明佔位憑證路徑可編譯，不能證明實際 AP 掃描、密碼、DHCP、RSSI、斷線或重連已通過實機驗證。
