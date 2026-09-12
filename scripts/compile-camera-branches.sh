#!/usr/bin/env bash
# Compile enabled camera paths with synthetic configuration; NEVER upload these.
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/config/toolchain.lock"
staging="$(mktemp -d "${TMPDIR:-/tmp}/part5-camera-build.XXXXXX")"
trap 'rm -rf "$staging"' EXIT
mkdir -p "$staging/05_camera_ble_multitasking"
cp -R "$repo_root/examples/05_camera_ble_multitasking/support" "$staging/05_camera_ble_multitasking/"
for source_dir in "$repo_root"/examples/05_camera_ble_multitasking/0{7,8,9}_*; do
  name="$(basename "$source_dir")"
  target="$staging/05_camera_ble_multitasking/$name"
  mkdir -p "$target"
  # Only public files. Never copy a maintainer's secrets or verified pin config.
  cp "$source_dir/$name.ino" "$source_dir/camera_config.example.h" "$target/"
  cat > "$target/camera_config.h" <<'CONFIG'
#pragma once
// Compile fixture ONLY: this is not a validated physical pin recommendation.
constexpr int CAMERA_OLED_SDA = 13;
constexpr int CAMERA_OLED_SCL = 14;
constexpr bool OLED_PINS_VERIFIED = true;
constexpr bool ENABLE_CAMERA_TEST = true;
constexpr int CAMERA_PIR_PIN = 33;
constexpr bool PIR_PIN_VERIFIED = true;
CONFIG
  if [[ -f "$source_dir/secrets.example.h" ]]; then
    cp "$source_dir/secrets.example.h" "$target/"
    # Synthetic values keep the enabled runtime paths in the linked image.
    python3 - "$target" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1])
s = (p / 'secrets.example.h').read_text()
s = re.sub(r'"REPLACE_WITH_[A-Z_]+"', '"compile-fixture"', s)
s = s.replace('MQTT_HOST[] = "compile-fixture"', 'MQTT_HOST[] = "invalid.example"')
s = s.replace('MQTT_ROOT_CA[] = "compile-fixture"', 'MQTT_ROOT_CA[] = "-----BEGIN CERTIFICATE-----\\nCOMPILE_FIXTURE_ONLY\\n-----END CERTIFICATE-----"')
(p / 'secrets.h').write_text(s)
PY
  fi
  echo "Compiling enabled camera branch: $name (synthetic config, never upload)"
  arduino-cli --config-file "$repo_root/arduino-cli.yaml" compile --fqbn "$FQBN" --warnings all "$target"
done
