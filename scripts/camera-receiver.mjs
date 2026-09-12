// Node.js 24; reads mosquitto_sub -F '%j' envelopes. No network credentials here.
import { mkdirSync, writeFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { pathToFileURL } from 'node:url';
export const MAX_BYTES = 49152, CHUNK_BYTES = 384;
export function crc32(bytes) {
  let crc = 0xffffffff;
  for (const value of bytes) {
    crc ^= value;
    for (let i = 0; i < 8; i++) crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return ((~crc) >>> 0).toString(16).padStart(8, '0');
}
export class CameraReceiver {
  constructor(topic) { this.topic = topic; this.frame = null; this.recent = new Map(); }
  expire(now) {
    for (const [id, time] of this.recent) if (now - time > 60000) this.recent.delete(id);
    if (this.frame && now - this.frame.started > 30000) { this.frame = null; return true; }
    return false;
  }
  accept(envelope, now = Date.now()) {
    this.expire(now);
    const fail = () => { this.frame = null; throw new Error('FRAME REJECTED'); };
    if (!envelope || envelope.topic !== this.topic || envelope.retain !== 0 || typeof envelope.payload !== 'string' || Buffer.byteLength(envelope.payload) > 850) return fail();
    let message; try { message = JSON.parse(envelope.payload); } catch { return fail(); }
    if (!message || message.v !== 1 || typeof message.id !== 'string' || !/^[a-f0-9]{32}-[1-9][0-9]{0,9}$/.test(message.id)) return fail();
    if (this.recent.has(message.id)) return null; // QoS duplicate after completion.
    if (message.kind === 'begin') {
      if (!Number.isInteger(message.bytes) || message.bytes < 4 || message.bytes > MAX_BYTES ||
          message.chunks !== Math.ceil(message.bytes / CHUNK_BYTES) || message.mime !== 'image/jpeg' ||
          typeof message.crc32 !== 'string' || !/^[a-f0-9]{8}$/.test(message.crc32)) return fail();
      if (this.frame?.id === message.id) {
        if (this.frame.bytes !== message.bytes || this.frame.crc32 !== message.crc32) return fail();
        return null; // Duplicate begin must not reset the timeout or erase chunks.
      }
      this.frame = { ...message, started: now, parts: [], received: 0 };
      return null;
    }
    const frame = this.frame;
    if (!frame || frame.id !== message.id) return fail();
    if (message.kind === 'chunk') {
      if (!Number.isInteger(message.index) || message.index < 0 || message.index >= frame.chunks ||
          typeof message.data !== 'string' || message.data.length > 512 || !message.data.length ||
          !/^[A-Za-z0-9+/]*={0,2}$/.test(message.data)) return fail();
      const bytes = Buffer.from(message.data, 'base64');
      if (bytes.toString('base64') !== message.data) return fail();
      const expected = Math.min(CHUNK_BYTES, frame.bytes - message.index * CHUNK_BYTES);
      if (bytes.length !== expected) return fail();
      if (message.index < frame.parts.length) {
        if (!frame.parts[message.index].equals(bytes)) return fail();
        return null;
      }
      if (message.index !== frame.parts.length) return fail();
      frame.parts.push(bytes); frame.received += bytes.length;
      return null;
    }
    if (message.kind !== 'end' || message.crc32 !== frame.crc32 || frame.parts.length !== frame.chunks || frame.received !== frame.bytes) return fail();
    const bytes = Buffer.concat(frame.parts);
    if (crc32(bytes) !== frame.crc32 || bytes[0] !== 0xff || bytes[1] !== 0xd8 || bytes.at(-2) !== 0xff || bytes.at(-1) !== 0xd9) return fail();
    this.frame = null; this.recent.set(frame.id, now);
    while (this.recent.size > 16) this.recent.delete(this.recent.keys().next().value);
    return { id: frame.id, bytes, crc32: frame.crc32 };
  }
}
if (process.argv[1] && import.meta.url === pathToFileURL(resolve(process.argv[1])).href) {
  const [topic, output] = process.argv.slice(2);
  if (!topic || !output || /[+#]/.test(topic)) {
    console.error('Usage: node scripts/camera-receiver.mjs EXACT_TOPIC OUTPUT_DIRECTORY'); process.exit(2);
  }
  const receiver = new CameraReceiver(topic);
  mkdirSync(output, { recursive: true, mode: 0o700 });
  let pending = '', dropping = false, saved = 0;
  const timer = setInterval(() => { if (receiver.expire(Date.now())) console.error('FRAME TIMEOUT / DROPPED'); }, 1000);
  process.stdin.setEncoding('utf8');
  process.stdin.on('data', chunk => {
    // Cap per-line buffering even if a malformed publisher never sends a newline.
    for (const char of chunk) {
      if (char === '\n') {
        if (!dropping && pending) {
          try {
            const result = receiver.accept(JSON.parse(pending));
            if (result) {
              writeFileSync(join(output, `${result.id}.jpg`), result.bytes, { flag: 'wx', mode: 0o600 });
              console.log(`JPEG ASSEMBLED ${++saved} / ${result.bytes.length} bytes / CRC ${result.crc32}`);
            }
          } catch { receiver.frame = null; console.error('FRAME/WRITE REJECTED'); }
        }
        pending = ''; dropping = false;
      } else if (!dropping) {
        pending += char;
        if (pending.length > 2048) { dropping = true; pending = ''; receiver.frame = null; console.error('LINE TOO LARGE'); }
      }
    }
  });
  process.stdin.on('end', () => {
    clearInterval(timer);
    if (receiver.frame || pending || dropping) console.error('INCOMPLETE INPUT / DROPPED');
    if (!saved) process.exitCode = 1;
  });
}
