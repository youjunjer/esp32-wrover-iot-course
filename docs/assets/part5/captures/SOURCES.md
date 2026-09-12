# 第五篇 CI 操作證據

- 擷取日期：2026-09-12（Asia/Taipei）。
- 真實來源：[GitHub Actions Run 34686731748](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/34686731748)。
- 實際測試 Commit：`f290ccb5d753b0a8f57020d439cb4a3b7c2b3dc9`。
- 工具：Chrome 152.0.7977.84、Playwright 1.62.1、1600×1100 viewport，未登入的獨立瀏覽器 context。
- 處理方式：直接瀏覽公開 Run Summary 並擷取完整頁面；沒有重畫訊息、注入成功狀態、遮蔽警告或合成工具輸出。另以 `gh run view --json ...` 與 `gh run view --log` 核對 40 筆正式 Sketch、3 個啟用分支及測試結果，再輸出精簡 JSON 紀錄。
- 隱私：只有公開 Repository、公開 GitHub 帳號、Commit 與 CI 工作資訊；不含本機帳號路徑、密碼、Token、真實 Wi-Fi／Broker 設定、板上識別或現場照片。
- 證據界線：編譯、程式合約與 Node-RED Runtime 成功，不等於 ESP32 燒錄、Bluetooth／BLE 無線通訊、OLED、PIR、相機共存或外部 Broker 實測。
- 原頁保留 Arduino setup Action 的 Node.js 20 相容性警告；該 Action 被平台以 Node.js 24 執行且整個工作成功。本次沒有宣稱 CI 零警告。

| 檔案 | 說明 | SHA-256 |
|---|---|---|
| `ci-part5-40-sketches-success.png` | 真實 GitHub Actions 成功摘要，顯示 compile 與 nodered 工作 | `447dc20b013d45a54d999c547b04a5fcb65238bdc3e72b7147df839d41d61955` |
| `ci-run-evidence.json` | 正式 Commit、工作時間、40 筆 Sketch、3 個分支、通過檢查與擷取工具資訊 | `49a1e820307b2903a7b1b60542fc63c49809722ce70f2b7e7a7dfcc1f087ba84` |
