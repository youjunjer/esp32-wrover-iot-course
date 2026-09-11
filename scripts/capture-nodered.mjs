import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { readFileSync, writeFileSync } from "node:fs";
import { resolve } from "node:path";

// Maintainer tool: run against a freshly created, loopback-only course Runtime.
// Install playwright-core separately; it is not a student Runtime dependency.
const require = createRequire(import.meta.url);
const { chromium } = require(process.env.PLAYWRIGHT_MODULE_PATH || "playwright-core");
const playwrightVersion = require(process.env.PLAYWRIGHT_MODULE_PATH
  ? resolve(process.env.PLAYWRIGHT_MODULE_PATH, "package.json")
  : "playwright-core/package.json").version;
const mode = process.argv[2];
assert(["26", "07", "safe"].includes(mode), "usage: node scripts/capture-nodered.mjs 26|07|safe http://127.0.0.1:PORT");
const base = new URL(process.argv[3]);
assert.equal(base.hostname, "127.0.0.1");
const files = mode !== "07" ? ["02_message_path", "03_core_nodes", "04_mqtt_tls_pubsub", "05_json_validation", "06_flowfuse_dashboard"] : ["07_safe_control"];
const expected = files.flatMap((file) => JSON.parse(readFileSync(`nodered/flows/${file}.json`, "utf8")));
const runtime = await (await fetch(new URL("flows", base))).json();
assert.deepEqual(runtime, expected, "refuse to capture another Runtime or locally configured private Flow");
const browser = await chromium.launch({
  ...(process.env.CHROME_EXECUTABLE_PATH ? { executablePath: process.env.CHROME_EXECUTABLE_PATH } : {}),
  headless: true,
});
const context = await browser.newContext({ viewport: { width: 1600, height: 1000 }, locale: "zh-TW", colorScheme: "light" });
const page = await context.newPage();
const out = resolve("docs/assets/part4/captures");
const captures = [];
async function capture(filename, detail) {
  await page.mouse.move(800, 20);
  await page.screenshot({ path: `${out}/${filename}`, fullPage: true });
  captures.push({ filename, detail, at: new Date().toISOString() });
}
async function select(label) {
  await page.locator(".red-ui-tab-label").filter({ hasText: label }).first().click();
  await page.locator("#red-ui-view-zoom-fit").click();
  await page.waitForTimeout(200);
}
async function trigger(id) {
  const response = await page.request.post(new URL(`inject/${id}`, base).href);
  assert.equal(response.status(), 200);
  await page.waitForTimeout(350);
}
async function expandDebug() {
  for (let index = 0; index < 8; index++) {
    const collapsed = page.locator(".red-ui-debug-msg-element.collapsed > .red-ui-debug-msg-row > .red-ui-debug-msg-object-handle:visible");
    if (!await collapsed.count()) break;
    await collapsed.first().click();
  }
}
try {
  await page.goto(base.href, { waitUntil: "networkidle" });
  await page.locator("#red-ui-header-button-deploy").waitFor({ state: "visible" });
  const close = page.getByRole("button", { name: /^(關閉|Close)$/ });
  if (await close.count()) await close.last().click();
  else await page.keyboard.press("Escape");
  if (mode === "safe") {
    await select("04-03 基礎節點");
    await capture("node-red-5-safe-mode.png", "CLI --safe: editor loaded without starting any Flow");
  } else if (mode === "26") {
    for (const [label, filename] of [
      ["04-02 Flow 與 Message", "node-red-5-editor-message-flow.png"],
      ["04-03 基礎節點", "node-red-5-core-nodes-flow.png"],
      ["04-04 MQTT TLS 發布與訂閱", "node-red-5-mqtt-tls-flow.png"],
      ["04-05 JSON 驗證", "node-red-5-json-validation-flow.png"],
      ["04-06 FlowFuse Dashboard", "node-red-5-dashboard-flow.png"],
    ]) {
      await select(label);
      await capture(filename, `${label}: unchanged public Flow in the editor`);
    }
    await page.locator("#red-ui-sidebar-debug-open").click();
    await select("04-02 Flow 與 Message");
    await trigger("4020000000000003");
    // Expand the actual object in Debug; never replace the UI with generated text.
    await expandDebug();
    await capture("node-red-5-message-debug.png", "lesson 2 actual DEMO message in Debug");
    await select("04-03 基礎節點");
    await page.locator("#red-ui-sidebar-debug-clear").click();
    await trigger("4030000000000003");
    await trigger("4030000000000004");
    await trigger("403000000000000c");
    await expandDebug();
    await capture("node-red-5-core-debug.png", "lesson 3 valid, wrong-type and range-error DEMO outputs");
    await select("04-05 JSON 驗證");
    await page.locator("#red-ui-sidebar-debug-clear").click();
    await trigger("4050000000000003");
    await trigger("4050000000000004");
    await trigger("4050000000000005");
    await capture("node-red-5-json-debug.png", "lesson 5 valid, range-error and parser-error DEMO outputs");
    await select("04-03 基礎節點");
    for (const [id, name] of [["4030000000000005", "change"], ["4030000000000006", "function"], ["4030000000000007", "switch"]]) {
      await page.evaluate((id) => RED.editor.edit(RED.nodes.node(id)), id);
      await page.locator("#node-dialog-cancel:visible").last().waitFor({ state: "visible" });
      await page.waitForTimeout(300);
      await capture(`node-red-5-${name}-settings.png`, `lesson 3 actual ${name} editor dialog`);
      await page.locator("#node-dialog-cancel:visible").last().click();
      await page.waitForTimeout(400);
    }
    await page.evaluate(() => RED.editor.editConfig("", "tls-config", "40400000000000f2"));
    await page.locator("#node-config-dialog-cancel:visible").last().waitFor({ state: "visible" });
    await page.waitForTimeout(300);
    await capture("node-red-5-tls-settings.png", "public TLS config: certificate verification enabled, no private CA or credentials");
    await page.locator("#node-config-dialog-cancel:visible").last().click();
    await page.goto(new URL("dashboard/part4-demo", base).href, { waitUntil: "networkidle" });
    for (const [id, state, file] of [
      ["d600000000000003", "VALID", "flowfuse-dashboard-demo.png"],
      ["d600000000000004", "INVALID", "flowfuse-dashboard-invalid.png"],
      ["d600000000000005", "STALE", "flowfuse-dashboard-stale.png"],
    ]) {
      await trigger(id);
      await page.getByText(new RegExp(`DEMO ONLY \\| ${state} \\|`)).waitFor();
      await page.waitForTimeout(600);
      await capture(file, `lesson 6 actual ${state} DEMO; never hardware data`);
    }
  } else {
    await select("07 MQTT Safe Control");
    await capture("node-red-5-safe-control-flow.png", "lesson 7 unchanged public Flow; MQTT disabled");
    await page.goto(new URL("control-dashboard/safe-control", base).href, { waitUntil: "networkidle" });
    await trigger("d700000000000004");
    await page.getByText(/BLOCKED: named persistent/).waitFor();
    await capture("flowfuse-safe-control-locked.png", "lesson 7 missing file context and device status: WAIT/BLOCKED");
  }
  // Evidence metadata is written outside the repository; source paths are never included.
  writeFileSync(process.env.CAPTURE_METADATA || `/tmp/nodered-captures-${mode}.json`, JSON.stringify({ browser: browser.version(), playwright: playwrightVersion, viewport: "1600x1000", captures }, null, 2));
  console.log(`Captured ${captures.length} real UI images (${browser.version()}).`);
} finally {
  await browser.close();
}
