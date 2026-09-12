import assert from 'node:assert/strict';
import { CameraReceiver, crc32 } from './camera-receiver.mjs';
const topic = 'course/demo/image', id = 'a'.repeat(32) + '-1';
// Synthetic byte contract fixture, NOT a real camera JPEG or decoder test.
const bytes = Buffer.alloc(900, 0x41); bytes[0] = 255; bytes[1] = 216; bytes[898] = 255; bytes[899] = 217;
const sum = crc32(bytes);
assert.equal(crc32(Buffer.from('123456789')), 'cbf43926');
const begin = { v:1, kind:'begin', id, bytes:900, chunks:3, crc32:sum, mime:'image/jpeg' };
const chunks = [0, 1, 2].map(index => ({ v:1, kind:'chunk', id, index, data:bytes.subarray(index*384, (index+1)*384).toString('base64') }));
const end = { v:1, kind:'end', id, crc32:sum };
const envelope = p => ({ topic, retain:0, payload:JSON.stringify(p) });
const send = (r,p,t=0) => r.accept(envelope(p),t);
let r = new CameraReceiver(topic);
send(r,begin); send(r,chunks[0]); send(r,chunks[0]); send(r,begin); send(r,chunks[1]); send(r,chunks[2]);
assert.deepEqual(send(r,end).bytes,bytes); assert.equal(send(r,end),null);
for (const bad of [ {...begin, bytes:49153}, {...begin, chunks:4}, {...begin, bytes:-1}, {...begin, id:'../../bad'}, {...begin, mime:'text/html'} ]) {
  assert.throws(() => send(new CameraReceiver(topic),bad));
}
for (const mutate of [() => end, () => chunks[1], () => ({...chunks[0],data:'%%%'}), () => ({...chunks[0],data:'AA=='}), () => ({...chunks[0],index:-1})]) {
  r = new CameraReceiver(topic); send(r,begin); assert.throws(() => send(r,mutate())); assert.equal(r.frame,null);
}
r = new CameraReceiver(topic); send(r,begin); send(r,chunks[0]);
assert.throws(() => send(r,{...chunks[0],data:Buffer.alloc(384).toString('base64')}));
r = new CameraReceiver(topic); send(r,begin); assert.throws(() => send(r,chunks[0],30001));
r = new CameraReceiver(topic); send(r,begin); assert.equal(r.expire(30001),true);
r = new CameraReceiver(topic); send(r,begin); for (const p of chunks) send(r,p); assert.throws(() => send(r,{...end,crc32:'00000000'}));
r = new CameraReceiver(topic); send(r,{...begin,crc32:'00000000'}); for (const p of chunks) send(r,p); assert.throws(() => send(r,{...end,crc32:'00000000'}));
for (const bad of [{...envelope(begin),retain:1}, {...envelope(begin),topic:'wrong'}, {...envelope(begin),payload:'{bad'}, {...envelope(begin),payload:'x'.repeat(851)}]) assert.throws(() => new CameraReceiver(topic).accept(bad));
console.log('Camera receiver contracts passed: bounded size, order, duplicates, timeout, retain, checksum and completion');
// Exercise the actual CLI stream and file-writing path in an isolated directory.
const { mkdtempSync, readdirSync, readFileSync, rmSync } = await import('node:fs');
const { tmpdir } = await import('node:os');
const { join } = await import('node:path');
const { spawnSync } = await import('node:child_process');
const { fileURLToPath } = await import('node:url');
const output = mkdtempSync(join(tmpdir(), 'camera-receiver-test-'));
try {
  const program = fileURLToPath(new URL('./camera-receiver.mjs', import.meta.url));
  let result = spawnSync(process.execPath, [program, topic, output], {
    input: [begin, ...chunks, end].map(p => JSON.stringify(envelope(p))).join('\n') + '\n', encoding:'utf8'
  });
  assert.equal(result.status,0); assert.match(result.stdout,/JPEG ASSEMBLED 1/);
  assert.deepEqual(readFileSync(join(output, `${id}.jpg`)),bytes);
  assert.equal(readdirSync(output).length,1);
  result = spawnSync(process.execPath, [program, topic, output], { input:'x'.repeat(4096)+'\n', encoding:'utf8' });
  assert.equal(result.status,1); assert.match(result.stderr,/LINE TOO LARGE/);
  assert.equal(readdirSync(output).length,1);
  result = spawnSync(process.execPath, [program, topic, output], { input:JSON.stringify(envelope(begin))+'\n', encoding:'utf8' });
  assert.equal(result.status,1); assert.match(result.stderr,/INCOMPLETE INPUT/);
  assert.equal(readdirSync(output).length,1);
  console.log('Camera receiver CLI passed: isolated output, overflow rejection and incomplete-frame discard');
} finally { rmSync(output,{recursive:true,force:true}); }
