# 05 WebServer OLED

在可信任且隔離的教室區網中啟動 ESP32 WebServer，透過網頁查看 GPIO 13 狀態，並以經過驗證的 POST 指令切換輸出。OLED 是主要狀態介面，範例不依賴 Serial Monitor。

## 準備設定

```bash
cp examples/03_network_cloud_mqtt/05_webserver_oled/secrets.example.h \
  examples/03_network_cloud_mqtt/05_webserver_oled/secrets.h
```

編輯未追蹤的 `secrets.h`：

- `WIFI_SSID`、`WIFI_PASSWORD`：教室 Wi-Fi。
- `DEVICE_HOSTNAME`：每組不同，最多 15 個字元，只能使用小寫英文字母、數字與連字號，例如 `esp32-team01`。
- `WEB_USERNAME`、`WEB_PASSWORD`：網頁登入資料；密碼至少 12 個字元且不得與 Wi-Fi 密碼共用。

未建立 `secrets.h` 或仍保留替代文字時，程式只會在 OLED 顯示 `CONFIG ERR`／`SAFE OFF`，不會嘗試連線。CI 使用 `secrets.example.h` 只做編譯，不代表有真實憑證。

## 路由與安全邊界

| 方法與路由 | 用途 |
|---|---|
| `GET /` | 經 Digest 驗證後載入控制頁 |
| `GET /api/status` | 回傳輸出狀態與開機毫秒數 |
| `POST /api/led` | 只接受唯一欄位 `state=on` 或 `state=off` |
| `GET` 或其他非 `POST` 方法 `/api/led` | 安全關閉輸出並回傳 `405 Method Not Allowed` |

控制頁在每次 Server 啟動或 Wi-Fi 重連時產生新的 CSRF Token，POST 必須帶正確的 `X-CSRF-Token`。重連後請重新整理舊網頁；否則舊 Token 會得到 403 並觸發 `SAFE OFF`。未知值、多餘欄位、錯誤 Token 或對控制 route 使用錯誤 HTTP 方法，都會回傳錯誤並把 GPIO 13 切回 `SAFE OFF`。程式也會明確拒絕 Basic Authentication，只接受 Digest Authentication。

Digest 驗證不能把明文 HTTP 變成 HTTPS。本範例只適合隔離或可信任的教室區網：禁止路由器 Port Forwarding、禁止直接公開到 Internet，也不得用來控制繼電器、市電或其他危險負載。

## OLED 狀態碼

| 狀態 | 意義 |
|---|---|
| `CONFIG ERR` | 缺少本機設定或設定格式不合要求 |
| `WIFI CONNECT` | 正在連線，畫面保留倒數 |
| `WEB READY` | HTTP Server 已啟動 |
| `MDNS ERR` | Server 已啟動，但 `.local` 名稱註冊失敗 |
| `REQ GET` / `REQ STATUS` | 讀取頁面或狀態成功 |
| `CMD OK ON/OFF` | 合法 POST 已完成 |
| `AUTH ERR` / `CSRF ERR` | 登入或 CSRF 驗證失敗 |
| `CMD ERR` / `METHOD ERR` | 指令內容或 HTTP 方法錯誤 |
| `WIFI TIMEOUT` / `RETRY` | 首次 Wi-Fi 連線逾時，輸出已安全關閉並等待重試 |
| `NET LOST` / `RETRY` | 已上線後 Wi-Fi 中斷，輸出已安全關閉並等待重連 |
| `SAFE OFF` | GPIO 13 保持 LOW |

成功 HTTP 事件顯示 2.5 秒後回到 RSSI；HTTP 錯誤不會一閃而過，會保留到下一個狀態改變或合法事件。

正常時以瀏覽器開啟 `http://DEVICE_HOSTNAME.local/`。若所在網路不支援 mDNS，OLED 的 `MDNS ERR` 可證明名稱註冊失敗，但本章不把完整私人網址印在畫面或文件截圖；請由教師從教室 DHCP 管理介面確認該裝置位址。

## 編譯

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/05_webserver_oled
```

CI 編譯只能證明程式與 ESP32 Core 3.3.11 API 相容；不能證明 Wi-Fi、mDNS、Digest、CSRF、瀏覽器頁面或 GPIO 13 已在指定課程板完成實測。
