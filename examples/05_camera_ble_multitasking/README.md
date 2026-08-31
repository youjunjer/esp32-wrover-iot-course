# 第五篇：相機、藍牙與多工

本篇使用的開發板設定仍然是：

```text
esp32:esp32:esp32wrover
```

啟用板載相機時，程式內使用：

```cpp
#define CAMERA_MODEL_AI_THINKER
```

不將開發板 FQBN 改成 AI Thinker。

規劃收錄：

- Bluetooth Classic
- BLE 掃描與 Beacon
- FreeRTOS Task 與雙核執行
- AI Thinker 相機腳位配置
- CameraWebServer
- 拍照與 Base64
- MQTT 影像傳輸
- 感測器觸發拍照
- 相機、網路與背景工作的多工整合
