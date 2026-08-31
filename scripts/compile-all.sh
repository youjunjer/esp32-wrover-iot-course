#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/config/toolchain.lock"

while IFS= read -r sketch || [[ -n "$sketch" ]]; do
  [[ -z "$sketch" || "$sketch" == \#* ]] && continue
  echo "Compiling $sketch"
  arduino-cli --config-file "$repo_root/arduino-cli.yaml" compile \
    --fqbn "$FQBN" \
    --warnings all \
    "$repo_root/$sketch"
done < "$repo_root/config/sketches.txt"
