# 3. Arduino C 程式結構

Arduino Sketch 至少包含 `setup()` 與 `loop()`。

## `setup()`

ESP32 每次上電或重啟後執行一次，適合設定 GPIO、初始化模組及建立安全的啟動狀態。

```cpp
void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
}
```

先設定輸出為關閉狀態，可避免開機時出現非預期動作。

## `loop()`

`setup()` 完成後，ESP32 會重複執行 `loop()`。

```cpp
void loop() {
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
}
```

## 常數與註解

使用有意義的名稱取代散落在程式中的腳位數字：

```cpp
constexpr uint8_t LED_PIN = 2;
```

註解說明「為什麼這樣做」，不要只把下一行程式翻譯成中文。正式範例使用 UTF-8，避免沿用舊教材的 Big5 亂碼。

## 可見輸出原則

第一篇使用 LED 等硬體動作直接顯示程式是否執行，不留下只能從 Serial 看到的訊息。第二篇建立 OLED 後，所有文字狀態、錯誤與數值都改由 OLED 優先呈現。

## 練習

修改 `02_led` 的亮燈與熄燈時間，觀察哪一段程式控制哪一個狀態；再將腳位數字改成具名常數。
