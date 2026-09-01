# AB143 第二篇圖像來源

本目錄只保存第二篇實際引用、並已核對不會誤導目前腳位與工具流程的單張課本圖。原始 DOCX、PDF、出版社整頁、舊 IDE 畫面及不相容接線不放入 Repository。

## 使用與授權邊界

圖片源自 AB143 課本工作檔，並依教材負責人的指示用於本 Repository 的教學說明。第三方不得因圖片出現在 GitHub，逕自推定已取得重製或再散布授權。

## 來源清單

| 圖檔 | 課本來源 | 原始媒體／頁面／尺寸 | 交叉核對 | 處理方式 | SHA-256 |
|---|---|---|---|---|---|
| `pir-module-front-rear.jpg` | `四版2026/第3章_課本PDF.docx` | `word/media/image20.jpeg`；原書 P35；514×238 | `三版：課本.pdf`，PDF p41 | 原樣取出並語意化改名 | `a32bb6947e3c15c2b9d7b328c3a53358b7d8030c15e921fb5cec76b21eda887b` |
| `pir-pins-adjustments.jpg` | `四版2026/第3章_課本PDF.docx` | `word/media/image22.jpeg`；原書 P35；285×282 | `三版：課本.pdf`，PDF p41 | 原樣取出並語意化改名 | `4ef2797dfc805cbd9626c7456cd28e18e815437c6dd9015d82d8a74f6f836ab5` |
| `light-sensor-ao-do.png` | `四版2026/第4章_課本PDF.docx` | `word/media/image6.png`；原書 P43；768×250 | `三版：課本.pdf`，PDF p49 | 原樣取出並語意化改名 | `e3e81de12d502c7c5b06a3778cb49240a0c5b0a5eaaf3e4a1197712afc784d13` |

## 未納入的舊圖

- 第 3 章 `image25.jpeg`：舊 WROOM 板、GPIO 17／16、5V 與 ISD1820 混合接線，不符合目前 GPIO 14、OLED-first 與單一模組測試。
- 第 4 章 `image8.jpeg`：舊 WROOM 板、GPIO 36、LED 與無 OLED 接線，不符合目前 GPIO 33 與可視化診斷流程。
- 舊 Serial Monitor／IDE 畫面：不是 Arduino CLI 主流程，也不能取代 OLED 可見狀態。
