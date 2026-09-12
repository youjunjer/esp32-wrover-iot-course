#include <Arduino.h>
#include <NetworkClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include <mbedtls/base64.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool oledReady = false;
uint8_t oledError = 2;

void oledStatus(const char *title, const char *state, const char *detail = "", const char *footer = "") {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0); display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 17); display.print(state);
  display.setCursor(0, 34); display.print(detail);
  display.setCursor(0, 51); display.print(footer);
  display.display();
}

bool beginOled(int sda, int scl) {
  pinMode(2, OUTPUT); digitalWrite(2, LOW);
  if (!Wire.begin(sda, scl)) { oledError = 3; return false; }
  Wire.setTimeOut(50);
  delay(50);
  for (uint8_t address : {0x3C, 0x3D}) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    if (!display.begin(SSD1306_SWITCHCAPVCC, address, true, false)) {
      oledError = 3; return false;
    }
    display.setRotation(2); display.setTextWrap(false);
    oledReady = true;
    oledStatus("PART 5", "BOOT", "OLED OK");
    return true;
  }
  return false;
}

void oledFallback() {
  for (uint8_t i = 0; i < oledError; ++i) {
    digitalWrite(2, HIGH); delay(160);
    digitalWrite(2, LOW); delay(160);
  }
  delay(1000);
}

#define CAMERA_MODEL_AI_THINKER
#include "../support/course_logic.h"
#include "../support/camera_driver.h"
#if __has_include("camera_config.h")
#include "camera_config.h"
#else
#include "camera_config.example.h"
#endif
bool cameraReady = false;

bool cameraBoot() {
  pinMode(2, OUTPUT); digitalWrite(2, LOW);
  if (!Course::cameraOledPins(CAMERA_OLED_SDA, CAMERA_OLED_SCL)) {
    // No valid I2C bus can be initialized. Four pulses; never fall back to 21/22.
    oledError = 4; return false;
  }
  if (!beginOled(CAMERA_OLED_SDA, CAMERA_OLED_SCL)) return false;
  if (!OLED_PINS_VERIFIED || !ENABLE_CAMERA_TEST) {
    oledStatus("CAMERA PREFLIGHT", "CAM LOCKED", "CHECK OLED / BOARD", "READ CHAPTER 6");
    return false;
  }
  if (!psramFound()) { oledStatus("CAMERA PREFLIGHT", "PSRAM ERR"); return false; }
  oledStatus("CAMERA PREFLIGHT", "CAM INIT", "COEXISTENCE TEST");
  const esp_err_t error = initializeCamera();
  if (error != ESP_OK) {
    char detail[22]; snprintf(detail, sizeof(detail), "CODE 0x%x", static_cast<unsigned>(error));
    oledStatus("CAMERA PREFLIGHT", "CAM INIT ERR", detail); return false;
  }
  cameraReady = true; return true;
}

#if __has_include("secrets.h")
#include "secrets.h"
constexpr bool USING_EXAMPLE_SECRETS = false;
#else
#include "secrets.example.h"
constexpr bool USING_EXAMPLE_SECRETS = true;
#endif
bool placeholder(const char *value) {
  return !value || !*value || strstr(value, "REPLACE_WITH_");
}

WebServer server(80);
bool configured = false, wifiAttempt = false, serving = false;
uint32_t wifiStart = 0, nextWifi = 0, retries = 0, lastWiFiPaint = 0;
char csrf[33] = {};

