# 03 OLED 可視化診斷

本範例示範後續課程共用的狀態畫面，依序切換 `BOOT`、`READY`、`NO DATA`、`TIMEOUT`、`STALE` 與 `ERR`。

所有畫面都顯示 `DEMO STATUS`，代表這些是版型與錯誤碼教學，不是實際感測器或網路結果。後續範例必須用真實狀態替換示範資料，不能把固定字串當成成功證明。

## 可見驗收

- 每個狀態約顯示 3 秒。
- 畫面同時包含模組、狀態與細節。
- `NO DATA`、`TIMEOUT`、`STALE` 與 `ERR` 不會被空白畫面取代。
- OLED 初始化失敗仍以 GPIO 2 閃爍碼表示。

完整規則見 [`docs/part2/03_oled_diagnostics.md`](../../../docs/part2/03_oled_diagnostics.md)與 [`docs/oled-status-standard.md`](../../../docs/oled-status-standard.md)。
