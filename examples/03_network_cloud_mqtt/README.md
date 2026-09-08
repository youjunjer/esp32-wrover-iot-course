# 第三篇：網路、雲端與 MQTT

本篇先建立可觀察、可重試且不把憑證提交到 Git 的網路基礎，再加入雲端服務與 MQTT：

1. `01_wifi_oled`：Wi-Fi 非同步掃描、有限時連線與退避重試。
2. `02_ntp_clock_oled`：NTP 校時、臺灣時區與同步狀態。
3. `03_https_get_oled`：先有可信時間，再用 CA 驗證執行 HTTPS GET。
4. `04_air_quality_json_oled`：ArduinoJson 7、欄位驗證、資料年齡與公開空氣品質模式資料。
5. `05_webserver_oled`：教室區網 WebServer、安全啟動與 POST 控制。
6. ThingSpeak 資料上傳與圖表。
7. Google Sheets／Apps Script 資料紀錄。
8. MQTT 基礎：Broker、Topic、QoS、Retain 與 Client ID。
9. MQTT Publish／Subscribe。
10. MQTT JSON Payload。
11. MQTT 遠端控制。

第 1～5 章是目前整理範圍。HTTPS 章排在 NTP 後面，是因為 CA 憑證日期驗證需要可信系統時間；正式範例禁止以 `setInsecure()` 繞過驗證。ThingSpeak、Google Sheets 與 MQTT 都是正式教材，不列為歷史附錄，後續會依同一安全與 OLED 規範逐章加入。

每個學生或小組必須使用獨立的主機名稱、MQTT Topic 與 Client ID。實際 Wi-Fi、Web、雲端與 MQTT 資料只放在未追蹤的 `secrets.h`，Repository 只提交 `secrets.example.h`。

所有連線階段與錯誤都必須在 OLED 顯示，包括 Wi-Fi 掃描／連線／重試、取得網路時間、TLS／HTTP／JSON 錯誤、資料過期、ThingSpeak／Google Sheets 上傳結果，以及 MQTT 連線、發布與訂閱狀態。Serial 只能作為相同訊息的同步副本。

CI 編譯通過只證明程式與鎖定工具鏈相容，不代表 Wi-Fi、DNS、NTP、TLS、公開 API、瀏覽器或 GPIO 已在指定課程板完成實測。
