import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { resolve, join } from "node:path";
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { spawn } from "node:child_process";
import { setTimeout as delay } from "node:timers/promises";

// Test-only injection exercises the unchanged public nodes in a real Runtime.
// Brokers stay disabled. Synthetic status/ACK/data are not hardware evidence.
const mode = process.argv[2];
if (!mode) {
  const root = mkdtempSync(join(tmpdir(), "esp32-nodered-test-"));
  try {
    for (const scenario of ["lessons", "missing-store", "persistent-first", "persistent-restart"]) {
      await new Promise((done, reject) => {
        const userDir = join(root, scenario.startsWith("persistent") ? "persistent" : scenario);
        const child = spawn(process.execPath, [process.argv[1], scenario, userDir], { stdio: "inherit" });
        child.on("error", reject);
        child.on("exit", (code) => code === 0 ? done() : reject(new Error(`${scenario} failed (${code})`)));
      });
    }
    console.log("Node-RED runtime tests passed: six public Flows, 45s watchdog, fail-closed gate and persisted sequence.");
  } finally {
    rmSync(root, { recursive: true, force: true });
  }
} else {
  const require = createRequire(import.meta.url);
  const modules = resolve(process.env.NODE_RED_MODULES || "nodered/node_modules");
  const RED = require(join(modules, "node-red"));
  assert.equal(RED.version(), "5.0.7");
  assert.equal(require(join(modules, "@flowfuse/node-red-dashboard/package.json")).version, "1.31.0");
  const records = [];
  const runtimeErrors = [];
  const server = createServer((req, res) => RED.httpAdmin(req, res, () => RED.httpNode(req, res, () => {
    res.writeHead(404); res.end();
  })));
  RED.init(server, {
    userDir: process.argv[3],
    flowFile: "flows.json",
    nodesDir: [modules],
    uiHost: "127.0.0.1",
    telemetry: { enabled: false },
    contextStorage: mode.startsWith("persistent") ? {
      default: "memoryOnly", memoryOnly: { module: "memory" }, file: { module: "localfilesystem" },
    } : { default: { module: "memory" } },
    logging: {
      console: { level: "off", metrics: false, audit: false },
      test: { level: "error", handler: () => (entry) => runtimeErrors.push(entry.msg) },
    },
  });
  RED.hooks.add("onSend.course-test", (events) => {
    for (const event of events) records.push({ source: event.source.id, port: event.source.port,
      destination: event.destination.id, msg: structuredClone(event.msg) });
  });
  async function until(check, label, timeout = 5000) {
    const end = Date.now() + timeout;
    while (Date.now() < end) {
      const result = check();
      if (result) return result;
      await delay(25);
    }
    throw new Error(`Timed out: ${label}`);
  }
  function inject(id, msg = {}) {
    const node = RED.nodes.getNode(id);
    assert(node, `loaded node ${id}`);
    records.length = 0;
    node.receive(structuredClone(msg));
  }
  const sent = (source, match = () => true) => records.find((entry) => entry.source === source && match(entry.msg, entry));
  try {
    await RED.start();
    await new Promise((done) => server.listen(0, "127.0.0.1", done));
    const files = mode === "lessons" ? ["02_message_path", "03_core_nodes", "04_mqtt_tls_pubsub", "05_json_validation", "06_flowfuse_dashboard"] : ["07_safe_control"];
    const flows = files.flatMap((file) => JSON.parse(readFileSync(`nodered/flows/${file}.json`, "utf8")));
    for (const node of flows.filter((node) => node.type === "mqtt-broker")) assert.equal(node.autoConnect, false);
    await RED.runtime.flows.setFlows({ flows: { flows } });
    await until(() => RED.nodes.getNode(mode === "lessons" ? "d600000000000008" : "d700000000000014"), "loaded functions");
    await delay(250); // Allow Function on-start and the disabled MQTT node status events to settle.
    const base = `http://127.0.0.1:${server.address().port}`;
    assert.equal((await fetch(`${base}/`)).status, 200);
    assert.equal((await fetch(`${base}${mode === "lessons" ? "/dashboard/part4-demo" : "/control-dashboard/safe-control"}`)).status, 200);
    if (mode === "lessons") {
      inject("4020000000000003");
      const basic = await until(() => sent("4020000000000003"), "Inject to Debug");
      assert.equal(basic.msg.payload.temp, 26.4);
      assert.equal(typeof basic.msg._msgid, "string");
      for (const [id, expected] of [["4030000000000003", 82.4], ["4030000000000004", "temp_not_number"], ["403000000000000c", "temp_out_of_range"]]) {
        inject(id);
        const result = await until(() => sent("4030000000000006"), "core node routing");
        assert.equal(result.msg.payload.temp_f ?? result.msg.payload.reason, expected);
      }
      inject("4050000000000003");
      const accepted = await until(() => sent("d600000000000008"), "lesson 5 Link to Dashboard widgets");
      assert.equal(sent("4050000000000008").msg.demo, true);
      assert.equal(Object.keys(sent("4050000000000008").msg.payload).length, 8);
      assert.equal(accepted.msg.payload, 26.4);
      assert(!sent("d600000000000016", (msg, entry) => entry.port === 2), "DEMO cannot arm LIVE watchdog");
      for (const [id, source, code] of [["4050000000000004", "4050000000000008", "TEMP_RANGE"], ["4050000000000005", "405000000000000c", "JSON_PARSE"]]) {
        inject(id);
        const rejected = await until(() => sent(source, (msg) => msg.validation?.ok === false), code);
        assert.equal(rejected.msg.validation.code, code);
        assert.equal(rejected.msg.payload, undefined, "invalid input is suppressed");
      }
      const valid = { v: 1, device: "REPLACE-WITH-DEVICE-ID", seq: 1, ts: Math.floor(Date.now()/1000), temp: 26.4, humi: 63, light: 2048, status: "ok" };
      for (const [payload, code] of [[{ ...valid, ts: valid.ts - 11 }, "STALE"], [{ ...valid, extra: 1 }, "FIELDS"], [{ ...valid, seq: "1" }, "SEQ"], ["x".repeat(385), "PAYLOAD_SIZE"]]) {
        inject("405000000000000d", { topic: "REPLACE_WITH_UNIQUE_TOPIC_ROOT/data", payload: typeof payload === "string" ? payload : JSON.stringify(payload), demo: true });
        const rejected = await until(() => records.find((entry) => entry.msg.validation?.code === code), code);
        assert.equal(rejected.msg.payload, undefined);
      }
      for (const [id, text] of [["d600000000000004", "INVALID"], ["d600000000000005", "STALE"]]) {
        inject(id);
        await until(() => sent("d600000000000007", (msg) => String(msg.payload).includes(text)), text);
        assert(!sent("d600000000000008"), "invalid/stale samples cannot update gauges");
      }
      // Synthetic LIVE input verifies timing only; no Broker or ESP32 is used.
      inject("405000000000000d", { topic: "REPLACE_WITH_UNIQUE_TOPIC_ROOT/data", payload: JSON.stringify({ ...valid, ts: Math.floor(Date.now()/1000) }) });
      await until(() => sent("d600000000000016", (msg, entry) => entry.port === 2 && msg.payload.startsWith("LIVE")), "synthetic LIVE pulse");
      console.log("PASS lessons 2–6; waiting for the unchanged 45s LIVE watchdog.");
      await until(() => sent("d600000000000017", (msg) => msg.payload.startsWith("OFFLINE")), "45s OFFLINE watchdog", 48000);
    } else {
      const statusId = "d700000000000007";
      const commandId = "d700000000000014";
      const request = { payload: { target: "traffic", value: "green", ttl: 10 } };
      inject(commandId, request);
      await until(() => sent(commandId, (msg) => msg.payload.startsWith("BLOCKED")), "no status blocks control");
      inject(statusId, { payload: '{"online":true,"boot_id":"12ab34cd"}', retain: true });
      await until(() => sent(statusId, (msg) => msg.payload.startsWith("READY")), "synthetic status");
      inject(commandId, request);
      if (mode === "missing-store") {
        await until(() => sent(commandId, (msg) => msg.payload.includes("absent or aliases")), "missing file store blocks control");
        assert(!records.some((entry) => entry.destination === "d700000000000015"), "blocked request cannot reach MQTT Out");
      } else {
        const output = await until(() => sent(commandId, (msg, entry) => entry.port === 0), "valid command envelope");
        const command = JSON.parse(output.msg.payload);
        assert.equal(command.cmd_seq, mode === "persistent-first" ? 1 : 2, "sequence survives a real Runtime restart");
        assert.equal(Object.keys(command).length, 8);
        assert.equal(output.msg.retain, false);
        inject("d700000000000009", { payload: JSON.stringify({ v: 1, cmd_id: command.cmd_id, cmd_seq: command.cmd_seq, boot_id: command.boot_id, ok: false, reason: "REPLAY/DUPLICATE", ts: Math.floor(Date.now()/1000) }), retain: false });
        await until(() => sent("d700000000000009", (msg) => msg.payload.startsWith("ACK MATCHED REJECTED")), "matching synthetic ACK");
        inject("d700000000000017", { status: { text: "node-red:common.status.disconnected" } });
        await until(() => sent("d700000000000017"), "disconnect invalidates status");
        inject(commandId, request);
        await until(() => sent(commandId, (msg) => msg.payload.startsWith("BLOCKED")), "disconnect blocks next command");
      }
    }
    assert.deepEqual(runtimeErrors, [], "no Runtime node errors");
    console.log(`PASS ${mode} (Node-RED 5.0.7 / Dashboard 1.31.0 / ${process.version})`);
  } finally {
    await RED.stop();
    await new Promise((done) => server.close(done));
  }
}
