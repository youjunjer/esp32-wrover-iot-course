# 第二篇真實操作畫面來源

本目錄只保存由真實 CLI、GitHub Actions 或實體硬體工作階段取得的畫面。每張圖都必須標明它能證明的範圍；CI 畫面不能當成燒錄、接線或 OLED 實機成功證據。

| 圖檔 | 來源與環境 | 擷取方式 | 證據邊界 | SHA-256 |
|---|---|---|---|---|
| `github-actions-part2-success.png` | GitHub Actions Run [33474648376](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33474648376)，Commit `9934db9`；Arduino CLI 1.5.1、ESP32 Core 3.3.11、Wrover FQBN、鎖定 OLED 函式庫 | 2026-09-01（Asia/Taipei）以未登入的乾淨 Chrome 直接擷取公開 Run Summary，1440×1000；未裁切、未重打輸出 | 證明 Workflow 與全部 11 個 Sketch 編譯成功；不證明燒錄、接線、OLED 或感測器實機成功 | `5362c06005e6beea380213d25ae6b2d6bbdef9f64cd48b43051f2b223f2d63b7` |
| `github-actions-part2-sensors-success.png` | GitHub Actions Run [33478348736](https://github.com/youjunjer/esp32-wrover-iot-course/actions/runs/33478348736)，Commit `dad0084`；Arduino CLI 1.5.1、ESP32 Core 3.3.11、`esp32:esp32:esp32wrover`、鎖定 OLED 函式庫 | 2026-09-01（Asia/Taipei）以未登入的 Codex 內建瀏覽器直接擷取公開 Run Summary，1280×720；只將擷取格式轉為 PNG，未裁切、未重打輸出 | 畫面直接證明 Workflow 與 `compile` 工作成功；Run log 配合 14 筆 manifest 證明全部 14 個 Sketch 編譯成功。不證明燒錄、接線、OLED、PIR 或 ADC 實機成功 | `6e9aa27beefb0c881936c042af018c06ba903cf7d155dfa1b71cd89bfdf9739b` |

目前尚無實體 OLED 與完整接線照片。完成指定課程板測試後，應另外加入板型、驗證人、拍攝日期、Sketch Commit 與實體驗收結果，不覆蓋上述 CI 截圖。
