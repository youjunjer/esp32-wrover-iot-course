# 第二篇教學示意圖來源

本目錄保存依目前課程腳位與程式版型製作的 Repository 原生 SVG。這些圖用來說明接線與預期畫面，不是 CLI 截圖、實體 OLED 照片或感測結果。

| 圖檔 | 依據 | 證據邊界 | SHA-256 |
|---|---|---|---|
| `oled-wrover-wiring.svg` | `docs/hardware-pin-modes.md`、能源專案 OLED GPIO 21／22、3.3V 教學基準 | 一般模式接線示意；未證明實體已接線 | `c3cb8006dba0b7ad3fa22a537664b817fb49736abc739875f0c187b045f917e6` |
| `oled-self-test-expected.svg` | `01_oled_self_test.ino` 的實際文字與版型 | 預期畫面；位址與秒數不是實機讀值 | `c7c38ce214ace7de0702a9e168fb084bb55f4ac6554980c012f5d1f3353d3bdd` |
| `oled-basics-expected.svg` | `02_oled_basics.ino` 的三頁文字與圖形 | 預期畫面；未證明實體更新速度或方向 | `31903fedbbfc68078799335acb0e4e6a224a424fba626e8b498d9ea30192f0e4` |
| `oled-diagnostics-demo.svg` | `03_oled_diagnostics.ino` 的 `DEMO_STATES` | DEMO 版型；不是感測器或網路結果 | `b3c02040a7c2a06d03c097495a4da2f0080eebb7c22a891a13c0f45935202330` |

所有 SVG 已以 100% 尺寸渲染檢查，確認文字、圖形與警示標籤沒有裁切或重疊。真實 GitHub Actions／CLI 畫面要等本次 CI 成功後另行加入 `../captures/`；目前尚未取得，不得以本目錄的示意圖代替。
