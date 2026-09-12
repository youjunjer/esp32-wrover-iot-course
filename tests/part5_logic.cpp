#include <initializer_list>
#include "../examples/05_camera_ble_multitasking/support/course_logic.h"
#include <cassert>
#include <cstdio>
using namespace Course;
int main() {
  LineReader line;
  for (char c : "PING") if (c) assert(line.push(c, 0) == LineResult::Waiting);
  assert(line.push('\n', 1) == LineResult::Ready && !strcmp(line.text, "PING"));
  line.reset(); for (int i = 0; i < 65; ++i) line.push('A', 5);
  assert(line.push('\n', 6) == LineResult::Invalid);
  line.reset(); line.push('\0', 0); assert(line.push('\n', 1) == LineResult::Invalid);
  line.reset(); line.push('P', 0xfffffff0); assert(!line.expired(20)); assert(line.expired(2100));
  assert(servoAngle("SERVO 0") == 0 && servoAngle("SERVO 180") == 180);
  for (const char *bad : {"SERVO -1", "SERVO 181", "SERVO 90x", "SERVO 1.5", "SERVO ", "SERVO 9999", "SERVO +5"}) assert(servoAngle(bad) == -1);
  assert(isLabBeacon(LAB_BEACON, 25) && !isLabBeacon(LAB_BEACON, 24));
  uint8_t other[25]; memcpy(other, LAB_BEACON, 25); other[4] ^= 1; assert(!isLabBeacon(other, 25));
  // Logical test inputs only. These numbers do NOT prove course-board wiring.
  assert(cameraOledPins(13, 14));
  assert(!cameraOledPins(21, 22) && !cameraOledPins(16, 17) && !cameraOledPins(-1, -1) && !cameraOledPins(13, 13));
  for (int pin : {0, 2, 5, 6, 11, 16, 17, 18, 21, 22, 26, 32, 34, 39}) assert(!alternativePin(pin));
  assert(crc32(reinterpret_cast<const uint8_t *>("123456789"), 9) == 0xcbf43926);
  assert(chunkCount(384) == 1 && chunkCount(385) == 2 && chunkCount(JPEG_LIMIT) == 128);
  puts("Part 5 C++ contracts passed");
}
