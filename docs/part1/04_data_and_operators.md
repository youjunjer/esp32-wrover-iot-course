# 4. 變數、資料型態與運算

## 常用資料型態

| 型態 | 第一篇用途 | 範例 |
|---|---|---|
| `bool` | 開／關、按下／放開 | `bool ledOn = false;` |
| `uint8_t` | GPIO、0～255 PWM 值 | `uint8_t brightness = 128;` |
| `uint16_t` | 毫秒時間或較大正整數 | `uint16_t waitMs = 500;` |
| `unsigned long` | `millis()` 時間戳 | `unsigned long changedAt = 0;` |
| `int` | 一般整數計算 | `int step = 1;` |
| `float` | 帶小數點的感測或換算結果 | `float voltage = 3.3f;` |
| `String` | Arduino 文字資料 | `String state = "BOOT";` |

固定腳位與不應改變的設定使用 `constexpr`：

```cpp
constexpr uint8_t BUTTON_PIN = 13;
constexpr uint8_t LED_PIN = 2;
```

## 運算子

- 指派：`=`
- 相等比較：`==`
- 不相等：`!=`
- 大於／小於：`>`、`<`
- 邏輯 AND／OR／NOT：`&&`、`||`、`!`
- 加減乘除與餘數：`+`、`-`、`*`、`/`、`%`

不要把 `=` 和 `==` 混用：

```cpp
if (buttonPressed == true) {
  digitalWrite(LED_PIN, HIGH);
}
```

也可以寫成：

```cpp
if (buttonPressed) {
  digitalWrite(LED_PIN, HIGH);
}
```

## 整數、浮點與文字

`5 / 2` 使用整數運算時結果是 `2`；需要小數時，至少一個運算元要是浮點數：

```cpp
float half = 5.0f / 2.0f;
```

Arduino 的 `String` 是 C++ 物件，方便組合 OLED 訊息，但不應在長時間執行的微控制器程式中無限量拼接。第一篇還沒有文字顯示裝置，第二篇會將文字狀態實際顯示在 OLED。

## 明確型態轉換

當運算需要較大的中間範圍時，可使用 `static_cast`：

```cpp
uint8_t limited =
    (static_cast<uint16_t>(brightness) * 96) / 255;
```

這裡先將 8 位元數值轉成 16 位元，避免乘法的中間結果超出 0～255。

## 有效範圍

PWM 解析度為 8 位元時，有效 duty 範圍是 0～255。寫入前要確保變數不會超出範圍；未來讀取感測器時，也要先判斷資料是否有效，不能把錯誤值當成真實數值。

## 練習

將 `02_led` 的等待時間改成具名常數，再建立 `bool` 變數表示目前 LED 是否開啟。
