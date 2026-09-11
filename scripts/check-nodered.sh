#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required to validate Node-RED flow JSON." >&2
  exit 1
fi

expected_flows=(
  nodered/flows/02_message_path.json
  nodered/flows/03_core_nodes.json
  nodered/flows/04_mqtt_tls_pubsub.json
  nodered/flows/05_json_validation.json
  nodered/flows/06_flowfuse_dashboard.json
  nodered/flows/07_safe_control.json
)

for flow_file in "${expected_flows[@]}"; do
  if [[ ! -f "$flow_file" ]]; then
    echo "Missing Node-RED lesson flow: $flow_file" >&2
    exit 1
  fi

  jq -e 'type == "array" and length > 0' "$flow_file" >/dev/null
  jq -e '([.[].id] | length) == ([.[].id] | unique | length)' \
    "$flow_file" >/dev/null
  jq -e '
    [.[].id] as $ids |
    [ .[] | (.wires? // [])[][]? | . as $target | select(($ids | index($target)) == null) ] |
    length == 0
  ' "$flow_file" >/dev/null
done

jq -s -e 'add | ([.[].id] | length) == ([.[].id] | unique | length)' \
  "${expected_flows[@]}" >/dev/null

jq -s -e '
  add as $nodes |
  [$nodes[].id] as $ids |
  [ $nodes[] | (.links? // [])[]? | . as $target | select(($ids | index($target)) == null) ] |
  length == 0
' "${expected_flows[@]}" >/dev/null

jq -e '
  .engines.node == "24.x" and
  .dependencies["node-red"] == "5.0.7" and
  .dependencies["@flowfuse/node-red-dashboard"] == "1.31.0"
' nodered/package.json >/dev/null

jq -e '
  .lockfileVersion == 3 and
  .packages[""].engines.node == "24.x" and
  .packages[""].dependencies["node-red"] == "5.0.7" and
  .packages[""].dependencies["@flowfuse/node-red-dashboard"] == "1.31.0" and
  .packages["node_modules/node-red"].version == "5.0.7" and
  .packages["node_modules/@flowfuse/node-red-dashboard"].version == "1.31.0"
' nodered/package-lock.json >/dev/null

if [[ "$(tr -d '[:space:]' < nodered/.node-version)" != "24" ]]; then
  echo "nodered/.node-version must lock Node.js major version 24." >&2
  exit 1
fi

if rg -n '"type"[[:space:]]*:[[:space:]]*"ui_[A-Za-z]|"node-red-dashboard"[[:space:]]*:' \
  nodered/flows nodered/package.json; then
  echo "Legacy Dashboard nodes or package detected." >&2
  exit 1
fi

if rg -n --pcre2 '(https?://|mqtts?://|wss?://|(?<![0-9])(?:10(?:\.[0-9]{1,3}){3}|127(?:\.[0-9]{1,3}){3}|169\.254(?:\.[0-9]{1,3}){2}|192\.168(?:\.[0-9]{1,3}){2}|172\.(?:1[6-9]|2[0-9]|3[01])(?:\.[0-9]{1,3}){2})(?![0-9]))' \
  nodered/flows; then
  echo "A URL or private/local IP was embedded in a public Node-RED flow." >&2
  exit 1
fi

jq -s -e '
  add |
  all(.[ ];
    if .type == "mqtt-broker" then
      (.broker | type == "string" and startswith("REPLACE_WITH_MQTT_HOST")) and
      (.port == "8883" or .port == 8883) and
      .autoConnect == false and
      .usetls == true and
      (.tls | type == "string" and length > 0) and
      (.clientid | type == "string" and startswith("REPLACE_WITH_"))
    elif .type == "tls-config" then
      .verifyservercert == true
    elif .type == "mqtt in" or .type == "mqtt out" then
      ((.topic | type) == "string") and
      (.topic | startswith("REPLACE_WITH_UNIQUE_TOPIC_ROOT/")) and
      ((.topic | contains("#")) | not) and
      ((.topic | contains("+")) | not)
    else true
    end
  ) and
  all(.[]; if .type == "mqtt out" then (.retain == "false" or .retain == false) else true end) and
  ([.. | objects | select(has("credentials"))] | length == 0)
' "${expected_flows[@]}" >/dev/null

if rg -l -U --pcre2 \
  '(BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY|AIza[0-9A-Za-z_-]{30,}|gh[pousr]_[0-9A-Za-z]{20,}|script\.google\.com/macros/s/)' \
  nodered/flows nodered/package.json nodered/package-lock.json nodered/README.md; then
  echo "Possible secret or unrelated deployment endpoint detected in Node-RED material." >&2
  exit 1
fi

if git ls-files nodered | rg '(^|/)(node_modules|\.node-red|\.node-red-course)(/|$)|flows_cred|credentials'; then
  echo "Tracked Node-RED runtime or credential material detected." >&2
  exit 1
fi

if ! command -v node >/dev/null 2>&1; then
  echo "Node.js is required to validate Node-RED Function nodes." >&2
  exit 1
fi

node scripts/test-nodered-functions.mjs
node scripts/check-nodered-evidence.mjs

echo "Node-RED lesson flow checks passed."
