#pragma once
#include <Arduino.h>
#include <esp_camera.h>
#if !defined(CAMERA_MODEL_AI_THINKER)
#error Select CAMERA_MODEL_AI_THINKER while retaining the Wrover FQBN.
#endif
inline esp_err_t initializeCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0; config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5; config.pin_d1 = 18; config.pin_d2 = 19; config.pin_d3 = 21;
  config.pin_d4 = 36; config.pin_d5 = 39; config.pin_d6 = 34; config.pin_d7 = 35;
  config.pin_xclk = 0; config.pin_pclk = 22; config.pin_vsync = 25; config.pin_href = 23;
  config.pin_sccb_sda = 26; config.pin_sccb_scl = 27; config.pin_pwdn = 32; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 14; config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM; config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  const esp_err_t error = esp_camera_init(&config);
  if (error != ESP_OK) return error;
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor || sensor->set_framesize(sensor, FRAMESIZE_QVGA) != 0) {
    esp_camera_deinit(); return ESP_FAIL;
  }
  return ESP_OK;
}
