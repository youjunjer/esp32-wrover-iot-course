# 第四篇：Node-RED

本篇把 Node-RED 獨立成一篇，延伸第三篇已完成的 MQTT Topic、JSON schema 與安全控制合約。Node-RED 端可以使用 Debug sidebar 觀察 `msg`；ESP32 端仍以 OLED 為主要可觀測介面，不能把 Serial Monitor 或 Node-RED 畫面當成裝置現場狀態的替代品。

## 章節順序

1. [Node-RED 環境、CLI 與專用 userDir](01_environment.md)
2. [Flow、Node、Wire 與 Message](02_flow_message.md)
3. [Inject、Debug、Change、Switch 與 Function](03_core_nodes.md)
4. [Node-RED 與 MQTT TLS](04_mqtt.md)
5. [JSON 解析、欄位驗證與異常分流](05_json_validation.md)
6. [FlowFuse Dashboard 2 即時儀表板](06_dashboard.md)
7. [雙向安全控制、匯出與還原](07_safe_control_backup.md)

請依章序學習。第 2、3 章先使用 DEMO；第 4 章設定 MQTT，第 5 章可先用本地 DEMO 驗證，再接第 4 章資料；第 6 章加入 Dashboard。第 7 章要先取得有效 status／boot ID、主機時間與持久序號，才能產生命令；送出後再比對 ACK。

## 鎖定環境

| 元件 | 本教材基準 |
|---|---|
| Node.js | 24.x LTS |
| Node-RED | 5.0.7 |
| Dashboard | `@flowfuse/node-red-dashboard` 1.31.0 |
| Editor 連線 | 第一次限本機 `127.0.0.1:1880` |
| MQTT | 依 Broker 規格使用 TLS；本教材 Flow 以 8883 作為教學預設 |

Node-RED 5.0.7 的套件條件包含 Node.js 24，Dashboard 1.31.0 也宣告支援 Node-RED 3.0.0 以上。這些條件表示版本範圍相容，不代表上游已對這三個精確版本完成同一組整合測試。本 Repository 會另行記錄本地啟動、Flow 載入與 Dashboard 畫面驗證的證據邊界。

## 教材目錄與匯入順序

- 課文：`docs/part4/`
- 可公開 Flow：`nodered/flows/`
- 環境版本：`nodered/package.json` 與 `nodered/.node-version`
- 實際執行的 userDir：學生本機專用目錄，不放進 Repository

匯入 Flow 時使用 Node-RED Editor 右上角選單的 **Import**，一次匯入一章。匯入後先檢查所有紅色三角與未知節點，再輸入本組的 Broker 設定；不可一匯入就直接 Deploy。第 6、7 章各含一個 `ui-base`，FlowFuse Dashboard 2 不支援同一 Runtime 同時存在多個 `ui-base`，兩章必須分開匯入／Deploy。

## 公開安全邊界

- 本 Repository 不提供真實 Broker host、使用者名、密碼、CA 檔案、內網 IP、Topic root 或 Client ID。
- 公開 Flow 使用 `REPLACE_WITH_...` 占位值；匯入後由學生在本機 Editor 輸入。
- `cmd`、`data`、`ack` 不使用 Retain；只有 `status` 可依第三篇合約使用 Retain。
- 每組必須使用唯一 Topic root、Client ID 與最小 ACL，不訂閱 `#` 或 `+`。
- Editor 與 Dashboard 不直接暴露到 Internet。若要離開受信任教室 LAN，必須另行設計 Editor/Admin API、Dashboard HTTP 與 Socket.IO 的驗證與反向代理。
- Flow Export 不是秘密掃描器。匯出 JSON 仍可能包含 host、Topic、Client ID、檔案路徑與 Function 內常數，提交前必須人工與自動檢查。

## 本篇完成條件

- 專用 userDir 能啟動 Node-RED，畫面與啟動紀錄顯示鎖定版本。
- 六份教學 Flow 能在新建 userDir 匯入，不依賴舊 `.node-red` 目錄。
- DEMO 流程可分辨正常、缺欄位、型別錯誤、範圍錯誤與資料過期。
- MQTT 必須驗證伺服器憑證，精確 Topic 的 Publish／Subscribe 與 ACL 由學生 Broker 另行實測。
- Dashboard 必須把 `DEMO／非實機` 、`LIVE`、`STALE` 與 `OFFLINE` 區分顯示，不留下沒有時效狀態的舊數字。
- 控制只在取得當次 `boot_id` 後才可產生命令；指令必須有遞增序號、TTL、不 Retain，ACK 不得被標示為實體回授。
- 真實操作截圖必須可追溯版本、Flow Commit、DEMO／實測邊界與遮蔽處理。

舊 AB143 `flows.json` 只作為「MQTT 能源資料 → 拆欄 → Gauge／Chart」的概念來源，不直接複製舊 Dashboard 節點或明文 MQTT 設定。完整遷移理由見 [第四篇來源與遷移記錄](source-map.md)。

## 教材驗證狀態

7 章課文、6 份公開 Flow 與本機操作證據已完成。自動測試涵蓋實際 Runtime 的資料流、Dashboard 時效與控制阻擋；硬體及外部 Broker 驗收仍依本篇完成條件另外執行。詳見[驗證紀錄與重現方法](verification.md)。
