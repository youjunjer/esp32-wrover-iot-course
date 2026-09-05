# 第二篇教學示意圖來源

本目錄保存依目前課程腳位與程式版型製作的 Repository 原生 SVG。這些圖用來說明接線與預期畫面，不是 CLI 截圖、實體 OLED 照片或感測結果。

第 7～12 章新增圖由 Codex 以純 SVG XML 原生繪製，只使用文字與幾何圖元，未裁切或嵌入課本頁面、產品照片或第三方點陣素材。驗證時使用 Sharp 0.35.4 依 SVG 宣告尺寸轉成 PNG，再逐張檢查構圖、文字與警示標籤；SHA-256 使用 `shasum -a 256` 計算。

| 圖檔 | 依據 | 證據邊界 | SHA-256 |
|---|---|---|---|
| `oled-wrover-wiring.svg` | `docs/hardware-pin-modes.md`、能源專案 OLED GPIO 21／22、3.3V 教學基準 | 一般模式接線示意；未證明實體已接線 | `c3cb8006dba0b7ad3fa22a537664b817fb49736abc739875f0c187b045f917e6` |
| `oled-self-test-expected.svg` | `01_oled_self_test.ino` 的實際文字與版型 | 預期畫面；位址與秒數不是實機讀值 | `c7c38ce214ace7de0702a9e168fb084bb55f4ac6554980c012f5d1f3353d3bdd` |
| `oled-basics-expected.svg` | `02_oled_basics.ino` 的三頁文字與圖形 | 預期畫面；未證明實體更新速度或方向 | `31903fedbbfc68078799335acb0e4e6a224a424fba626e8b498d9ea30192f0e4` |
| `oled-diagnostics-demo.svg` | `03_oled_diagnostics.ino` 的 `DEMO_STATES` | DEMO 版型；不是感測器或網路結果 | `b3c02040a7c2a06d03c097495a4da2f0080eebb7c22a891a13c0f45935202330` |
| `sensor-input-wiring.svg` | GPIO 14 數位輸入、GPIO 33 ADC1、OLED GPIO 21／22、共地與 3.3V 輸入上限 | 一般模式接線示意；未證明實體接線 | `486eff2c2f04f4628b0f465e3e4d0cd36ae381f29fb4cc1c223c7f151bf56e58` |
| `sensor-input-expected.svg` | `04_sensor_input_basics.ino` 的 `D14`、`A33` 與浮接提醒 | 預期畫面；`2048` 不是實測資料 | `eed900b65f3fa948558428b58a833806b4f1ed1d25c0f918db79294674e7b684` |
| `pir-wrover-wiring.svg` | GPIO 14 PIR、課程模組 5V、OLED GPIO 21／22 與共地規則 | 指定一般模式接線示意；未確認模組批次或輸出電壓 | `a68d2bcac14cacc0712778a8f5a858dbfff565de02acc09114cdf0f2b712b74b` |
| `pir-expected.svg` | `05_pir_oled.ino` 的 60 秒暖機、150 ms 穩定判定與事件版型 | 預期畫面；事件數與 RAW 不是實測資料 | `a738eaf3c127b3224b784db10df8d1a1e45ff46d3e4c83494189cdd45de17b2b` |
| `light-wrover-wiring.svg` | 光敏模組 3.3V、AO GPIO 33、DO 不接與 OLED GPIO 21／22 | 一般模式接線示意；未證明實體 ADC 讀值 | `e5780255fd441d7c4d3ad209a257bea72bd5c426261289f519b85f6bac1480d0` |
| `light-expected.svg` | `06_light_oled.ino` 的 `UNCAL` 預設與校正後版型；校正說明置於 OLED 框外 | 預期畫面；原始值、門檻與明暗分級不是實測資料 | `348e362829272d06090d663364ae2cbf4a90eb2897c212c76671dad54964f93e` |
| `mq2-wrover-wiring.svg` | OLED GPIO 21／22；MQ-2 5V、AO→10kΩ→GPIO33、GPIO33→12kΩ→GND，DO 不接 | Repository 原生接線示意；名目分壓約 2.73V，仍須量測模組 AO 並完成實體驗證 | `33451a90ba24eb51e8278fd94979f453f8df8ca7b391d9eafb97eab2b32c8463` |
| `mq2-expected.svg` | OLED 的暖機、未校正 RAW／MIN／MAX 與相對警示 `REL WARN`；本機校正情境只置於框外圖說 | Repository 原生預期畫面；OLED 固定標示 `NO PPM`，圖內數值及警示均非實測安全濃度 | `f900363b9178fb83bdeb1b72db6f5d587c417f1967a7e5d67613909b99698136` |
| `ultrasonic-wrover-wiring.svg` | OLED GPIO 21／22；HC-SR04 TRIG GPIO25；ECHO→1% 10kΩ→GPIO14、GPIO14→1% 15kΩ→GND | Repository 原生接線示意；5.0V 名目分壓約 3.00V，仍須量測或改用合適位準轉換器 | `276bd0f4644bbabf4e0b0e0751db609a5ae3f11eb4ff63d1a310949016999db9` |
| `ultrasonic-expected.svg` | OLED 的 `OK`＋`DIST`／`ECHO`、`TIMEOUT`＋`NO VALID DIST`／`INVALID`、`RANGE ERR`＋`VALUE`／`OUT OF 2-400` | Repository 原生預期畫面；距離、脈衝時間與計數均非實測 | `58c708876471e8b5f6624fed5205279610f9597c2a991a0c55b8780402576121` |
| `parking-radar-wiring.svg` | 超音波 ECHO 使用 1% 10kΩ／15kΩ 安全分壓，加蜂鳴器模組 SIG GPIO13；OLED GPIO 21／22 | Repository 原生接線示意；5.0V 名目分壓約 3.00V 且仍須量測；GPIO13 只驅動模組訊號 | `32bbf9df7b2389fa4ff8c4537343c56bd87bb4a480838df54f4f22c0f98dbdc8` |
| `parking-radar-expected.svg` | OLED 的 `SAFE`＋`SILENT`、`WARNING`＋`TONE 1000 Hz`、`TIMEOUT`＋`NO VALID DIST`／`SAFE OFF` | Repository 原生預期畫面；距離為示意值，連續音輸出仍須以指定蜂鳴器模組實測 | `ed151e2a7d0db720c8b41ceddf4f09eab6fcd024665b17ccedd4bf85543d37bb` |
| `dht11-wrover-wiring.svg` | OLED GPIO 21／22；DHT11 DATA GPIO14、3.3V 與共地；裸感測器上拉提醒 | Repository 原生接線示意；不同模組腳位次序與板載上拉須在通電前確認 | `3c90fa24e82d5bab863984fd18c7ef4d1432a1b87bed938ce68eb125101f9fda` |
| `dht11-expected.svg` | OLED 的 `WAIT`／`FIRST READ 2s`、`OK`＋溫濕度／`AGE`／`ERR`、`NO DATA`＋`CHECK VCC/DATA/GND` | Repository 原生預期畫面；溫度、濕度、時間與錯誤計數均非實測 | `0cc0179ee70106cdc1a1a9c746f5313c090a58d48b173ef921e21a790456053c` |
| `lcd1602-wrover-wiring.svg` | 1602 LCD SDA GPIO21／SCL GPIO22；3.3V 相容背板或雙向 I²C 位準轉換 | Repository 原生接線示意；未確認指定背板的工作電壓、I²C 上拉或位址 | `9e7ec8a6921c1d8056e802df918f9d065f5707949e64de267e48e7a5706de92b` |
| `lcd1602-expected.svg` | OLED 的 `LCD SCAN`／`LCD NO ACK R3s`；1602 的 `T:24.0°C H:55%`／`DHT OK AGE 0s` 版型 | Repository 原生預期畫面；`LCD ACK` 只代表 PCF8574 位址回應，不代表 LCD 完全正常；位址與讀值均非實測 | `8175ae8a2bcab21fb1e3f849ac71cb775eca023f1860633d201a1ec39a7548a9` |
| `multisensor-wrover-wiring.svg` | DHT11 DATA GPIO14、光敏 AO GPIO33、OLED 21／22，選配 3.3V 安全 1602 共用 I²C | Repository 原生整合接線示意；須確認 OLED／LCD 位址及 I²C 上拉，不代表實體已同時運作 | `b720f688dc75d0a826c82db0cd4ab6e67e2cff409d9696f38fb49f3fd6a79b45` |
| `multisensor-expected.svg` | OLED 實際四頁：`SUMMARY`、`DHT11 GPIO14`、`LIGHT GPIO33`、`SYSTEM`；每頁 footer 顯示 `LCD ACK 0x27` | Repository 原生預期畫面；ACK 只代表 PCF8574 位址回應，位址、讀值、AGE 與 UP 均非實測 | `fc5e1815cb572f0215acd0f2427e47adc232aadda4f24291ac4156d2732bc26c` |

所有 SVG 已以檔案宣告尺寸渲染檢查，確認文字、圖形與警示標籤沒有裁切或重疊。真實 GitHub Actions 編譯畫面另存於 `../captures/`；不得以本目錄的示意圖代替實際 CI 或硬體證據。
