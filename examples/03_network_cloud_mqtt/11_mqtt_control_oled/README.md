# 11 MQTT 安全遠端控制 OLED

本章延伸原能源專案的紅黃綠燈、低電位繼電器與 SG90 控制，但重新建立安全啟動、精確 JSON、命令時效、開機識別、去重與 ACK。所有控制先在低風險、無市電負載狀態驗證。

## 腳位與安全狀態

| 功能 | GPIO | 開機／錯誤狀態 |
|---|---:|---|
| 紅／黃／綠 LED | 4／2／15 | LOW／全關 |
| 低電位觸發繼電器 | 18 | HIGH／安全關閉 |
| SG90 訊號 | 5 | 未 attach；GPIO 5 也是 strapping pin |
| OLED SDA／SCL | 21／22 | 主要診斷介面 |

程式先把繼電器 latch 設為 HIGH，再切換成 OUTPUT。設定缺失、Wi-Fi／NTP／TLS／MQTT 錯誤、命令／ACK／status 錯誤或連線中斷都呼叫 `safeOutputs()`。SG90 使用適當的獨立 5V 電源並與 ESP32 共地；`detach()` 只停止控制脈波，不代表機械結構會回到安全角度或仍有保持力，因此不可連接危險機構。繼電器只做無負載測試，禁止依本章文字操作市電。GPIO 2、5、15 都涉及開機 strapping，接線不得在開機時強迫錯誤電位。

軟體只有在韌體開始執行後才能主動維持 GPIO 18 為 HIGH；上電至程式接管前腳位可能處於高阻抗。真正的失效安全設計還需要合適的硬體偏壓、隔離、急停與獨立斷電機制，本教材程式不能取代。

## 命令格式

控制端先訂閱 retained `status`，取得本次隨機 `boot_id`，再向精確 `cmd` Topic 發布**不保留**命令：

```json
{"v":1,"cmd_id":"c001","cmd_seq":1,"boot_id":"12ab34cd","ts":1789000000,"ttl":10,"target":"traffic","value":"green"}
```

`target`／`value` 可為：

- `traffic`：`red`、`yellow`、`green`、`off`
- `relay`：`on`、`off`
- `servo`：整數 0～180

命令最多 320 bytes，必須剛好包含八個欄位；`ttl` 只接受 5～30 秒，收到時至少還要剩 2 秒。`cmd_seq` 必須在本次 boot 中從 1 單調遞增，任何小於或等於最後成功序號的命令都視為重播。錯誤 boot ID、過期、未來時間、重播／重複序號、錯誤型別、未知 target 或值，一律 `CMD REJECT`、全輸出關閉並向 `ack` 回覆原因。程式以 `gettimeofday()` 的毫秒值計算 `ts + ttl` 尚餘時間，並在套用輸出前建立 deadline；合法的 ON／燈色／伺服角度租約總上限為 30 秒。到期顯示 `LEASE EXPIRED` 並安全關閉，若要持續動作，控制端必須送出新的遞增命令。實際關閉仍由主迴圈執行，MQTT 單次 timeout 可能造成約 1 秒執行延遲，因此這不是硬體級安全計時器。

目前 `MQTT` library 的簡化 callback 不提供接收封包的 retained flag，因此防護採四層：每組精確 Topic 與 ACL、連線後先用零長度 retained publish 清除自己的 `cmd` Topic、每個命令必須符合本次 `boot_id` 與 TTL、`cmd_seq` 必須單調增加。控制端仍不得對 `cmd` 使用 Retain。最小 ACL：裝置可 publish 自己的 `status`／`ack` 及僅為清除 retained 而使用的 `cmd`，可 subscribe 自己的 `cmd`；控制端則可 subscribe `status`／`ack`、publish 非 retained `cmd`。不可直接授權 `class702/#`。

`MQTT_ROOT_CA` 只向 Broker 維運者或簽發 CA 官方來源取得，上課前核對憑證鏈、有效期及 SHA-256 指紋；禁止不明來源 PEM 與 `setInsecure()`。Retained `status` 只宣告 `online` 與本次 `boot_id`，不是繼電器、LED 或伺服的實體回授；命令 ACK 也只代表韌體已套用命令。涉及風險設備時必須另設實體感測與聯鎖。

## OLED 與驗收

| 畫面 | 意義 |
|---|---|
| `SAFE OFF`／`CONFIG ERR` | 啟動先關閉全部軟體控制輸出，或本機設定缺失 |
| `WIFI WAIT`／`WIFI OFF`／`WIFI ERR` | Wi-Fi 連線中、退避等待或逾時；輸出保持關閉 |
| `TIME WAIT`／`TIMEOUT` | NTP 同步中或逾時；控制保持封鎖 |
| `TLS CONNECT`／`TLS ERR` | 正在驗證 Broker 憑證，或 CA／日期／網路失敗 |
| `MQTT OFF`／`MQTT ERR` | 等待退避，或 CONNECT／CONNACK 失敗 |
| `RETAIN CLEAR`／`RETAIN ERR` | 已清除精確 `cmd` retained 訊息，或清除失敗 |
| `CONTROL READY`／`SUB ERR` | 精確命令訂閱與 status 成功，或建立失敗 |
| `CMD OK`／`CMD REJECT` | 命令通過全部檢查並套用，或已拒絕且安全關閉 |
| `ACK ERR` | 命令回覆發布失敗；輸出回到安全狀態 |
| `LEASE EXPIRED` | 命令租約到期，全部輸出關閉 |
| `LINK LOST` | MQTT loop 失敗，輸出關閉並進入重連 |

OLED 不顯示 Broker、帳密或完整 Topic；本次 boot ID 透過受控 MQTT status 提供給控制端。若 OLED 無畫面，GPIO 2 重複閃 2 下表示找不到 0x3C／0x3D，重複閃 3 下表示 SSD1306 初始化失敗；GPIO 2 同時是本章黃燈，這個閃爍碼只在 OLED 啟動失敗時使用。

```bash
arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/11_mqtt_control_oled

arduino-cli board list
arduino-cli --config-file arduino-cli.yaml upload \
  -p YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:esp32wrover \
  examples/03_network_cloud_mqtt/11_mqtt_control_oled
```

依序驗收 LED → 紅黃綠燈 → 外部供電 SG90 → **未接市電負載**的繼電器。重開機、斷 Wi-Fi、Broker 停止、錯誤 retained 命令、舊 boot ID、過期／重複／超長 JSON 都必須維持繼電器 HIGH／OFF。CI 只驗證編譯，不能證明電源、GPIO、馬達、繼電器、Broker ACL 或控制時序已實測。
