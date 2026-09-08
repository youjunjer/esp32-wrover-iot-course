# 02 NTP 網路時鐘與 OLED

本範例在 Wi-Fi 連線後以 `configTzTime("CST-8", ...)` 啟動非同步 NTP 校時。`CST-8` 依 POSIX 時區規則代表 UTC+8；正負號與一般直覺相反。

## OLED 接線

| OLED | ESP32 Wrover |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

OLED 旋轉值為 `2`。找不到 OLED 時 GPIO 2 閃 2 下；OLED 初始化失敗時閃 3 下。

## 憑證設定

```bash
cp examples/03_network_cloud_mqtt/02_ntp_clock_oled/secrets.example.h \
  examples/03_network_cloud_mqtt/02_ntp_clock_oled/secrets.h
```

Windows PowerShell：

```powershell
Copy-Item examples/03_network_cloud_mqtt/02_ntp_clock_oled/secrets.example.h `
  examples/03_network_cloud_mqtt/02_ntp_clock_oled/secrets.h
```

將 `WIFI_SSID` 與 `WIFI_PASSWORD` 只填在本機 `secrets.h`。缺檔或仍為佔位字時，OLED 會固定顯示 `CONFIG ERR`。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `CONFIG ERR` | 缺少可用的本機 Wi-Fi 憑證 |
| `WIFI` / `CONNECT LEFT Ns` | Wi-Fi 連線截止計時中 |
| `NO AP` / `CONNECT ERR` / `WIFI TIMEOUT` | Wi-Fi 失敗類型 |
| `NTP SYNC` | NTP 已設定，正在等待時間 |
| `NTP T/O` / `NO TIME` | 15 秒內未取得有效時間 |
| `TIME OK` | 曾完成校時，畫面時區為 UTC+8 |
| `NTP STALE` | 最後校時通知已超過 90 分鐘；時鐘可繼續走，但不再宣稱剛同步 |
| `NET LOST` | Wi-Fi 中斷；舊時鐘若仍有效會標成 `CLOCK LOCAL` |
| `SYNC Ns` | 距離最後一次校時通知的秒數 |

NTP callback 只改變一個原子通知旗標；OLED 仍由主 `loop()` 更新，不在網路任務中操作 I²C。

## CLI

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/02_ntp_clock_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/02_ntp_clock_oled
```

## 可見驗收與證據邊界

1. 正常開機時依序看到 Wi-Fi 連線、`NTP SYNC`、`TIME OK`。
2. 日期與時間要與台灣 UTC+8 相符，且秒數持續前進。只看到「像真的日期」不足以證明時區正確。
3. 在從未同步的狀態下斷開網路，應顯示 Wi-Fi 錯誤或 `NTP T/O`，不可顯示偽造時間。
4. 同步成功後關閉 AP，時鐘可繼續走，但 OLED 必須同時顯示 `NET LOST` 與 `CLOCK LOCAL`。

CI 只能證明 NTP、Wi-Fi 與 OLED API 在鎖定工具鏈下可編譯；不能證明 DNS、UDP 123、NTP 伺服器、UTC+8 轉換、斷線後時鐘精準度或 OLED 實機畫面。
