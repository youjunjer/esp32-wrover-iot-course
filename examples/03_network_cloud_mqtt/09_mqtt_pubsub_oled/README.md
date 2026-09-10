# 09 MQTT Publish／Subscribe OLED

本章在第 8 章的 TLS、Last Will 與唯一命名空間上，加入雙向 Publish／Subscribe。裝置每 30 秒向 `data` 發布不保留的 heartbeat，並只訂閱自己的精確 `cmd` Topic；收到 `PING` 後在 `ack` 回覆 `PONG`。

```text
class702/<group>/<device>/data    device -> broker, QoS 1, no retain
class702/<group>/<device>/cmd     controller -> device, QoS 1, no retain
class702/<group>/<device>/ack     device -> controller, QoS 1, no retain
class702/<group>/<device>/status  device -> broker, QoS 1, retained/LWT
```

每組必須使用獨立 Topic root、Client ID 與 Broker ACL。控制訂閱不能使用 `#` 或 `+`；命令不得 Retain。回呼函式只把有限長度訊息放進待處理區，不在 callback 內 publish／subscribe，以免回呼期間產生鎖死或封包狀態衝突。

最小 ACL：裝置只可 publish 自己的精確 `data`、`ack`、`status`，並只可 subscribe 自己的精確 `cmd`；控制端方向相反。不要授權 `class702/#`。`MQTT_ROOT_CA` 必須由 Broker 維運者或簽發 CA 官方來源取得，核對當下憑證鏈、有效期與 SHA-256 指紋；禁止不明來源 PEM 與 `setInsecure()`。

## 接線

| 模組腳位 | ESP32 Wrover |
|---|---|
| OLED VCC／GND | 3.3V／GND |
| OLED SDA／SCL | GPIO 21／22 |
| 備援狀態 LED 訊號 | GPIO 2 |

本章不接感測器、繼電器或馬達，先隔離驗證 MQTT 雙向資料流。

## OLED 狀態

| 畫面 | 意義 |
|---|---|
| `BOOT`／`CONFIG ERR` | OLED 已啟動，或本機設定仍是佔位值 |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線中、退避等待或逾時 |
| `TIME WAIT`／`TIMEOUT` | NTP 同步中或逾時；未完成時阻止 TLS |
| `TLS CONNECT`／`TLS ERR` | 正在驗證 Broker 憑證，或 CA／日期／網路失敗 |
| `MQTT OFF`／`MQTT ERR` | 等待退避，或 CONNECT／CONNACK 失敗 |
| `SUB OK`／`SUB ERR` | 精確 `cmd` Topic 與 retained status 建立成功或失敗 |
| `PUB OK`／`PUB ERR` | heartbeat／ACK 發布成功或失敗 |
| `CMD OK`／`CMD REJECT` | 收到 `PING` 並回覆 `PONG`，或拒絕其他／過長命令 |
| `LINK LOST` | MQTT loop 失敗，停止宣稱在線並重連 |

若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗。超過 48 bytes 的命令一律拒絕；它不會控制硬體。

## 編譯、燒錄與驗收

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/09_mqtt_pubsub_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/09_mqtt_pubsub_oled
```

實機驗收：訂閱端收到遞增 heartbeat；只向該組精確 `cmd` 發布非 retained `PING` 後收到 `PONG`；錯誤文字與過長訊息得到 `REJECT`；另一組 Topic 不影響本裝置。CI 不驗證 Broker、ACL、TLS、封包 QoS、Retain 或 OLED。
