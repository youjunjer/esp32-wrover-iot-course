# 第六篇：智慧能源監測專題

本篇將來自 `esp32-mqtt-energy-meter` 的漸進式範例整合為最終專題：

1. PZEM-004T UART2 與 OLED 通訊狀態
2. 能源資料解析、有效性檢查與 OLED 顯示
3. MQTT 能源資料發布
4. 低電位觸發繼電器
5. SG90 伺服馬達
6. DHT11 與光敏電阻
7. Node-RED 能源 Dashboard
8. 完整 OLED＋MQTT＋感測＋控制整合

匯入前必須修正繼電器啟動安全、固定 MQTT Topic、憑證、PZEM 封包驗證與舊資料保留問題。

所有能源數值、連線狀態與控制結果都必須在 OLED 顯示。PZEM 無資料、CRC 錯誤、逾時或資料過期時，必須以 `NO DATA`、`CRC ERR`、`TIMEOUT` 或 `STALE` 取代舊數值，不能只寫入序列監控器。
