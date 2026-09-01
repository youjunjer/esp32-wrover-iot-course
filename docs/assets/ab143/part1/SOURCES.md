# AB143 第一篇圖像來源

本目錄只保存實際用於第一篇、並已核對目前教材內容的單張圖片。原始 DOCX、PDF、出版社頁面、舊工具畫面與不相容接線不放入 Repository。

## 使用與授權邊界

圖片源自 AB143 課本工作檔，並依教材負責人的指示用於本 Repository 的教學說明。第三方不得因圖片出現在 GitHub，逕自推定已取得重製或再散布授權。

## 來源清單

| 圖檔 | 課本來源 | 原始媒體／頁面 | 處理方式 | SHA-256 |
|---|---|---|---|---|
| `digital-high-low.png` | AB143 第四版 2026，第 3 章 | `word/media/image9.png` | 原樣取出 | `6c496493b23f0a84813abc4838185c355c99c97485769967e93194d0c533dc35` |
| `onboard-led-location.jpg` | AB143 第四版 2026，第 3 章；原書頁 P30 | `word/media/image13.jpeg` | 原樣取出 | `6ab511d2beb394fb81de63fc42d237f7690e31bed5962c67c715593470f85c04` |
| `breadboard-connections.jpg` | AB143 第四版 2026，第 3 章；原書頁 P31 | `word/media/image15.jpeg` | 原樣取出 | `217932839fa6bb0ea3573ac3e8c529ebaf446d841a1eb44506e22f8cf16b6ff3` |
| `led-polarity.png` | AB143 第四版 2026，第 3 章；原書頁 P32 | `word/media/image17.png` | 原樣取出 | `5a5dd711d9bd89c1bc734fc8d832549c1897bd74341b614bd9a69b5a1712be0e` |
| `pwm-waveform.png` | AB143 三版，原書頁 P46／PDF 第 52 頁 | PDF 向量圖 | 180 dpi 渲染後只裁圖表範圍 | `ee7afff705353edb82df7fb0e762f247436d5da42b8ec97f37f110aeb4bbdf88` |
| `rgb-led-pinout.jpg` | AB143 第四版 2026，第 4 章；原書頁 P53 | `word/media/image22.jpeg` | 原樣取出 | `454ac646c776c5bc646c0a7400e66461dbd4a206040859e0746c9d7e44a5750d` |

## 未納入的圖片

- Arduino IDE、Serial Monitor、USB 驅動與 Library Manager 舊畫面：與 CLI 主流程或目前版本不一致。
- GPIO 2 的舊 PWM 接線圖：目前範例已改為 GPIO 15。
- 紅綠燈接線圖：圖中板型不是本課程的 Wrover 相容相機板，即使 LED 腳位相同也不直接沿用。
- 舊版 PWM 亮度曲線：橫軸把工作週期數值寫成輸出電壓，容易誤解為 HIGH 電壓隨 duty 改變。
- MQ-2 與 RGB 混合接線圖：感測器已移至 OLED 建立後的第二篇，RGB 腳位也與目前範例不同。
- NodeMCU-32S 名稱腳位圖與其他板型畫面：不能當成通用 Wrover 腳位圖。
- 人物照、新聞照片、商品照、網站色碼表、出版社標誌與來源不明素材。
- AB143 沒有 GPIO 13 外接按鈕圖，因此不以 BOOT 鍵圖片替代。
