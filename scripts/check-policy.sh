#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

failed=0

if git ls-files | grep -E '(^|/)secrets\.h$|(^|/)credentials\.h$|(^|/)\.env($|\.)' >/dev/null; then
  echo "Tracked credential file detected." >&2
  failed=1
fi

if rg -l --glob '*.ino' --glob '*.h' --glob '*.cpp' \
  --glob '!**/secrets.h' --glob '!**/credentials.h' \
  '(BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY|AIza[0-9A-Za-z_-]{30,}|gh[pousr]_[0-9A-Za-z]{20,})' .; then
  echo "Possible secret detected." >&2
  failed=1
fi

if rg -l -U --pcre2 --glob '*.ino' --glob '*.h' --glob '*.hpp' --glob '*.cpp' \
  --glob '!**/secrets.h' --glob '!**/credentials.h' \
  '(WIFI_SSID|WIFI_PASSWORD|MQTT_USERNAME|MQTT_PASSWORD|THINGSPEAK_WRITE_API_KEY|GOOGLE_SHEETS_GAS_URL|GOOGLE_SHEET_ID|GOOGLE_SHEET_TAG)[^=\n]*=[[:space:]]*"(?![^"\n]*(?:REPLACE_WITH_|YOUR_))[^"\n]+"' \
  examples; then
  echo "Non-placeholder credential assignment detected." >&2
  failed=1
fi

if rg -l --pcre2 --glob '!**/secrets.h' --glob '!**/credentials.h' \
  'https://script\.google\.com/macros/s/(?!(?:REPLACE_WITH_|YOUR_))[0-9A-Za-z_-]+/exec' \
  examples; then
  echo "Hard-coded Google Apps Script deployment URL detected." >&2
  failed=1
fi

if rg -l --glob '*.ino' 'CAMERA_MODEL_AI_THINKER' examples \
  | grep -v '^examples/05_camera_ble_multitasking/' >/dev/null; then
  echo "AI Thinker camera define found outside part 05." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' --glob '*.hpp' --glob '*.cpp' \
  'Serial[0-9]*\.(begin|print|println|printf|write)' examples/01_basics; then
  echo "Part 01 must not depend on Serial output." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' --glob '*.hpp' --glob '*.cpp' \
  'analogRead[[:space:]]*\(' examples/01_basics; then
  echo "Analog sensing must begin after the OLED interface in part 02." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' --glob '*.hpp' --glob '*.cpp' \
  '(analogWrite|ledcSetup|ledcAttachPin)[[:space:]]*\(' examples; then
  echo "Legacy PWM API detected; use the ESP32 Core 3.x LEDC API." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' --glob '*.hpp' --glob '*.cpp' \
  'setInsecure[[:space:]]*\(' examples; then
  echo "TLS certificate verification bypass detected." >&2
  failed=1
fi

if rg -n --glob '*.ino' 'http://[^"[:space:]]+' examples/03_network_cloud_mqtt; then
  echo "Plain HTTP URL detected in a network sketch; client endpoints must use HTTPS." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' \
  '(PubSubClient|MQTT_PORT[[:space:]]*=[[:space:]]*1883|eric1030/class70)' \
  examples/03_network_cloud_mqtt; then
  echo "Legacy or plaintext MQTT configuration detected in part 03." >&2
  failed=1
fi

while IFS= read -r mqtt_ino; do
  if ! grep -Fq '#include <NetworkClientSecure.h>' "$mqtt_ino" ||
     ! grep -Fq 'setCACert(MQTT_ROOT_CA)' "$mqtt_ino"; then
    echo "MQTT sketch must use NetworkClientSecure with a configured CA: $mqtt_ino" >&2
    failed=1
  fi
done < <(find examples/03_network_cloud_mqtt -type f -name '*mqtt*.ino' | LC_ALL=C sort)

control_ino="examples/03_network_cloud_mqtt/11_mqtt_control_oled/11_mqtt_control_oled.ino"
if [[ -f "$control_ino" ]] &&
   { ! grep -Fq 'digitalWrite(RELAY_PIN, HIGH)' "$control_ino" ||
     ! grep -Fq 'safeOutputs();' "$control_ino"; }; then
  echo "MQTT control lesson is missing the active-low relay safe-off path." >&2
  failed=1
fi

if rg -n -U --glob '*.ino' \
  'while[[:space:]]*\([^)]*WiFi\.(status|waitForConnectResult)' \
  examples/03_network_cloud_mqtt; then
  echo "Blocking Wi-Fi connection loop detected; use a timed OLED-visible state machine." >&2
  failed=1
fi

while IFS= read -r network_ino; do
  network_dir="${network_ino%/*}"
  if [[ ! -f "$network_dir/secrets.example.h" ]]; then
    echo "Network sketch is missing secrets.example.h: $network_dir" >&2
    failed=1
  fi
  if ! grep -Fq '__has_include("secrets.h")' "$network_ino"; then
    echo "Network sketch does not use the secrets.h fallback pattern: $network_ino" >&2
    failed=1
  fi
done < <(find examples/03_network_cloud_mqtt -type f -name '*.ino' | LC_ALL=C sort)

while IFS= read -r sketch || [[ -n "$sketch" ]]; do
  [[ -z "$sketch" || "$sketch" == \#* ]] && continue
  sketch_name="$(basename "$sketch")"
  if [[ ! -f "$sketch/$sketch_name.ino" ]]; then
    echo "Sketch folder and main .ino name do not match: $sketch" >&2
    failed=1
  fi
  if [[ ! -f "$sketch/README.md" ]]; then
    echo "Sketch documentation missing: $sketch/README.md" >&2
    failed=1
  fi
done < config/sketches.txt

while IFS= read -r ino_file; do
  sketch_dir="${ino_file%/*}"
  if ! grep -Fxq "$sketch_dir" config/sketches.txt; then
    echo "Sketch is not included in config/sketches.txt: $sketch_dir" >&2
    failed=1
  fi
done < <(find examples -type f -name '*.ino' | LC_ALL=C sort)

if find . -type d \( -name .git -o -name node_modules -o -name .node-red-course \) -prune -o -type f -size +10M -print | grep -q .; then
  echo "File larger than 10 MiB detected." >&2
  failed=1
fi

if [[ -x scripts/check-nodered.sh ]] && ! scripts/check-nodered.sh; then
  failed=1
fi

exit "$failed"
