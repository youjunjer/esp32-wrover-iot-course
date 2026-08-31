#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
while IFS= read -r library || [[ -n "$library" ]]; do
  [[ -z "$library" || "$library" == \#* ]] && continue
  arduino-cli --config-file "$repo_root/arduino-cli.yaml" lib install "$library"
done < "$repo_root/config/libraries.lock"
