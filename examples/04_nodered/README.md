# 第四篇：Node-RED

Node-RED 獨立為一篇，與第三篇的 MQTT 連接，但不混在同一章。

本篇已完成 7 章課文與 6 份可匯入 Flow。Node-RED 程式是 JSON Flow，正式檔案集中在 `nodered/flows/`，本目錄作為 Arduino 範例分類中的教材入口。

- [第四篇課文與章節順序](../../docs/part4/README.md)
- [安裝環境與六份 Flow 清單](../../nodered/README.md)
- [本機 Runtime 驗證、截圖與實機驗收範圍](../../docs/part4/verification.md)

教材涵蓋：

- Node-RED 安裝與資料目錄
- Flow、Node、Wire 與 Message
- MQTT In／MQTT Out
- Debug、Change 與 Function
- JSON 資料處理
- Dashboard 與圖表
- 設備控制
- Flow 匯入、匯出與備份

與 Node-RED 連線的 ESP32 範例仍須在 OLED 顯示 Wi-Fi、MQTT、發布、訂閱與控制狀態；不能只依賴 Node-RED Debug 或序列監控。Node-RED 端的流程錯誤則在 Dashboard／Debug 畫面呈現。
