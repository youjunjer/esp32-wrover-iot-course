# 第三篇：網路、雲端與 MQTT

本篇正式包含：

1. Wi-Fi 連線與掃描
2. HTTPClient
3. JSON 解析
4. NTP 網路校時
5. 公開資料 API
6. ThingSpeak 資料上傳與圖表
7. Google Sheets／Apps Script 資料紀錄
8. MQTT Publish／Subscribe
9. MQTT JSON Payload
10. MQTT 遠端控制

ThingSpeak、Google Sheets 與 MQTT 都是正式教材，不列為歷史附錄。舊版作法必須先確認服務及 API 仍受支援，失效的流程改用現行作法。

每個學生或小組必須使用獨立的 MQTT Topic 與 Client ID。

所有連線階段與錯誤都必須在 OLED 顯示，包括 Wi-Fi 掃描／連線／重試、取得網路時間、HTTP 回應碼、ThingSpeak／Google Sheets 上傳結果，以及 MQTT 連線、發布與訂閱狀態。Serial 只能作為相同訊息的同步副本。
