#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/config/toolchain.lock"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found. Install Arduino CLI ${ARDUINO_CLI_VERSION} first." >&2
  exit 1
fi

arduino-cli --config-file "$repo_root/arduino-cli.yaml" core update-index
arduino-cli --config-file "$repo_root/arduino-cli.yaml" core install "esp32:esp32@${ESP32_CORE_VERSION}"
arduino-cli --config-file "$repo_root/arduino-cli.yaml" core list
