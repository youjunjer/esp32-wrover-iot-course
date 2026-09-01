# 02 OLED 基礎顯示

本範例依序顯示文字、矩形／圓形／三角形，以及持續更新的進度條，讓學生確認文字大小、座標、圖形與 `clearDisplay()`／`display()` 的更新流程。

接線與初始化方式與 `01_oled_self_test` 相同。先讓第一個範例顯示 `INIT OK`，再燒錄本範例。

## 可見驗收

1. `PAGE 1 TEXT` 顯示兩種文字大小。
2. `PAGE 2 SHAPES` 顯示矩形、實心圓與三角形。
3. `PAGE 3 UPDATE` 的百分比與進度條持續變化。
4. 三個頁面約每 3 秒切換，畫面不殘留上一頁內容。

若 OLED 完全沒有畫面，仍依 GPIO 2 的 2 下／3 下閃爍碼排查，不使用 Serial 作為唯一線索。

完整教學見 [`docs/part2/02_oled_basics.md`](../../../docs/part2/02_oled_basics.md)。
