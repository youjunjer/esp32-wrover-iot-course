# 第四篇來源與遷移記錄

## 來源範圍

第四篇延續 AB143 的 MQTT／能源資料流概念，並參考能源監測專案對 Dashboard 與雙向控制的需求。舊資料只用來辨識欄位、資料流、儀表與課程順序；Node-RED 5 編輯器、專用 `userDir`、Core Node 操作與 FlowFuse Dashboard 2 都以新版實作及實際截圖為準。

## 新版基準

| 項目 | 鎖定值 | 決策 |
|---|---:|---|
| Node.js | 24.x | 依 Node-RED 5 建議的執行環境作為課程基準 |
| Node-RED | 5.0.7 | 以局部 npm 套件鎖定，不依賴版本不明的全域安裝 |
| FlowFuse Dashboard | 1.31.0 | 安裝 `@flowfuse/node-red-dashboard`；不安裝舊 `node-red-dashboard` |
| Runtime 資料 | 專用 `ESP32_NodeRED_Runtime` | 用 `--userDir` 明確指定，不搬入舊 `.node-red` |
| 初期監聽 | `127.0.0.1:1880` | 尚未設定編輯器認證與 HTTPS 前，不暴露到 LAN 或 Internet |

鎖定值是 Repository 建立當下的教材基準，不把它們寫成永久的「最新版」。

## 前三章來源對照

| 新章節 | 主要來源 | 可沿用的概念 | 必須重建或淘汰 |
|---|---|---|---|
| 1. 環境與 userDir | Node-RED 官方 Local／Runtime 文件；本 Repository `nodered/README.md`、`docs/CONVERSATION_HANDOFF.md` | CLI 啟動、`userDir`、Flow 檔與終端 log | 全域浮動版本、舊 `.node-red`、舊 credentials、舊 `node_modules`、未防護的全網路介面監聽 |
| 2. Flow 與 Message | Node-RED 官方 Editor、Flows、Nodes、Wires 與 Messages 文件 | Flow tab、port、Wire、`msg.payload`、`msg.topic`、`_msgid` 與 Debug 追蹤 | 使用舊編輯器截圖、把 JSON 字串誤當 object、以 Debug 代替 ESP32 OLED |
| 3. Core Nodes | Node-RED 官方 Core Nodes 與 Function 文件；AB143 `flows.json` 的 Change／Debug 概念 | Inject、Debug、Change、Switch、Function；依欄位與範圍分流 | 靜默轉型、無 Otherwise 的無聲丟棄、用 Function 取代所有基本 Node、把私密 Payload 寫入 log |

## 後四章來源對照

| 新章節 | 主要來源 | 沿用或建立的內容 | 明確邊界 |
|---|---|---|---|
| 4. MQTT TLS | 第三篇 MQTT Topic／QoS／Retain 合約；Node-RED 5.0.7 MQTT 與 TLS Core Node | 8883、驗證伺服器憑證、精確 data／status／ack／cmd Topic、UTF-8 raw input、安全狀態摘要 | Broker 預設 `autoConnect=false`；不含 host、Client ID、帳密、CA；未做外部 Broker 實測 |
| 5. JSON 驗證 | 第三篇 Schema v1；Node-RED JSON／Function／Catch | 解析前 1～384 UTF-8 bytes、精確 Topic／device、八欄、範圍、未來 5 秒與 stale 10 秒 | 拒絕分支不回顯原 Payload；軟體驗證不等於資料真實或感測器校正 |
| 6. Dashboard 2 | AB143 儀表／趨勢概念；FlowFuse Dashboard 1.31.0 | Text、Gauge、Chart、DEMO／LIVE／STALE／OFFLINE 區分 | 不沿用舊 `ui_*`；公開截圖只有本地 DEMO，沒有 ESP32、MQTT 或感測器 |
| 7. 安全控制 | 第三篇 status／boot ID／cmd／ack 合約；能源專案控制需求 | file-backed 序號、TTL、QoS 1、cmd 不 Retain、ACK 與實體回授分離、fail-close | Broker 預設停用；無 `file` context 時阻擋；relay 無市電、SG90 獨立 5V 共地後才可實測 |

## AB143 舊 Flow 盤點

本地來源：

```text
Google Drive/1_ESP32教學資料/AB143：ESP32使用C/flows.json
```

審查結果：

- JSON 有效，共 22 個 Node。
- SHA-256：`20a2a536955f40e5ffc75bfdc191c7a21a52152be3e36e740103d43fbd7bc9c0`。
- 資料流是 MQTT In → 四個 Change → Gauge／Chart，另有一個 Debug。
- Change 欄位為 `payload.V`、`payload.I`、`payload.W`、`payload.kWh`，可作為後續能源資料分流的需求參考。

