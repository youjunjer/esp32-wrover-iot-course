# 第四篇真實操作截圖來源

擷取日期：2026-09-11（Asia/Taipei）。所有圖片由本機 Google Chrome 對實際 Node-RED Editor／Dashboard 擷取，未重畫內容，沒有生成圖片或舊版教材畫面。

- 第四篇 Flow／測試程式 Commit：[`4d516486304577c1a5ce48c3e2754c609b8ca619`](https://github.com/youjunjer/esp32-wrover-iot-course/commit/4d516486304577c1a5ce48c3e2754c609b8ca619)。擷取時載入的公開 JSON 與該 Commit 的 Git blob／SHA-256 相同，詳見文末。
- 作業系統：macOS Darwin 25.5.0 arm64。
- Node.js 24.19.0、npm 11.19.1、Node-RED 5.0.7、FlowFuse Dashboard 1.31.0；透過 npm ci 安裝 Repository lockfile。
- Google Chrome 152.0.7977.84、Playwright Core 1.62.1、headless、zh-TW，1600 × 1000 viewport；Dashboard 依實際內容使用 full-page screenshot。
- 來源程式：`scripts/capture-nodered.mjs`；每次先比對 Runtime `/flows` 與公開 JSON 完全一致。
- 啟動命令形式：`node-red --userDir TEMP_COURSE_DIR --port PORT -D uiHost=127.0.0.1 --no-telemetry`；Safe mode 另加 `--safe`。
- 第 2～6 章與第 7 章分別使用乾淨 userDir，監聽本機 18881／18882；Safe mode 另用 18883。它們都沒有讀取個人 `.node-red`。
- 操作：用內建 Inject 產生 DEMO、展開 Debug、開啟實際節點設定後取消；未修改 Flow、未 Deploy 私人設定。截圖工具僅拍攝頁面，沒有注入替代成功文字。
- 圖片未裁切、重繪或遮蔽；畫面只有公開 Flow、合成 DEMO、占位設定與隨機訊息 ID，不含真實裝置識別資訊或 credentials。

## 圖檔追溯

| 圖檔 | 實際內容與擷取時間（UTC） | SHA-256 |
|---|---|---|
| `node-red-5-editor-message-flow.png` | 04-02 Flow 與 Message: unchanged public Flow in the editor；2026-09-11T13:26:56.127Z | `2625b907037bfbd0bceeaa31148d827229f834f784b69fee2e04e0761e4e3dd1` |
| `node-red-5-core-nodes-flow.png` | 04-03 基礎節點: unchanged public Flow in the editor；2026-09-11T13:26:56.539Z | `bd9583c22c6a36453b8e105e40c52e7bfc66b9f43a2ca5ca65bf73984e22fcfb` |
| `node-red-5-mqtt-tls-flow.png` | 04-04 MQTT TLS 發布與訂閱: unchanged public Flow in the editor；2026-09-11T13:26:56.952Z | `a130baf6951111e49839bd4cb3877fa069aac73d846fd2683dd1cd229b426134` |
| `node-red-5-json-validation-flow.png` | 04-05 JSON 驗證: unchanged public Flow in the editor；2026-09-11T13:26:57.330Z | `b3893c31f6ce7d869f113f180d52d693b5d7a7ba07097f1415625f82f0a6f2bd` |
| `node-red-5-dashboard-flow.png` | 04-06 FlowFuse Dashboard: unchanged public Flow in the editor；2026-09-11T13:26:57.740Z | `b24208a8714c210c5d12a279fdcc805ce4a5ef3cf744a276e9aba5de6c904945` |
| `node-red-5-message-debug.png` | lesson 2 actual DEMO message in Debug；2026-09-11T13:26:59.322Z | `1ade507c9466c24979998657a4fab50dc2e028783aab4c9d54826dfeca904023` |
| `node-red-5-core-debug.png` | lesson 3 valid, wrong-type and range-error DEMO outputs；2026-09-11T13:27:00.970Z | `a5dabb4ed1114c4d404d232477d7f140a023dc93eee1ff7e3dc0155095cfed31` |
| `node-red-5-json-debug.png` | lesson 5 valid, range-error and parser-error DEMO outputs；2026-09-11T13:27:02.474Z | `3a0fca6a6435d40ebd0bc4691b32cd2c5056e69f05e1307b3990a0a6de1175d8` |
| `node-red-5-change-settings.png` | lesson 3 actual change editor dialog；2026-09-11T13:27:03.548Z | `85a5a5e113ded7221ef4a8a1fd74c0c636428198358648ada3ef005428afabb8` |
| `node-red-5-function-settings.png` | lesson 3 actual function editor dialog；2026-09-11T13:27:04.517Z | `5a7e78eaf8ad7202e9f5f26543f83fe71e482f4129d8dda5d2cccfe172e46d88` |
| `node-red-5-switch-settings.png` | lesson 3 actual switch editor dialog；2026-09-11T13:27:05.399Z | `452c6ee23d3a4ab3693282809989eb2acd20689505d9f244eb396c1a5bd362c8` |
| `node-red-5-tls-settings.png` | public TLS config: certificate verification enabled, no private CA or credentials；2026-09-11T13:27:06.255Z | `13fbd55e523924c5b266218a4c52789304d74a3511c7eb549aab3cc1cfe952f1` |
| `flowfuse-dashboard-demo.png` | lesson 6 actual VALID DEMO; never hardware data；2026-09-11T13:27:08.382Z | `2d6212e77e8863d7a216bc1571553628ea3e5186b6efa373b78afa7070e5378f` |
| `flowfuse-dashboard-invalid.png` | lesson 6 actual INVALID DEMO; never hardware data；2026-09-11T13:27:09.417Z | `f90b236d77be595f7c2294951056fc96c733a5caaac7470f9435b0882892de96` |
| `flowfuse-dashboard-stale.png` | lesson 6 actual STALE DEMO; never hardware data；2026-09-11T13:27:10.448Z | `50c8d570df5d4b30f965cc9865c5849b18c5f892f5474a857f39b741edc9e9e9` |
| `node-red-5-safe-control-flow.png` | lesson 7 unchanged public Flow; MQTT disabled；2026-09-11T13:20:41.737Z | `de3f79976290a72138ddfe808573f6bae23c7b1ef754e2d51094718dad41173a` |
| `flowfuse-safe-control-locked.png` | lesson 7 missing file context and device status: WAIT/BLOCKED；2026-09-11T13:20:43.380Z | `ef9556478ac4f4677628a2a8bf1d9c301b6342b8973e0aac4b4adbd39e12d531` |
| `node-red-5-safe-mode.png` | CLI --safe: editor loaded without starting any Flow；2026-09-11T13:22:39.966Z | `4bb5578d19f335fa72de7e68ece601afa3866896022cbe3fa0192a0e929edc9b` |

## CLI 紀錄

本檔只保留實際版本／JSON 解析輸出與相關 Runtime log；本機一次性路徑明示替換為 `<TEMP_COURSE_DIR>`。未製作仿終端圖片。

| 檔案 | 內容 | SHA-256 |
|---|---|---|
| `environment-verification.txt` | 實際版本、FLOW JSON OK、兩個 Runtime 與 Safe mode 啟動摘錄 | `43744a2913f4b28ea404682d4c1070bb0e6c6522df37bf67f9fcc5a5fafc4c10` |

## Flow 與程式碼追溯

| Flow | Git blob | SHA-256 |
|---|---|---|
| `02_message_path.json` | 5507635cbe422fb9ec9d3a03a216b72b2d3f7f97 | `f46e319db3309b10075cb43d107da63798fdab6dc3f1501e5ea0d82edd0c92ce` |
| `03_core_nodes.json` | 96ddea181fa9a07a456dbe5c516d6922acd1113a | `37ebede2617269c32203f4c47ea5faf9e6c5b2ff1782ca442d5639742e900ea2` |
| `04_mqtt_tls_pubsub.json` | e7299f37870fb8f0e05e6a875ca625e646dec244 | `f7a5aca1ad0ccb1563673d66794168bc1df262944f233d0c1b249ca2ab251535` |
| `05_json_validation.json` | dc4bbda88f0fe56be3d1d9259bf915ce2d3e1ef8 | `4aff12c10be3ed60353ca24b6a6c72b26210f87f550af2ff8bb5b1394cae79ba` |
| `06_flowfuse_dashboard.json` | c622ed14d2fa8fc0cf429b5f33747e3a63986f20 | `5c9f7f224b0acd53db941923026f0fcc791e2e466809972e52056832d995a592` |
| `07_safe_control.json` | eba56443aeba3d0e585fd1724124dc95067c0f55 | `8006b93d349588e1e6a09f715499c51d48fed5cce0c223c616a26a321da98c7c` |

## 證據邊界

- 編輯器、設定與 Safe mode 截圖證明對應公開 Flow 能在鎖定 Runtime 載入；設定為 TLS 不表示已完成實際連線。
- Dashboard 的 26.4 °C、63 %RH、2048 ADC 來自內建 DEMO。INVALID／STALE 圖像保留先前接受值並呈現錯誤狀態；LIVE 仍為 OFFLINE。
- 控制畫面在沒有 status／boot ID 與具名 file context 時維持 WAIT／BLOCKED；按鈕可見不等於已准許控制。
- MQTT Broker 保持 autoConnect=false，沒有對外 MQTT 連線，也沒有使用 ESP32、OLED、感測器、relay、SG90 或市電負載。
- Runtime 自動測試中的合成 LIVE、status／ACK 與正常重啟持久序號另見 `docs/part4/verification.md`；這些測試不冒充實體硬體結果。
- 本次未呼叫 ThingSpeak、Google Sheets 或教師既有 GAS。
