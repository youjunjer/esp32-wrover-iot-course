import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const flowFiles = [
  "nodered/flows/02_message_path.json",
  "nodered/flows/03_core_nodes.json",
  "nodered/flows/04_mqtt_tls_pubsub.json",
  "nodered/flows/05_json_validation.json",
  "nodered/flows/06_flowfuse_dashboard.json",
  "nodered/flows/07_safe_control.json",
];

const nodes = flowFiles.flatMap((path) => JSON.parse(readFileSync(path, "utf8")));

for (const node of nodes.filter((item) => item.type === "function")) {
  // Function nodes run inside Node-RED with these bindings available.
  new Function("msg", "node", "flow", "Buffer", node.func);
  if (node.initialize) {
    new Function("node", "flow", "Buffer", node.initialize);
  }
  if (node.finalize) {
    new Function("node", "flow", "Buffer", node.finalize);
  }
}

function functionNode(id) {
  const item = nodes.find((node) => node.id === id);
  assert(item, `missing node ${id}`);
  return new Function("msg", "node", "flow", "Buffer", item.func);
}

function createFlow({ separateFileStore = true } = {}) {
  const defaultStore = new Map();
  const fileStore = separateFileStore ? new Map() : defaultStore;
  const selectStore = (name) => (name === "file" ? fileStore : defaultStore);
  return {
    get(key, storeName) {
      return selectStore(storeName).get(key);
    },
    set(key, value, storeName) {
      const store = selectStore(storeName);
      if (value === undefined) store.delete(key);
      else store.set(key, value);
    },
  };
}

function execute(id, msg, flow = createFlow()) {
  const statuses = [];
  const node = { status(value) { statuses.push(value); } };
  const result = functionNode(id)(msg, node, flow, Buffer);
  return { result, statuses, flow };
}

const now = Math.floor(Date.now() / 1000);
const exactTelemetry = {
  v: 1,
  device: "REPLACE-WITH-DEVICE-ID",
  seq: 1,
  ts: now,
  temp: 26.4,
  humi: 63,
  light: 2048,
  status: "ok",
};

{
  const msg = {
    topic: "REPLACE_WITH_UNIQUE_TOPIC_ROOT/data",
    payload: { ...exactTelemetry },
    _payloadBytes: Buffer.byteLength(JSON.stringify(exactTelemetry), "utf8"),
    _receivedAt: Date.now(),
    demo: true,
  };
  const { result } = execute("4050000000000008", msg);
  assert(result[0], "lesson 05 valid telemetry must be accepted");
  assert.equal(Object.keys(result[0].payload).length, 8, "payload must remain exactly eight fields");
  assert.equal(Object.hasOwn(result[0].payload, "age_s"), false, "age belongs in validation sidecar");
  assert.equal(result[0].validation.ok, true);
  assert(Number.isInteger(result[0].validation.age_s));
}

{
  const envelope = {
    payload: { ...exactTelemetry },
    validation: { ok: true, code: "OK", age_s: 0 },
    demo: true,
  };
  const { result } = execute("d600000000000016", envelope);
  assert.equal(result[0].dashboardMode, "DEMO");
  assert.equal(result[2], null, "DEMO must never arm the LIVE watchdog");
}

{
  const envelope = {
    payload: { ...exactTelemetry },
    validation: { ok: true, code: "OK", age_s: 0 },
  };
  const { result } = execute("d600000000000016", envelope);
  assert.equal(result[0].dashboardMode, "LIVE");
  assert.match(result[2].payload, /^LIVE /);
}

{
  const { result } = execute("d600000000000016", {
    validation: { ok: false, code: "STALE", age_s: 11 },
  });
  assert.equal(result[0], null, "rejected telemetry must not update widgets");
  assert.match(result[1].payload, /STALE/);
}

{
  const flow = createFlow({ separateFileStore: false });
  flow.set("controllerBootId", "12ab34cd");
  flow.set("deviceOnline", true);
  flow.set("statusReceivedAt", Date.now());
  const { result } = execute("d700000000000014", {
    payload: { target: "traffic", value: "green", ttl: 10 },
  }, flow);
  assert.equal(result[0], null, "aliased/missing file store must fail closed");
  assert.match(result[1].payload, /absent or aliases/);
}

let command;
let commandFlow;
{
  commandFlow = createFlow({ separateFileStore: true });
  commandFlow.set("controllerBootId", "12ab34cd");
  commandFlow.set("deviceOnline", true);
  commandFlow.set("statusReceivedAt", Date.now());
  const { result } = execute("d700000000000014", {
    payload: { target: "traffic", value: "green", ttl: 10 },
  }, commandFlow);
  assert(result[0], "separate file store and online device should produce a command");
  command = JSON.parse(result[0].payload);
  assert.equal(Object.keys(command).length, 8);
  assert.equal(commandFlow.get("part4CmdSeq", "file"), 1);
  assert.equal(result[0].retain, false);
}

{
  const flow = createFlow();
  flow.set("controllerBootId", "12ab34cd");
  flow.set("deviceOnline", true);
  const { result } = execute("d700000000000017", {
    status: { text: "node-red:common.status.connected" },
  }, flow);
  assert.equal(result, null, "Node-RED 5 connected status token must be recognized");
  assert.equal(flow.get("controllerBootId"), "12ab34cd");
}

{
  const flow = createFlow();
  flow.set("controllerBootId", "12ab34cd");
  flow.set("deviceOnline", true);
  flow.set("pendingCommand", { cmd_id: "x", cmd_seq: 1, boot_id: "12ab34cd" });
  const { result } = execute("d700000000000017", {
    status: { text: "node-red:common.status.disconnected" },
  }, flow);
  assert.match(result.payload, /BLOCKED/);
  assert.equal(flow.get("controllerBootId"), null);
  assert.equal(flow.get("pendingCommand"), null);
}

const ack = {
  v: 1,
  cmd_id: command.cmd_id,
  cmd_seq: command.cmd_seq,
  ok: false,
  reason: "REPLAY/DUPLICATE",
  boot_id: command.boot_id,
  ts: now,
};

{
  const { result } = execute("d700000000000009", {
    payload: JSON.stringify(ack),
    retain: true,
  }, commandFlow);
  assert.match(result.payload, /retained ACK is prohibited/);
}

{
  const noPending = createFlow();
  noPending.set("controllerBootId", command.boot_id);
  const { result } = execute("d700000000000009", {
    payload: JSON.stringify(ack),
    retain: false,
  }, noPending);
  assert.match(result.payload, /ACK UNMATCHED/);
}

{
  const { result } = execute("d700000000000009", {
    payload: JSON.stringify(ack),
    retain: false,
  }, commandFlow);
  assert.match(result.payload, /ACK MATCHED REJECTED/);
  assert.equal(commandFlow.get("pendingCommand"), null);
}

console.log("Node-RED Function syntax and contract tests passed.");
