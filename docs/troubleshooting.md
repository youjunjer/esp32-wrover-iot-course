# 故障排除

## 找不到序列埠

1. 確認 USB 線支援資料傳輸，不是只能充電。
2. 拔除開發板前後比較 `arduino-cli board list`。
3. 依實際 USB-to-Serial 晶片安裝合適驅動。
4. Linux 確認使用者擁有 `/dev/ttyUSB*` 或 `/dev/ttyACM*` 權限。

## 卡在 Connecting

1. 確認序列埠與 FQBN。
2. 關閉其他佔用相同序列埠的監控工具。
3. 開始燒錄時按住 BOOT，進入寫入後再放開。
4. 必要時短按 EN/RESET 後重試。

## 燒錄成功但沒有輸出

- 序列鮮率設為 `115200`。
- 按一次 EN/RESET。
- 確認程式有執行 `Serial.begin(115200)`。

## 網路與雲端範例

依序確認：

1. ESP32 取得 IP。
2. DNS 與外網可用。
3. HTTP/MQTT 伺服器可連線。
4. Topic、Payload 與憑證正確。
5. 接收端能看到來自正確裝置的新資料。

請不要一次同時修改腳位、函式庫、網路與資料格式。
