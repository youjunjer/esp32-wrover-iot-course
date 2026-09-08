#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <esp_system.h>

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int STATUS_LED_PIN = 2;
constexpr int CONTROL_LED_PIN = 13;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ROTATION = 2;
constexpr uint8_t ERROR_NO_OLED = 2;
constexpr uint8_t ERROR_DISPLAY_INIT = 3;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250UL;
constexpr unsigned long EVENT_HOLD_MS = 2500UL;
constexpr unsigned long RETRY_DELAYS_MS[] = {3000UL, 6000UL, 12000UL, 30000UL};

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WebServer server(80);

enum class AppState { CONFIG_ERR, WIFI_CONNECT, WIFI_WAIT, RETRY_WAIT, WEB_READY };

AppState appState = AppState::CONFIG_ERR;
bool oledReady = false;
bool serverStarted = false;
bool mdnsStarted = false;
bool outputOn = false;
bool lastEventIsError = false;
uint8_t errorCode = 0;
uint8_t retryIndex = 0;
unsigned long stateStartedMillis = 0;
unsigned long retryAtMillis = 0;
unsigned long lastDisplayMillis = 0;
unsigned long lastEventMillis = 0;
int lastHttpCode = 0;
char csrfToken[17] = {};
char lastEvent[17] = "SAFE OFF";
char lastNetworkFailure[17] = "WIFI TIMEOUT";

const char *COLLECTED_HEADERS[] = {"X-CSRF-Token"};

uint8_t findOledAddress() {
  constexpr uint8_t CANDIDATE_ADDRESSES[] = {0x3C, 0x3D};
  for (uint8_t address : CANDIDATE_ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) return address;
  }
  return 0;
}

void blinkErrorCode(uint8_t code) {
  for (uint8_t pulse = 0; pulse < code; ++pulse) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(160);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(160);
  }
  delay(1000);
}

bool isPlaceholder(const char *value) {
  return value == nullptr || value[0] == '\0' || strncmp(value, "REPLACE_", 8) == 0;
}

bool validHostname(const char *value) {
  if (value == nullptr) return false;
  const size_t length = strlen(value);
  if (length == 0 || length > 15 || value[0] == '-' || value[length - 1] == '-') return false;
  for (size_t index = 0; index < length; ++index) {
    const char c = value[index];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
  }
  return true;
}

bool configurationReady() {
  return !USING_EXAMPLE_SECRETS && !isPlaceholder(WIFI_SSID) &&
         !isPlaceholder(WIFI_PASSWORD) && !isPlaceholder(DEVICE_HOSTNAME) &&
         validHostname(DEVICE_HOSTNAME) &&
         !isPlaceholder(WEB_USERNAME) && !isPlaceholder(WEB_PASSWORD) &&
         strlen(WEB_PASSWORD) >= 12;
}

void setOutput(bool on) {
  outputOn = on;
  digitalWrite(CONTROL_LED_PIN, on ? HIGH : LOW);
}

void setEvent(const char *message, int httpCode) {
  snprintf(lastEvent, sizeof(lastEvent), "%s", message);
  lastHttpCode = httpCode;
  lastEventIsError = httpCode >= 400;
  lastEventMillis = millis();
}

void drawConfigError() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("WEB SERVER");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print("CONFIG ERR");
  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print("COPY secrets.h");
  display.setCursor(0, 53);
  display.print("SAFE OFF");
  display.display();
}

void drawRuntime(unsigned long now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("WEB GPIO13");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  if (appState == AppState::WIFI_CONNECT || appState == AppState::WIFI_WAIT) {
    display.print("WIFI CONNECT");
    display.setCursor(0, 28);
    const unsigned long elapsed = now - stateStartedMillis;
    const unsigned long left = elapsed >= WIFI_TIMEOUT_MS
                                   ? 0
                                   : (WIFI_TIMEOUT_MS - elapsed + 999UL) / 1000UL;
    display.print("TIME LEFT ");
    display.print(left);
    display.print("s");
    display.setCursor(0, 51);
    display.print("SAFE OFF");
  } else if (appState == AppState::RETRY_WAIT) {
    display.print(lastNetworkFailure);
    display.setCursor(0, 28);
    const long remaining = static_cast<long>(retryAtMillis - now);
    const unsigned long seconds = remaining <= 0 ? 0 : (remaining + 999L) / 1000L;
    display.print("RETRY ");
    display.print(seconds);
    display.print("s");
    display.setCursor(0, 51);
    display.print("SAFE OFF");
  } else {
    display.print("WEB READY");
    display.setCursor(0, 26);
    if (mdnsStarted) {
      display.print(DEVICE_HOSTNAME);
      display.print(".local");
    } else {
      display.print("MDNS NOT READY");
    }
    display.setCursor(0, 38);
    display.print(outputOn ? "OUTPUT ON" : "SAFE OFF");
    display.setCursor(0, 50);
    if (lastEventIsError || now - lastEventMillis < EVENT_HOLD_MS) {
      display.print(lastEvent);
      if (lastHttpCode > 0) {
        display.print(" ");
        display.print(lastHttpCode);
      }
    } else {
      display.print("RSSI ");
      display.print(WiFi.RSSI());
      display.print(" dBm");
    }
  }
  display.display();
}

