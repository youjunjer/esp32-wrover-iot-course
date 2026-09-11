# 第四篇驗證紀錄

驗證日期：2026-09-11（Asia/Taipei）。教材基準為 Node.js 24、Node-RED 5.0.7、FlowFuse Dashboard 1.31.0；本機實際 Node.js 為 24.19.0，使用 npm 11.19.1 的 `npm ci` 安裝既有 lockfile。

## GitHub Actions 成功紀錄

[Run 34605058274](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/34605058274) 已在 2026-09-11 21:46（Asia/Taipei）完成，對應程式與工作流程 Commit `200f84a0066193956662075a67843100a218c355`。

- `nodered` job：1 分 5 秒成功；GitHub Runner 使用 Node.js 24.20.0、Node-RED 5.0.7、Dashboard 1.31.0，四組 Runtime 測試全部 PASS。
- `compile` job：12 分 3 秒成功；policy 檢查與鎖定 Arduino 工具鏈通過，log 中共 31 行 `Compiling examples/`，全部 Sketch 編譯完成。
- 本次使用 `actions/setup-node@v7`，Action 本身也執行於 Node.js 24。

本段及教材入口連結在成功後以文件提交補記，沒有改動已驗證的 Flow、測試、lockfile、Arduino 程式或 CI 工作流程。

## 已完成

| 層級 | 實際驗證 | 能證明的範圍 |
|---|---|---|
| 靜態檢查 | 六份 JSON、Wire／Link 目標、版本鎖定、公開占位設定、Function 語法與合約 | Flow 結構、依賴與規則符合教材 |
| 環境 | 全新 userDir、鎖定套件、CLI 啟動、Editor、Safe mode | 本機可啟動與載入；Safe mode 不自動執行 Flow |
| 第 2、3 章 | 實際 Inject、訊息 ID、28°C → 82.4°F、型別與範圍錯誤分流 | Core Node 的本地資料路徑 |
| 第 5 章 | 合法、範圍錯誤、JSON 解析錯誤、過期、未知欄位、序號型別、385 bytes | 驗證器與安全摘要；沒有把原始錯誤 Payload 送到後段 |
| 第 5 → 6 章 | 原始八欄加 validation envelope 經 Link 送至 widgets；保留 DEMO 標記 | 章節之間的訊息合約與實際連線 |
| 第 6 章 | DEMO 正常／INVALID／STALE；合成 LIVE 後等待原設定的 45 秒 | 無效資料不更新儀表；watchdog 到期產生 OFFLINE |
| 第 7 章 | 無 status、缺少或別名 file store 阻擋；合成 status／ACK、斷線阻擋 | 軟體 gate 與 ACK 配對，不是實體回授 |
| 持久序號 | 同一暫存 userDir 正常停止，再以新 Node.js 行程重啟 | file context 序號由 1 變成 2；不代表突然斷電零資料遺失 |
| 圖像追溯 | 18 張真實操作圖、環境輸出文字、六份 Flow 的 SHA-256 | 檔案與本次擷取版本一致；後續異動會使檢查失敗 |

Runtime 測試載入原始公開 JSON，不修改 Function、Wire 或 45 秒門檻；測試程式直接對節點注入合成訊息並觀察訊息傳遞。所有 MQTT Broker 均保持 `autoConnect=false`，測試並未對外連線或控制硬體。

## 重現自動測試

從 Repository 根目錄執行（需 Node.js 24、npm、jq 與 ripgrep）：

```bash
cd nodered
npm ci
cd ..
bash scripts/check-policy.sh
node scripts/test-nodered-runtime.mjs
```

Runtime 測試會建立與清除專屬暫存 userDir，不讀取個人 `.node-red`。其中 45 秒 watchdog 使用正式時間，因此完整測試約需一分鐘。GitHub Actions 以獨立 `nodered` job 執行同一測試；Arduino job 維持全部 31 個 Sketch 的編譯。

## 重現操作截圖

維護工具為 [`scripts/capture-nodered.mjs`](../../scripts/capture-nodered.mjs)，需另外提供 Playwright Core 與 Chrome，學生無須安裝這兩個截圖工具。

1. 依第一章建立乾淨 userDir；第 2～6 章載入同一 Runtime，第 7 章使用另一個 Runtime。
2. 保留公開 Flow 的全部占位值與停用 Broker，不放入真實設定。
3. 在 Repository 根目錄設定 `PLAYWRIGHT_MODULE_PATH` 與 `CHROME_EXECUTABLE_PATH` 為本機工具路徑。
4. 執行 `node scripts/capture-nodered.mjs 26 http://127.0.0.1:18881/`，另以 `07` 與第 7 章本機 URL 擷取控制畫面。
5. Safe mode 使用第 2～6 章的另一個乾淨 userDir，加上 `--safe` 啟動，再以 `safe` 模式擷取。
6. 工具先比對 Runtime 的 `/flows` 與 Repository JSON 完全一致，才擷取；開啟設定視窗後一律取消，不 Deploy 修改。
7. 依實際產生的 metadata 更新 [SOURCES.md](../assets/part4/captures/SOURCES.md)；不得只更新雜湊而省略重新執行與擷取。

CLI 輸出見[環境驗證文字](../assets/part4/captures/environment-verification.txt)。它是實際指令／Runtime log 的輸出，僅將一次性本機路徑換成明示占位值，沒有生成或重打假的終端畫面。

## 仍須實機驗收

- Broker 的 DNS、TLS 憑證鏈、ACL、Client ID、QoS、Retain、Last Will 與斷線重連。
- ESP32 OLED 與 Node-RED 同次資料、命令及 ACK 對照。
- 感測器校正、GPIO、SG90、繼電器無市電負載測試與租約到期。

這些項目依第四篇各章驗收表執行；本機 DEMO、合成訊息、截圖與 CI 不能替代。
