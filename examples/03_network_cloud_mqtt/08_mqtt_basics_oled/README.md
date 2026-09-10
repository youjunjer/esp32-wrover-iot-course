# 08 MQTT 基礎 OLED

本章以實際的 TLS 連線介紹 Broker、Client、Topic、QoS、Retain、Clean Session、Client ID 與 Last Will。只發布裝置上線狀態，不接控制負載。

## 為什麼改用 `MQTT` 2.5.3

舊能源範例使用 PubSubClient，但其官方 Repository 已在 2026 年標示為不再維護。新版教材改用仍有維護的 `MQTT` 2.5.3（arduino-mqtt），支援 MQTT 3.1.1 與 QoS 0／1／2。這是新版遷移決策，不冒充原專案既有實作。

## 每組設定

複製 `secrets.example.h` 為 `secrets.h`，填入 TLS Broker 主機、8883 埠、帳密、Broker 根憑證、每組唯一 `MQTT_TOPIC_ROOT` 與 `MQTT_CLIENT_ID`。主機名稱不能帶 `mqtt://` 或路徑，Topic 不得含 `+`／`#`，Client ID 最多 23 個英數或連字號字元。

`MQTT_ROOT_CA` 只向 Broker 維運者或簽發 CA 官方 Repository 取得。上課前核對 Broker 當下憑證鏈、根憑證主體、有效期與 SHA-256 指紋，將核對日期寫進教師紀錄；不要從部落格或不明網站複製 PEM，也不可用 `setInsecure()` 當作修正。若 Broker 更換憑證鏈，只更新各組未追蹤的 `secrets.h`，不要把真實憑證設定或帳密提交到 Git。

本章使用：

```text
class702/<group>/<device>/status
```

上線狀態以 QoS 1、Retain 發布；非正常斷線時 Broker 透過 Last Will 把同一 Topic 更新為 `online:false`。Clean Session 設為 true，因此每次連線都要重新建立需要的訂閱（本章尚未訂閱）。

最小 ACL 是裝置只可向自己的精確 `status` Topic 發布，觀察端只可訂閱該 Topic；Broker 必須允許同一裝置設定同 Topic 的 Last Will。不要授權整班共用的 `class702/#`。

## 接線

| 模組腳位 | ESP32 Wrover |
|---|---|
| OLED VCC／GND | 3.3V／GND |
| OLED SDA／SCL | GPIO 21／22 |
| 備援狀態 LED 訊號 | GPIO 2 |

## OLED 狀態

| 畫面 | 意義 |
|---|---|
| `BOOT`／`READY` | OLED 已啟動，或設定通過等待網路 |
| `CONFIG ERR` | 缺少帳密、CA、唯一 Topic 或 Client ID |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線中、退避等待或逾時，Broker 保持離線 |
| `TIME WAIT`／`TIME OK`／`TIMEOUT` | NTP 同步中、已取得足供憑證日期檢查的時間，或同步逾時 |
| `TLS CONNECT`／`TLS ERR` | 驗證 Broker 憑證或失敗 |
| `MQTT OFF`／`RETRY` | 尚未連 Broker，等待有限退避後重試 |
| `MQTT CONNECT`／`MQTT ERR` | 送出 MQTT CONNECT；錯誤畫面顯示 library error／CONNACK 數字 |
| `MQTT ONLINE` | 連線並發布 retained 上線狀態 |
| `PUB OK`／`PUB ERR` | QoS 1 狀態發布結果 |
| `LINK LOST`／`RETRY` | 斷線後有限間隔重連 |

若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗。

## 編譯與驗收

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/08_mqtt_basics_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/08_mqtt_basics_oled
```

實機驗收須由受控訂閱端確認 `status` Topic 的 online retained 訊息，斷開裝置網路後確認 Last Will 變為 offline，再重新上線。不要以公用未加密 1883 Broker、`setInsecure()` 或共用 Client ID 取代。CI 只證明編譯，不驗證 Broker、ACL、TLS、QoS、Retain、Last Will 或 OLED 實機。

官方資料：[MQTT 3.1.1 規範](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)、[`MQTT` library 2.5.3](https://github.com/256dpi/arduino-mqtt/tree/v2.5.3)。