不能原樣匯入新課程的原因：

- Broker 設為明文 1883，沒有 TLS、ACL 與每組獨立 Client ID。
- 使用固定共用 Topic，不符合第三篇已建立的組別命名空間。
- `ui_gauge`、`ui_chart`、`ui_tab`、`ui_group` 屬於舊 Dashboard 節點，不是 FlowFuse Dashboard 2 的新版 Flow。
- 沒有 `package.json` 或版本記錄，無法追溯當時 Node-RED 與 Dashboard 版本。
- 沒有 Inject、Switch、Function、JSON、MQTT Out、異常路徑與雙向控制。
- 未檢查 schema、型別、範圍、時間、序號或 stale 狀態。
- 檔案本身沒有授權與作者資訊；只能作內部遷移依據，對外發布前仍要確認權利與建立來源記錄。

決策：保留「一筆能源 object 分成 V／I／W／kWh」的學習目標，但所有 Node、Broker 設定、安全檢查、Dashboard 與截圖重新建立。

## 能源專案歷史畫面邊界

`esp32-mqtt-energy-meter/結案報告/` 可找到舊 Node-RED 編輯器、能源 Dashboard 與影像 Flow 畫面，但不納入前三章操作指引：

- `未命名.png`：能源分流概念可參考，但是舊 Dashboard Node、畫面裁切且含課堂名稱。
- `748883484_1030578019369667_6312289331201044388_n.jpg`：舊能源 Dashboard 結果，只能在權利確認後標為歷史成果，不能當 Dashboard 2 指引。
- `3.png` 與 `747536353_1312789540838556_185588493872055155_n.jpg`：使用額外 image-preview／base64 節點，後者還含可識別人物，不用於公開基礎教材。
- `4.png`：為其他 Dashboard Studio 介面，不得當作 Node-RED 畫面。

這些檔案缺少完整的工具版本、擷取日期、操作條件、作者與授權紀錄；在來源與隱私審查完成前不複製到公開課程 Repository。

## 現代化決策

### 專用與可重現環境

- 在 Repository 的 `nodered` 目錄用 `npm ci` 安裝 lockfile；另建 `ESP32_NodeRED_Runtime` 作為專用 userDir。
- 執行檔、`package.json`、`package-lock.json` 留在 Repository 的 `nodered` 目錄；Flow、credentials、settings 與 context 留在專用 userDir，不與個人舊 Runtime 混用。
- 不整包搬入舊 `settings.js`、`flows_cred.json`、`node_modules` 或 `.config.*`。
- 尚未配置編輯器認證與 HTTPS 前，用 `-D uiHost=127.0.0.1` 限制本機連線。

### Flow 與 Message

- 先用 Inject 產生明確標示的 DEMO object，在沒有 ESP32 與 Broker 的條件下就能驗證資料流。
- 每一個轉換先檢查型別與範圍，錯誤路徑明確送往 Debug，不無聲丟棄。
- Change 處理賦值、搬移、刪除與簡單轉換；Switch 處理分流；Function 保留給有界 JavaScript 計算。
- Function 只回傳 `msg` object 或 `null`，不將帳密、未過濾外部輸入或完整私密 Payload 寫入 log。

### Debug 與 OLED

- Node-RED Runtime 啟動／套件錯誤看 CLI log；Flow 訊息與分流結果看 Debug sidebar。
- ESP32 本身仍以 OLED 為主要觀察介面，不能要求學生只看 Node-RED Debug 推測裝置狀態。
- 後續端到端驗收要同時保留 Node-RED 收發狀態與 ESP32 OLED 畫面，其中任一端成功都不代表另一端成功。

### Dashboard

- 第 1 章先安裝鎖定的 `@flowfuse/node-red-dashboard@1.31.0`，但前三章只做環境、Message 與 Core Node 練習。
- 舊 `node-red-dashboard` 已淘汰，舊 `ui_gauge`／`ui_chart` Flow 不冒充 FlowFuse Dashboard 2 相容範例。
- 第 6、7 章分開載入 Dashboard；目前截圖與驗證詳見下方紀錄。
- 第 6、7 章各自帶有一個 `ui-base`。FlowFuse Dashboard 2 同一個 Runtime 不支援多個 `ui-base`，因此兩章必須在分開的乾淨 Runtime 匯入／Deploy，不能合併成一份執行 Flow。

## 2026-09-10 本機 Runtime 驗證

