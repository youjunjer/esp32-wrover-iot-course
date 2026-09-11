# Node-RED 可重現教學環境

第四篇使用一個與個人 Node-RED 環境分離的本機資料目錄，不匯入舊教材的 `.node-red` userDir、`flows_cred.json`、`settings.js` 或 `node_modules`。

## 鎖定版本

| 元件 | 版本 |
|---|---|
| Node.js | 24.x |
| Node-RED | 5.0.7 |
| FlowFuse Dashboard | `@flowfuse/node-red-dashboard` 1.31.0 |

`.node-version` 與 `package.json` 是正式基準。不得改用已淘汰的 `node-red-dashboard` 舊 Dashboard。

## 建立獨立環境

在 Repository 的 `nodered` 目錄執行：

```bash
node --version
npm ci
npm start
```

`npm start` 會以 `.node-red-course/` 作為 userDir，限定監聽 `127.0.0.1:1880`，並關閉 telemetry；該 userDir 只能留在本機，不得提交。瀏覽器開啟 Node-RED 編輯器後，先到 Palette 確認 FlowFuse Dashboard 版本，再匯入 `flows/` 內的 JSON。若舊 Flow 使 Runtime 無法啟動，使用 `npm run start:safe` 開啟不自動執行 Flow 的編輯器。

這是快速練習入口。[第 1 章的完整操作](../docs/part4/01_environment.md)則使用外部 `ESP32_NodeRED_Runtime` 目錄；兩種方式的 Flow／settings／context 各自獨立。請選定一種 userDir 持續使用，切換時先停止原 Runtime，並重新匯入公開 Flow，不要把空白新目錄誤認為資料遺失。

## 教學 Flow

| Flow | 教學重點 | 可見驗收 |
|---|---|---|
| `02_message_path.json` | Flow、Node、Wire 與完整 `msg` | Debug 展開 object 型別的 payload、topic 與 `_msgid` |
| `03_core_nodes.json` | Inject、Change、Function、Switch 與 Debug | 正常溫度分流；錯誤型別只輸出拒絕原因 |
| `04_mqtt_tls_pubsub.json` | TLS MQTT In／Out、精確 Topic、QoS 與 PING／PONG | Broker 連線後可收到 data／status／ack；`cmd` 絕不 Retain |
| `05_json_validation.json` | JSON 解析、Schema v1、範圍與新鮮度驗證 | 合法資料通過；結構錯誤與無效 JSON 分別進入安全錯誤輸出 |
| `06_flowfuse_dashboard.json` | Dashboard 2 Text、Gauge、Chart 與資料品質 | 只有 VALID DEMO 更新 widget；INVALID／STALE 被阻擋 |
| `07_safe_control.json` | status／boot ID、持久序號、TTL、cmd 與 ACK | 未設定 `file` context 時 fail-close；ACK 不冒充實體回授 |

`04_mqtt_tls_pubsub.json` 的文字 `PING`／`PONG` 對應第三篇第 9 章，只驗證訊息方向，不用來控制第 11 章的繼電器或 SG90。

第 6、7 章各有一個獨立 `ui-base`。FlowFuse Dashboard 2 不支援同一個 Runtime 同時存在多個 `ui-base`，請依章節使用乾淨 userDir 或先移除前一章的 Dashboard config nodes，不要把兩份 Dashboard Flow 一次 Deploy。

## MQTT 匯入後必做

`04_mqtt_tls_pubsub.json` 故意只放明確佔位值，匯入後必須由學生在本機編輯：

1. 將 `REPLACE_WITH_MQTT_HOST` 換成自己的 Broker 主機。
2. 將 `REPLACE_WITH_UNIQUE_NODERED_CLIENT_ID` 換成每台唯一的 Client ID。
3. 將每個 `REPLACE_WITH_UNIQUE_TOPIC_ROOT` 換成自己的 `class702/<group>/<device>` 精確根 Topic。
4. 只使用 8883 與可驗證的 TLS 憑證；不關閉主機憑證檢查。
5. MQTT 帳號與密碼只在本機憑證欄位輸入，不加入 Flow JSON、截圖或 Debug。
6. `04_mqtt_tls_pubsub.json` 與 `07_safe_control.json` 的 Broker 都預設 `autoConnect=false`；完成 TLS、ACL 與 Topic 設定前不啟用，安全控制還必須先完成無市電負載與 `file` context 驗證。

設備與 Node-RED 必須使用各自唯一的 Client ID。Node-RED 的最小 ACL 只允許訂閱本組精確 `data`、`status`、`ack`，並只允許發布本組精確 `cmd`；不授權 `class702/#`。ESP32 端使用相反方向的最小權限。`status` 可由 ESP32 Retain；`data`、`cmd` 與 `ack` 不 Retain。

## 公開安全

提交前確認 Flow 與截圖中沒有 MQTT 帳密、Token、私人 IP、真實 host、完整 Topic、Client ID、憑證路徑或終端使用者名。第 2 章純 DEMO 使用完整 message 教學；其餘 Debug 只顯示所需的 `msg.payload` 或驗證摘要，真實 MQTT 資料不使用完整 message 輸出。
