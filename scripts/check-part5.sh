#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
test_binary="$(mktemp "${TMPDIR:-/tmp}/part5-contracts.XXXXXX")"
trap 'rm -f "$test_binary"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror tests/part5_logic.cpp -o "$test_binary"
"$test_binary"
node scripts/test-camera-receiver.mjs
if git ls-files | rg '(^|/)(camera_config|secrets)\.h$'; then
  echo 'Private camera configuration tracked' >&2; exit 1
fi
if rg -n 'Serial\.(print|printf|println)|setInsecure\(' examples/05_camera_ble_multitasking; then
  echo 'Part 5 must keep private messages off Serial and verify TLS' >&2; exit 1
fi
python3 - <<'PYCODE'
from pathlib import Path
import re
root = Path('examples/05_camera_ble_multitasking')
for folder in sorted(root.glob('0[789]_*')):
    config = (folder / 'camera_config.example.h').read_text()
    for name in ['CAMERA_OLED_SDA', 'CAMERA_OLED_SCL', 'CAMERA_PIR_PIN']:
        assert re.search(r'\b' + name + r'\s*=\s*-1\s*;', config), 'Public pin config must remain unset'
    for name in ['OLED_PINS_VERIFIED', 'ENABLE_CAMERA_TEST', 'PIR_PIN_VERIFIED']:
        assert re.search(r'\b' + name + r'\s*=\s*false\s*;', config), 'Public camera gates must remain closed'
    ino = (folder / (folder.name + '.ino')).read_text()
    assert '#define CAMERA_MODEL_AI_THINKER' in ino
    assert 'beginOled(CAMERA_OLED_SDA, CAMERA_OLED_SCL)' in ino
    assert 'int sda = 21' not in ino, 'No default camera OLED on 21/22'
print('Public camera configuration gates passed')
PYCODE
