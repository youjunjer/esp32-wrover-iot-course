import assert from "node:assert/strict";
import { readFileSync, readdirSync } from "node:fs";
import { createHash } from "node:crypto";

// A Flow edit invalidates its recorded UI evidence until it is recaptured.
const directory = "docs/assets/part4/captures";
const sources = readFileSync(`${directory}/SOURCES.md`, "utf8");
const rows = [...sources.matchAll(/\| `([^`]+\.(?:png|json|txt))` \|[^\n]*?`([a-f0-9]{64})`/g)];
const names = rows.map((row) => row[1]);
assert.equal(new Set(names).size, names.length, "duplicate evidence entries");
const expected = [
  ...readdirSync(directory).filter((name) => /\.(png|txt)$/.test(name)),
  ...readdirSync("nodered/flows").filter((name) => name.endsWith(".json")),
].sort();
assert.deepEqual([...names].sort(), expected, "every public Flow and screenshot needs a source hash");
for (const [, name, expectedHash] of rows) {
  const path = name.endsWith(".json") ? `nodered/flows/${name}` : `${directory}/${name}`;
  const actual = createHash("sha256").update(readFileSync(path)).digest("hex");
  assert.equal(actual, expectedHash, `${name}: evidence is stale; rerun and recapture the affected lesson`);
}
console.log(`Node-RED evidence hashes passed (${rows.length} files).`);