本次以 Codex 工作區提供的 Node.js 24.19.0，在 `/tmp` 建立一次性乾淨 userDir，安裝並實際啟動 Node-RED 5.0.7 與 `@flowfuse/node-red-dashboard` 1.31.0。正式 Repository 沒有提交該 userDir、`node_modules`、settings、credentials 或 context 資料。

- 第 2～6 章 Flow 在同一乾淨 Runtime 載入，啟動 log 顯示 `Started flows`，Dashboard 1.31.0 於本機 `/dashboard` 啟動。
- 第 6 章按下合法 DEMO Inject 後，Dashboard 實際顯示 26.4 °C、63 %RH、2048 ADC 與 `DEMO ONLY／not hardware data`。
- 第 7 章改用另一個只載入該章 Flow 的乾淨執行狀態，Dashboard 實際顯示 `WAIT` 與 `BLOCKED`；Broker 停用且 memory context 不冒充 `file` context。
- 將第 6、7 章同時載入時，Dashboard 1.31.0 明確拒絕多個 `ui-base`。因此教材已把分章 Runtime 列為必要操作，而不是隱藏此相容限制。
- MQTT placeholder 未連線；未傳送任何外部 Broker、ESP32、ThingSpeak、Google Sheets 或教師 GAS 請求。

以上只證明鎖定版本能在本機啟動、Flow 節點能載入，以及兩個 Dashboard 的 DEMO／fail-closed 畫面。它不證明 ESP32、OLED、MQTT TLS／ACL、Broker、感測器、繼電器、SG90 或市電負載已通過。實際截圖與 SHA-256 見 [`docs/assets/part4/captures/SOURCES.md`](../assets/part4/captures/SOURCES.md)。

## 2026-09-11 收尾驗證

以現有公開 Flow 重新建立乾淨 userDir，依 `package-lock.json` 執行 `npm ci`，補拍全部本機教學畫面，並將截圖和 Flow 的 SHA-256 納入 policy 檢查。現行檔案與 9 月 10 日截圖不一致的三份 Flow 已改用本次驗證證據。

實際執行的自動測試涵蓋完整訊息、Core Node 分流、JSON 大小與 schema、跨章 Link、Dashboard DEMO／INVALID／STALE、45 秒 OFFLINE、缺少 file store 阻擋、ACK 配對，以及停止並重啟後持久序號由 1 遞增為 2。所有 Broker 均停用，沒有使用實體 ESP32。

現行操作步驟、版本與證據範圍見[第四篇驗證紀錄](verification.md)，圖檔與 Flow 追溯見[截圖來源](../assets/part4/captures/SOURCES.md)。圖片由真實 Node-RED Editor／Dashboard 擷取；CLI 使用經路徑去識別化的原始輸出文字，不製作仿終端截圖。

## 官方技術依據

- Node-RED [Version 5.0 released](https://nodered.org/blog/2026/06/09/version-5-0-released)：Node-RED 5 編輯器、Node.js 支援與安全變更。
- Node-RED [Supported Node versions](https://nodered.org/docs/faq/node-versions)：Node-RED 5 對 Node.js 的支援邊界；本課程固定使用 Node.js 24。
- Node-RED [Running locally](https://nodered.org/docs/getting-started/local)：CLI、`--userDir`、`--safe`、埠號與啟動 log。
- Node-RED [Runtime configuration](https://nodered.org/docs/user-guide/runtime/configuration)：`userDir`、`uiHost`、`uiPort` 與額外 Node 搜尋位置。
- Node-RED [Editor guide](https://nodered.org/docs/user-guide/editor/)、[Flows](https://nodered.org/docs/user-guide/editor/workspace/flows)、[Nodes](https://nodered.org/docs/user-guide/editor/workspace/nodes) 與 [Wires](https://nodered.org/docs/user-guide/editor/workspace/wires)。
- Node-RED [Working with messages](https://nodered.org/docs/user-guide/messages)：`msg`、`payload`、`_msgid` 與 Debug sidebar。
- Node-RED [The Core Nodes](https://nodered.org/docs/user-guide/nodes) 與 [Writing Functions](https://nodered.org/docs/user-guide/writing-functions)。
- FlowFuse Dashboard [Getting Started](https://dashboard.flowfuse.com/getting-started)：`@flowfuse/node-red-dashboard` 與 Dashboard 2 層級。
- FlowFuse Dashboard [Migration Guide](https://dashboard.flowfuse.com/user/migration.html)：舊 Dashboard 已不再主動開發，舊 Flow 通常需手動重建。
