# 7. PWM、RGB LED 與 WS2812

## PWM 與 ESP32 Core 3.x

PWM 以快速切換輸出模擬不同平均亮度。本教材鎖定 ESP32 Core 3.3.11，使用新版 LEDC API：

```cpp
ledcAttach(PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
ledcWrite(PWM_PIN, duty);
```

不使用舊版 `ledcSetup()`、`ledcAttachPin()`，也不沿用 AB143 舊範例的 `analogWrite()` 寫法。

8 位元解析度的 duty 範圍為 0～255。`06_pwm_fade` 使用 `for` 迴圈逐步增加及減少 duty，形成平滑呼吸效果。

![PWM 在 20、50、80 與 100 百分比工作週期下的波形](../assets/ab143/part1/pwm-waveform.png)

*圖：工作週期愈大，每個週期保持 HIGH 的時間愈長。來源：AB143 三版，原書頁 P46；由課本向量圖裁切。*

## 共陰極 RGB LED

本教材的 RGB 範例採共陰極接法：

| RGB 腳 | GPIO | 接線 |
|---|---:|---|
| R | 15 | GPIO 15 接紅色腳 |
| G | 2 | GPIO 2 接綠色腳 |
| B | 4 | GPIO 4 接藍色腳 |
| 共陰極 | — | 接 GND |

![共陰極 RGB LED 的 Red、GND、Green、Blue 腳位順序](../assets/ab143/part1/rgb-led-pinout.jpg)

*圖：課本中的共陰極 RGB LED 腳位示意；不同廠牌的腳序可能不同，接線前要核對零件規格。來源：AB143 第四版第 4 章，原書頁 P53。*

本範例只正式支援共陰極 RGB LED；共陽極接法可將啟動綁定腳位拉高，必須另選腳位與驅動方式並實測後才能使用。

`07_rgb_led` 以三路 LEDC PWM 混合紅、綠、藍、黃、青、紫與白色，並以較低 duty 開始，避免過亮。

RGB 色表使用結構陣列：

```cpp
struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

constexpr RgbColor COLORS[] = {
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
};
```

`for (const RgbColor &color : COLORS)` 依序取出每組顏色；`&` 表示以參照讀取原元素，`const` 保證迴圈內不會修改色表。

## WS2812

WS2812 將控制晶片與 RGB LED 整合，使用單一資料腳傳送每顆 LED 的顏色。

- 資料腳：GPIO 32
- 顆數：1
- 函式庫：`Adafruit NeoPixel` 1.15.5
- 色彩順序：`NEO_GRB + NEO_KHZ800`
- 第一個範例限制亮度，避免單顆白光使用過大電流。

一般接線：

```text
ESP32 GND ── WS2812 GND
GPIO 32 ── WS2812 DIN
依模組規格供電，ESP32 與 WS2812 必須共地
```

不要把 DIN 與 DOUT 接反。外接較多 WS2812 時要重新計算電流並使用合適的獨立電源，本章只驗證單顆 LED。

## 可見驗收

- `06_pwm_fade`：亮度平滑變化，沒有明顯跳階或一直全亮。
- `07_rgb_led`：每個基本色正確，白色時三色都亮。
- `08_ws2812`：依序顯示白、紅、綠、藍，最後確實熄滅。

如果顏色順序錯誤，先確認 RGB 腳位或 WS2812 色彩順序設定，不要只用軟體補償未確認的接線。
