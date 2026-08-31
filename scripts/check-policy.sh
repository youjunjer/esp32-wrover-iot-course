#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

failed=0

if git ls-files | grep -E '(^|/)secrets\.h$|(^|/)credentials\.h$|(^|/)\.env($|\.)' >/dev/null; then
  echo "Tracked credential file detected." >&2
  failed=1
fi

if rg -n --glob '*.ino' --glob '*.h' --glob '*.cpp' \
  '(BEGIN (RSA |OPENSSH )?PRIVATE KEY|AIza[0-9A-Za-z_-]{30,}|gh[pousr]_[0-9A-Za-z]{20,})' .; then
  echo "Possible secret detected." >&2
  failed=1
fi

if rg -l --glob '*.ino' 'CAMERA_MODEL_AI_THINKER' examples \
  | grep -v '^examples/05_camera_ble_multitasking/' >/dev/null; then
  echo "AI Thinker camera define found outside part 05." >&2
  failed=1
fi

while IFS= read -r sketch || [[ -n "$sketch" ]]; do
  [[ -z "$sketch" || "$sketch" == \#* ]] && continue
  sketch_name="$(basename "$sketch")"
  if [[ ! -f "$sketch/$sketch_name.ino" ]]; then
    echo "Sketch folder and main .ino name do not match: $sketch" >&2
    failed=1
  fi
done < config/sketches.txt

if find . -type f -size +10M -not -path './.git/*' -print -quit | grep -q .; then
  echo "File larger than 10 MiB detected." >&2
  failed=1
fi

exit "$failed"
