# 5. 判斷、迴圈與函式

## 條件判斷

`if` 適合依按鈕或狀態決定輸出：

```cpp
if (digitalRead(BUTTON_PIN) == LOW) {
  digitalWrite(LED_PIN, HIGH);
} else {
  digitalWrite(LED_PIN, LOW);
}
```

按鈕使用 `INPUT_PULLUP` 時，按下會讀到 `LOW`，稱為低電位有效。

## 迴圈

`for` 適合固定次數的重複工作，例如啟動時閃爍三次或逐步改變亮度：

```cpp
for (uint8_t count = 0; count < 3; ++count) {
  // 重複三次
}
```

`while` 適合條件成立時持續執行，但不可以讓程式永久卡在沒有可見狀態的等待迴圈。網路章會在 OLED 顯示重試進度。

## `switch`

當一個狀態有多個明確選項時，`switch` 比很多層 `if` 容易閱讀：

```cpp
switch (mode) {
  case 0:
    digitalWrite(LED_PIN, LOW);
    break;
  case 1:
    digitalWrite(LED_PIN, HIGH);
    break;
  default:
    digitalWrite(LED_PIN, LOW);
    break;
}
```

每個 `case` 通常要用 `break` 結束，`default` 負責未預期的值。

## 陣列與結構

陣列可儲存同類型的多個值：

```cpp
constexpr uint8_t LEVELS[] = {0, 64, 128, 255};

for (const uint8_t level : LEVELS) {
  ledcWrite(LED_PIN, level);
}
```

範圍式 `for` 會依序取出陣列元素。如果一組資料有多個欄位，可用 `struct` 放在一起；`07_rgb_led` 便將紅、綠、藍三個數值組成一個 `RgbColor`。

## 函式

把可重複的動作整理為函式：

```cpp
void setTrafficLights(bool redOn, bool yellowOn, bool greenOn) {
  digitalWrite(RED_PIN, redOn ? HIGH : LOW);
  digitalWrite(YELLOW_PIN, yellowOn ? HIGH : LOW);
  digitalWrite(GREEN_PIN, greenOn ? HIGH : LOW);
}
```

這比在每個流程重複三行 `digitalWrite()` 更容易檢查，也能集中處理安全的全關狀態。

## 阻塞與非阻塞

第一篇使用 `delay()` 幫助理解順序；但 `delay()` 期間不能處理其他工作。按鈕防彈跳會開始使用 `millis()`，後續網路與多工章則以非阻塞方式更新狀態。

## 練習

為紅綠燈加入「全部關閉」函式，再調整綠、黃、紅的持續時間。重啟時先確認三顆 LED 都保持關閉。