void generateCsrfToken() {
  const uint32_t first = esp_random();
  const uint32_t second = esp_random();
  snprintf(csrfToken, sizeof(csrfToken), "%08lx%08lx",
           static_cast<unsigned long>(first), static_cast<unsigned long>(second));
}

String *checkDigestAuthentication(HTTPAuthMethod mode, String username,
                                  String extraParams[]) {
  (void)extraParams;
  if (mode != DIGEST_AUTH || !username.equals(WEB_USERNAME)) return nullptr;
  return new String(WEB_PASSWORD);
}

bool requireAuthentication(bool safeOffOnFailure = false) {
  if (server.authenticate(checkDigestAuthentication)) return true;
  if (safeOffOnFailure) setOutput(false);
  setEvent("AUTH ERR", 401);
  server.requestAuthentication(DIGEST_AUTH, "ESP32 Classroom", "Authentication required");
  return false;
}

void sendJson(int code, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

String buildHomePage() {
  String page;
  page.reserve(2600);
  page += F(R"HTML(<!doctype html><html lang="zh-Hant"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Classroom Control</title><style>
body{font-family:system-ui,sans-serif;max-width:38rem;margin:2rem auto;padding:0 1rem;background:#f8fafc;color:#0f172a}
main{background:#fff;border:1px solid #cbd5e1;border-radius:1rem;padding:1.4rem;box-shadow:0 .3rem 1rem #cbd5e1}
button{font-size:1.1rem;padding:.8rem 1.4rem;margin:.4rem;border:0;border-radius:.6rem;color:#fff;background:#0369a1}
button.off{background:#475569}code{background:#e2e8f0;padding:.15rem .35rem;border-radius:.25rem}
#result{min-height:1.5rem;font-weight:700}.warning{color:#9a3412}</style></head><body><main>
<h1>ESP32 教室控制</h1><p>輸出腳位：<code>GPIO 13</code></p>
<button onclick="setOutput('on')">開啟</button><button class="off" onclick="setOutput('off')">安全關閉</button>
<p id="result">讀取狀態中…</p><p class="warning">僅限可信任、隔離的教室區網；禁止路由器轉發連接埠，也不可用於市電負載。</p>
<script>const csrf=')HTML");
  page += csrfToken;
  page += F(R"HTML(';
async function refresh(){const r=await fetch('/api/status',{cache:'no-store'});const d=await r.json();document.querySelector('#result').textContent='目前輸出：'+d.output;}
async function setOutput(state){const r=await fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':csrf},body:'state='+encodeURIComponent(state)});const d=await r.json();document.querySelector('#result').textContent=d.message;}
refresh().catch(()=>document.querySelector('#result').textContent='狀態讀取失敗');</script></main></body></html>)HTML");
  return page;
}

void handleHome() {
  if (!requireAuthentication()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", buildHomePage());
  setEvent("REQ GET", 200);
}

void handleStatus() {
  if (!requireAuthentication()) return;
  String body = F("{\"output\":\"");
  body += outputOn ? F("on") : F("off");
  body += F("\",\"uptime_ms\":");
  body += millis();
  body += '}';
  sendJson(200, body);
  setEvent("REQ STATUS", 200);
}

void handleLedPost() {
  if (!requireAuthentication(true)) return;
  if (!server.hasHeader("X-CSRF-Token") || server.header("X-CSRF-Token") != csrfToken) {
    setOutput(false);
    setEvent("CSRF ERR", 403);
    sendJson(403, F("{\"message\":\"CSRF validation failed; output is OFF\"}"));
    return;
  }
  if (server.args() != 1 || !server.hasArg("state") || server.argName(0) != "state") {
    setOutput(false);
    setEvent("CMD ERR", 400);
    sendJson(400, F("{\"message\":\"Invalid command; output is OFF\"}"));
    return;
  }

  const String requestedState = server.arg("state");
  if (requestedState == "on") {
    setOutput(true);
    setEvent("CMD OK ON", 200);
    sendJson(200, F("{\"message\":\"Output is ON\"}"));
  } else if (requestedState == "off") {
    setOutput(false);
    setEvent("CMD OK OFF", 200);
    sendJson(200, F("{\"message\":\"Output is OFF\"}"));
  } else {
    setOutput(false);
    setEvent("CMD ERR", 400);
    sendJson(400, F("{\"message\":\"Unknown state; output is OFF\"}"));
  }
}

void handleLedGet() {
  if (!requireAuthentication(true)) return;
  setOutput(false);
  setEvent("METHOD ERR", 405);
  server.sendHeader("Allow", "POST");
  sendJson(405, F("{\"message\":\"Use POST /api/led\"}"));
}

void handleNotFound() {
  const bool isControlPath = server.uri() == "/api/led";
  if (isControlPath) setOutput(false);
  if (!requireAuthentication()) return;
  if (isControlPath) {
    setEvent("METHOD ERR", 405);
    server.sendHeader("Allow", "POST");
    sendJson(405, F("{\"message\":\"Use POST /api/led; output is OFF\"}"));
    return;
  }
  setEvent("NOT FOUND", 404);
  sendJson(404, F("{\"message\":\"Not found\"}"));
}

void configureRoutes() {
  server.collectHeaders(COLLECTED_HEADERS, 1);
  server.on("/", HTTP_GET, handleHome);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/led", HTTP_POST, handleLedPost);
  server.on("/api/led", HTTP_GET, handleLedGet);
  server.onNotFound(handleNotFound);
}

void stopServer() {
  if (serverStarted) {
    server.stop();
    serverStarted = false;
  }
  if (mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }
}

void scheduleRetry(unsigned long now, const char *reason) {
  stopServer();
  setOutput(false);
  WiFi.disconnect();
  snprintf(lastNetworkFailure, sizeof(lastNetworkFailure), "%s", reason);
  const uint8_t delayIndex = retryIndex < 4 ? retryIndex : 3;
  retryAtMillis = now + RETRY_DELAYS_MS[delayIndex];
  if (retryIndex < 3) ++retryIndex;
  appState = AppState::RETRY_WAIT;
  setEvent(reason, 0);
}

void beginWifi(unsigned long now) {
  stopServer();
  setOutput(false);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  stateStartedMillis = now;
  appState = AppState::WIFI_WAIT;
  setEvent("WIFI CONNECT", 0);
}

void startServer() {
  generateCsrfToken();
  mdnsStarted = MDNS.begin(DEVICE_HOSTNAME);
  if (mdnsStarted) MDNS.addService("http", "tcp", 80);
  server.begin();
  serverStarted = true;
  retryIndex = 0;
  appState = AppState::WEB_READY;
  setEvent(mdnsStarted ? "WEB START" : "MDNS ERR", mdnsStarted ? 200 : 0);
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(CONTROL_LED_PIN, OUTPUT);
  setOutput(false);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setTimeOut(50);
  delay(50);
  const uint8_t address = findOledAddress();
  if (address == 0) {
    errorCode = ERROR_NO_OLED;
    return;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    errorCode = ERROR_DISPLAY_INIT;
    return;
  }

  display.setTextWrap(false);
  display.setRotation(OLED_ROTATION);
  oledReady = true;
  configureRoutes();

  if (!configurationReady()) {
    appState = AppState::CONFIG_ERR;
    drawConfigError();
    return;
  }

  appState = AppState::WIFI_CONNECT;
  beginWifi(millis());
  drawRuntime(millis());
}

void loop() {
  if (!oledReady) {
    blinkErrorCode(errorCode);
    return;
  }
  if (appState == AppState::CONFIG_ERR) {
    if (millis() - lastDisplayMillis >= 1000UL) {
      lastDisplayMillis = millis();
      drawConfigError();
    }
    return;
  }

  const unsigned long now = millis();
  if (appState == AppState::WIFI_WAIT) {
    if (WiFi.status() == WL_CONNECTED) {
      startServer();
    } else if (now - stateStartedMillis >= WIFI_TIMEOUT_MS) {
      scheduleRetry(now, "WIFI TIMEOUT");
    }
  } else if (appState == AppState::RETRY_WAIT) {
    if (static_cast<long>(now - retryAtMillis) >= 0) beginWifi(now);
  } else if (appState == AppState::WEB_READY) {
    if (WiFi.status() != WL_CONNECTED) {
      scheduleRetry(now, "NET LOST");
    } else {
      server.handleClient();
    }
  }

  if (now - lastDisplayMillis >= DISPLAY_INTERVAL_MS) {
    lastDisplayMillis = now;
    drawRuntime(now);
  }
}
