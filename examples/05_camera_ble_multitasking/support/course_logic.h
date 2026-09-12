#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace Course {
// Pure C++ contracts, also exercised by the host tests.
enum class LineResult { Waiting, Ready, Invalid };
struct LineReader {
  char text[65] = {};
  size_t length = 0;
  bool invalid = false;
  uint32_t started = 0;
  void reset() { length = 0; invalid = false; text[0] = 0; }
  bool expired(uint32_t now) const { return (length || invalid) && now - started >= 2000; }
  LineResult push(char c, uint32_t now) {
    if (!length && !invalid) started = now;
    if (c == '\r') return LineResult::Waiting;
    if (c == '\n') {
      text[length] = 0;
      return invalid || !length ? LineResult::Invalid : LineResult::Ready;
    }
    if (c < 32 || c > 126 || length >= 64) invalid = true;
    if (!invalid) text[length++] = c;
    return LineResult::Waiting;
  }
};
inline int servoAngle(const char *text) {
  if (strncmp(text, "SERVO ", 6) != 0) return -1;
  const char *p = text + 6;
  if (!*p || strlen(p) > 3) return -1;
  int value = 0;
  for (; *p; ++p) {
    if (*p < '0' || *p > '9') return -1;
    value = value * 10 + (*p - '0');
  }
  return value <= 180 ? value : -1;
}
// Public teaching identity; it is neither a person ID nor authentication.
constexpr uint8_t LAB_BEACON[25] = {
  0x4c,0x00,0x02,0x15, 0x71,0x47,0x3d,0x62,0xa1,0x52,0x4c,0x66,
  0x8e,0x20,0x6c,0x61,0x62,0x30,0x30,0x31, 0x00,0x01,0x00,0x01,0xc5
};
inline bool isLabBeacon(const uint8_t *data, size_t size) {
  return size == sizeof(LAB_BEACON) && memcmp(data, LAB_BEACON, 24) == 0;
}
// Logical exclusions are necessary, but never prove physical board routing.
inline bool cameraReserved(int pin) {
  constexpr int reserved[] = {0,5,18,19,21,22,23,25,26,27,32,34,35,36,39};
  for (int value : reserved) if (pin == value) return true;
  return false;
}
inline bool alternativePin(int pin) {
  // Excludes Flash, PSRAM, UART, fallback LED, input-only and camera pins.
  return pin >= 0 && pin <= 33 && !(pin >= 6 && pin <= 11) &&
    pin != 1 && pin != 2 && pin != 3 && pin != 16 && pin != 17 &&
    pin != 20 && pin != 24 && pin != 28 && pin != 29 && pin != 30 && pin != 31 &&
    !cameraReserved(pin);
}
inline bool cameraOledPins(int sda, int scl) {
  return sda != scl && alternativePin(sda) && alternativePin(scl);
}
constexpr size_t JPEG_LIMIT = 48 * 1024;
constexpr size_t CHUNK_BYTES = 384;
inline size_t chunkCount(size_t bytes) { return (bytes + CHUNK_BYTES - 1) / CHUNK_BYTES; }
inline uint32_t crc32(const uint8_t *data, size_t size) {
  uint32_t crc = 0xffffffff;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return ~crc;
}
}
