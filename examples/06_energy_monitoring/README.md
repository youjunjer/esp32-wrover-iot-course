# 第六篇：智慧能源監測專題

本篇將來自 `esp32-mqtt-energy-meter` 的漸進式範例整合為最終專題：

1. PZEM-004T 資料讀取
2. OLED 能源顯示
3. MQTT 能源資料發布
4. 低電位觸發繼電器
5. SG90 伺服馬達
6. DHT11 與光敏電阻
7. Node-RED 能源 Dashboard
8. 完整 OLED＋MQTT＋感測＋控制整合

匯入前必須修正繼電器啟動安全、固定 MQTT Topic、憑證、PZEM 封包驗證與舊資料保留問題。