bool authenticated() {
  if (server.authenticate(CAMERA_WEB_USER, CAMERA_WEB_PASSWORD)) return true;
  oledStatus("CAMERA WEB", "AUTH REQUIRED", "TRUSTED LAN ONLY");
  server.requestAuthentication(DIGEST_AUTH, "WroverCamera"); return false;
}
bool writeBounded(NetworkClient &client, const uint8_t *data, size_t size, uint32_t deadline) {
  size_t sent = 0;
  while (sent < size) {
    if (!client.connected() || static_cast<int32_t>(millis() - deadline) >= 0) return false;
    const size_t amount = client.write(data + sent, min(size - sent, static_cast<size_t>(1024)));
    if (!amount) return false;
    sent += amount; delay(1);
  }
  return true;
}
bool sendFrame(bool asBase64) {
  oledStatus("CAMERA WEB", "CAPTURE");
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) { server.send(503, "text/plain", "FRAME ERR"); oledStatus("CAMERA WEB", "FRAME ERR"); return false; }
  bool ok = frame->format == PIXFORMAT_JPEG && frame->len && frame->len <= Course::JPEG_LIMIT;
  if (!ok) {
    esp_camera_fb_return(frame); server.send(413, "text/plain", "FRAME SIZE/FORMAT ERR");
    oledStatus("CAMERA WEB", "FRAME SIZE ERR", "TRY QVGA"); return false;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(asBase64 ? 4 * ((frame->len + 2) / 3) : frame->len);
  server.send(200, asBase64 ? "text/plain" : "image/jpeg", "");
  NetworkClient &client = server.client(); client.setTimeout(2000);
  const uint32_t deadline = millis() + 10000;
  if (asBase64) {
    unsigned char encoded[513];
    for (size_t offset = 0; offset < frame->len && ok; offset += Course::CHUNK_BYTES) {
      size_t written = 0;
      ok = mbedtls_base64_encode(encoded, sizeof(encoded), &written, frame->buf + offset,
             min(Course::CHUNK_BYTES, frame->len - offset)) == 0;
      if (ok) ok = writeBounded(client, encoded, written, deadline);
    }
  } else ok = writeBounded(client, frame->buf, frame->len, deadline);
  esp_camera_fb_return(frame);
  if (!ok) client.stop();
  oledStatus("CAMERA WEB", ok ? "SEND OK" : "SEND TIMEOUT", asBase64 ? "BASE64 / NOT ENCRYPT" : "JPEG");
  return ok;
}
void streamFrames() {
  if (!authenticated()) return;
  oledStatus("CAMERA WEB", "STREAM", "MAX 10s / 20 FRAMES");
  NetworkClient &client = server.client(); client.setTimeout(2000);
  const uint32_t deadline = millis() + 10000;
  const char *header = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
  bool ok = writeBounded(client, reinterpret_cast<const uint8_t *>(header), strlen(header), deadline);
  for (int count = 0; ok && count < 20 && static_cast<int32_t>(millis() - deadline) < 0; ++count) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) { ok = false; break; }
    if (frame->format != PIXFORMAT_JPEG || !frame->len || frame->len > Course::JPEG_LIMIT) {
      esp_camera_fb_return(frame); ok = false; break;
    }
    char boundary[100];
    snprintf(boundary, sizeof(boundary), "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", static_cast<unsigned>(frame->len));
    ok = writeBounded(client, reinterpret_cast<const uint8_t *>(boundary), strlen(boundary), deadline) &&
         writeBounded(client, frame->buf, frame->len, deadline) &&
         writeBounded(client, reinterpret_cast<const uint8_t *>("\r\n"), 2, deadline);
    esp_camera_fb_return(frame);
    delay(200);
  }
  if (ok) ok = writeBounded(client, reinterpret_cast<const uint8_t *>("--frame--\r\n"), 11, deadline);
  client.stop();
  oledStatus("CAMERA WEB", ok ? "STREAM DONE" : "STREAM ERR", "CLICK TO START AGAIN");
}
void configureRoutes() {
  server.on("/", HTTP_GET, []() {
    if (!authenticated()) return;
    server.sendHeader("Cache-Control", "no-store");
    String page = "<!doctype html><html lang='zh-Hant'><meta charset='utf-8'><meta name='viewport' content='width=device-width'><title>Wrover camera</title><h1>Wrover 相機實驗</h1><p>受信任區網。影像未加密；不自動保存。</p><p><a href='/capture'>拍照 JPEG</a> | <a href='/base64'>Base64</a> | <a href='/stream'>短串流（最多 10 秒）</a></p><form method='post' action='/resolution'><input type='hidden' name='csrf' value='";
    page += csrf;
    page += "'><select name='size'><option value='qvga'>QVGA 320×240</option><option value='vga'>VGA 640×480</option></select><button>設定解析度</button></form><p>每次拍攝前確認現場同意。完成後關閉頁面並停止實驗。</p></html>";
    server.send(200, "text/html; charset=utf-8", page);
    oledStatus("CAMERA WEB", "PAGE SENT", "wrover-camera.local");
  });
  server.on("/capture", HTTP_GET, []() { if (authenticated()) sendFrame(false); });
  server.on("/base64", HTTP_GET, []() { if (authenticated()) sendFrame(true); });
  server.on("/stream", HTTP_GET, streamFrames);
  server.on("/resolution", HTTP_POST, []() {
    if (!authenticated()) return;
    const String size = server.arg("size");
    if (server.arg("csrf") != csrf || (size != "qvga" && size != "vga")) {
      server.send(400, "text/plain", "INVALID REQUEST"); oledStatus("CAMERA WEB", "REQUEST ERR"); return;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    const bool ok = sensor && sensor->set_framesize(sensor, size == "qvga" ? FRAMESIZE_QVGA : FRAMESIZE_VGA) == 0;
    oledStatus("CAMERA WEB", ok ? "SIZE OK" : "SIZE ERR", size == "qvga" ? "320x240" : "640x480");
    server.send(ok ? 200 : 500, "text/plain", ok ? "SIZE OK; return to home" : "SIZE ERR");
  });
  server.onNotFound([]() { server.send(404, "text/plain", "NOT FOUND"); oledStatus("CAMERA WEB", "HTTP 404"); });
}
void setup() {
  if (!cameraBoot()) return;
  configured = !USING_EXAMPLE_SECRETS && !placeholder(WIFI_SSID) && !placeholder(WIFI_PASSWORD) &&
    !placeholder(CAMERA_WEB_USER) && !placeholder(CAMERA_WEB_PASSWORD) && strlen(CAMERA_WEB_PASSWORD) >= 12;
  if (!configured) { oledStatus("CAMERA WEB", "CONFIG ERR", "CHECK secrets.h"); return; }
  for (int i = 0; i < 4; ++i) snprintf(csrf + i * 8, 9, "%08lx", static_cast<unsigned long>(esp_random()));
  configureRoutes(); WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(false);
}
void loop() {
  if (!oledReady) { oledFallback(); return; }
  if (!cameraReady || !configured) { delay(100); return; }
  const uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED) {
    wifiAttempt = false;
    if (!serving) {
      server.begin(); serving = true;
      if (MDNS.begin("wrover-camera")) oledStatus("CAMERA WEB", "WEB READY", "wrover-camera.local");
      else oledStatus("CAMERA WEB", "MDNS ERR", "CHECK ROUTER IP LIST");
    }
    server.handleClient();
  } else {
    if (serving) {
      server.stop(); MDNS.end(); serving = false;
      nextWifi = now + 10000; oledStatus("CAMERA WEB", "LINK LOST", "RETRY IN 10s");
    }
    if (wifiAttempt && now - wifiStart >= 15000) {
      WiFi.disconnect(); wifiAttempt = false; nextWifi = now + 10000;
      oledStatus("CAMERA WEB", "WIFI TIMEOUT", "RETRY IN 10s");
    }
    if (!wifiAttempt && static_cast<int32_t>(now - nextWifi) >= 0) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD); wifiAttempt = true; wifiStart = now; ++retries;
    }
    if (wifiAttempt && now - lastWiFiPaint >= 500) {
      lastWiFiPaint = now; char detail[22];
      snprintf(detail, sizeof(detail), "TRY %lu / %lus", static_cast<unsigned long>(retries), static_cast<unsigned long>((now - wifiStart) / 1000));
      oledStatus("CAMERA WEB", "WIFI CONNECT", detail, "TIMEOUT 15s");
    }
  }
  delay(10);
}
