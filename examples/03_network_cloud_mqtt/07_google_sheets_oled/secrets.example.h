#pragma once

// Copy to secrets.h and copy the EXISTING course GAS values from the teacher's
// original example. Do not deploy a new GAS for this lesson.
// Never commit secrets.h or show these values on the OLED.
inline constexpr char WIFI_SSID[] = "REPLACE_WITH_WIFI_SSID";
inline constexpr char WIFI_PASSWORD[] = "REPLACE_WITH_WIFI_PASSWORD";
inline constexpr char GOOGLE_SHEETS_GAS_URL[] =
    "https://script.google.com/macros/s/REPLACE_WITH_EXISTING_DEPLOYMENT_ID/exec";
inline constexpr char GOOGLE_SHEET_ID[] = "REPLACE_WITH_GOOGLE_SHEET_ID";
inline constexpr char GOOGLE_SHEET_TAG[] = "REPLACE_WITH_WORKSHEET_NAME";
