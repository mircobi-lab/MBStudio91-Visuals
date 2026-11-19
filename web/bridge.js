#!/usr/bin/env node
/*  MBVX bridge — collega MBVX (nel browser) a ciò che il browser da solo non può raggiungere.
    Nessuna dipendenza: basta Node.js 18 o più recente.   Avvio:  node bridge.js

    - http://localhost:17001/          serve mbvx.html da questa cartella (pagina "sicura": il microfono funziona)
    - /link   (Server-Sent Events)     Ableton Link tramite Carabiner (TCP 127.0.0.1:17000)
    - /dmx    (WebSocket)              Art-Net (UDP 6454) e sACN / E1.31 (UDP 5568) verso LED wall, barre, fari,
                                       QLC+ o qualsiasi nodo DMX; OSC (UDP) per QLC+, Resolume, ecc.

    Opzioni:  --port 17001   --carabiner 17000   --bind 0.0.0.0 (per usarlo da un altro computer della rete)  */
'use strict';
const http = require('http'), net = require('net'), dgram = require('dgram'), crypto = require('crypto'),
      fs = require('fs'), path = require('path'), os = require('os');

const arg = (name, def) => { const i = process.argv.indexOf('--' + name); return i > 0 ? process.argv[i + 1] : def; };
const PORT = +arg('port', 17001), CARABINER = +arg('carabiner', 17000), BIND = arg('bind', '127.0.0.1');
const ROOT = __dirname;
const VERSION = '2.0';

/* ------------------------------------------------------------------ pacchetti */
const ARTNET_ID = Buffer.from('Art-Net\0', 'ascii');
let artSeq = 1;
function artDmx(universe, data) {                 // ArtDmx: ID, OpCode 0x5000 (LE), ProtVer 14, Seq, Physical, SubUni, Net, Length (BE), dati
  let len = Math.min(512, data.length); if (len & 1) len++; if (len < 2) len = 2;
  const b = Buffer.alloc(18 + len);
  ARTNET_ID.copy(b, 0); b.writeUInt16LE(0x5000, 8); b[10] = 0; b[11] = 14;
  b[12] = artSeq; b[13] = 0; b[14] = universe & 0xff; b[15] = (universe >> 8) & 0x7f; b.writeUInt16BE(len, 16);
  Buffer.from(data.buffer, data.byteOffset, Math.min(len, data.length)).copy(b, 18);
  return b;
}
const ACN_ID = Buffer.from([0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00]);
const CID = crypto.randomBytes(16);
const sacnSeq = new Map();
function sacnDmx(universe, data) {                // ANSI E1.31: root layer, framing layer, DMP layer, 512 canali
  const b = Buffer.alloc(638);
  b.writeUInt16BE(0x0010, 0); b.writeUInt16BE(0, 2); ACN_ID.copy(b, 4);
  b.writeUInt16BE(0x7000 | (638 - 16), 16); b.writeUInt32BE(0x00000004, 18); CID.copy(b, 22);
  b.writeUInt16BE(0x7000 | (638 - 38), 38); b.writeUInt32BE(0x00000002, 40);
  b.write('MBVX', 44, 'ascii'); b[108] = 100; b.writeUInt16BE(0, 109);
  const s = ((sacnSeq.get(universe) || 0) + 1) & 0xff; sacnSeq.set(universe, s); b[111] = s; b[112] = 0;
  b.writeUInt16BE(universe, 113);
  b.writeUInt16BE(0x7000 | (638 - 115), 115); b[117] = 0x02; b[118] = 0xa1; b.writeUInt16BE(0, 119); b.writeUInt16BE(1, 121);
  b.writeUInt16BE(513, 123); b[125] = 0;
  Buffer.from(data.buffer, data.byteOffset, Math.min(512, data.length)).copy(b, 126);
  return b;
}
function oscMessage(addr, type, value) {          // OSC 1.0: indirizzo, type tag, argomento (float32 / int32 big endian)
  const pad = s => { const b = Buffer.from(s + '\0', 'ascii'); return Buffer.concat([b, Buffer.alloc((4 - (b.length % 4)) % 4)]); };
  const v = Buffer.alloc(4); if (type === 'i') v.writeInt32BE(value | 0, 0); else v.writeFloatBE(+value, 0);
  return Buffer.concat([pad(addr), pad(',' + (type === 'i' ? 'i' : 'f')), v]);
}

/* ------------------------------------------------------------------ uscita UDP */
const udp = dgram.createSocket({ type: 'udp4', reuseAddr: true });
udp.on('error', e => console.error('UDP:', e.message));
udp.bind(() => { try { udp.setBroadcast(true); udp.setMulticastTTL(4); } catch (e) {} });
const out = { artnetIp: '127.0.0.1', sacnIp: '', oscHost: '127.0.0.1', oscPort: 7700 };
const stats = { packets: 0, bytes: 0, universes: new Set(), t: Date.now() };
function send(buf, port, host) { udp.send(buf, port, host, err => { if (err) stats.err = err.message; }); stats.packets++; stats.bytes += buf.length; }

function handleDmxFrame(buf) {                    // [proto u8][universe u16 BE][len u16 BE][dati]... ripetuto
  let o = 0; let any = false;
  while (o + 5 <= buf.length) {
    const proto = buf[o], uni = buf.readUInt16BE(o + 1), len = buf.readUInt16BE(o + 3); o += 5;
    const data = buf.subarray(o, o + len); o += len;
    if (proto === 1) send(artDmx(uni, data), 6454, out.artnetIp);
    else if (proto === 2) send(sacnDmx(uni, data), 5568, out.sacnIp || `239.255.${(uni >> 8) & 0xff}.${uni & 0xff}`);
    stats.universes.add((proto === 1 ? 'A' : 'S') + uni); any = true;
  }
  if (any) artSeq = artSeq >= 255 ? 1 : artSeq + 1;
}

/* ------------------------------------------------------------------ WebSocket minimo (RFC 6455) */
function wsAccept(req, socket) {
  const key = req.headers['sec-websocket-key'];
  if (!key) { socket.destroy(); return null; }
  const acc = crypto.createHash('sha1').update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
  socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + acc + '\r\n\r\n');
  socket.setNoDelay(true);
  return socket;
}
function wsSend(socket, text) {
  const p = Buffer.from(text); let h;
  if (p.length < 126) h = Buffer.from([0x81, p.length]);
  else { h = Buffer.alloc(4); h[0] = 0x81; h[1] = 126; h.writeUInt16BE(p.length, 2); }
  try { socket.write(Buffer.concat([h, p])); } catch (e) {}
}
function wsReader(socket, onMessage) {
  let acc = Buffer.alloc(0), frag = [], fragOp = 0;
  socket.on('data', chunk => {
    acc = Buffer.concat([acc, chunk]);
    for (;;) {
      if (acc.length < 2) return;
      const fin = acc[0] & 0x80, op = acc[0] & 0x0f, masked = acc[1] & 0x80;
      let len = acc[1] & 0x7f, o = 2;
      if (len === 126) { if (acc.length < 4) return; len = acc.readUInt16BE(2); o = 4; }
      else if (len === 127) { if (acc.length < 10) return; len = Number(acc.readBigUInt64BE(2)); o = 10; }
      const mo = o; if (masked) o += 4;
      if (acc.length < o + len) return;
      const payload = Buffer.from(acc.subarray(o, o + len));
      if (masked) for (let i = 0; i < len; i++) payload[i] ^= acc[mo + (i & 3)];
      acc = acc.subarray(o + len);
      if (op === 0x8) { try { socket.end(Buffer.from([0x88, 0])); } catch (e) {} return; }
      if (op === 0x9) { try { socket.write(Buffer.concat([Buffer.from([0x8a, payload.length]), payload])); } catch (e) {} continue; }
      if (op === 0xa) continue;
      if (op !== 0) { fragOp = op; frag = [payload]; } else frag.push(payload);
      if (fin) { const msg = Buffer.concat(frag); frag = []; onMessage(fragOp === 1 ? msg.toString('utf8') : msg); }
    }
  });
}

/* ------------------------------------------------------------------ Ableton Link via Carabiner */
const linkClients = new Set();
let carabiner = null, carabinerOk = false, lastStatus = null;
function connectCarabiner() {
  if (carabiner) return;
  const s = net.connect(CARABINER, '127.0.0.1');
  carabiner = s; let buf = '';
  s.on('connect', () => { carabinerOk = true; console.log('Carabiner collegato sulla porta', CARABINER); });
  s.on('data', d => {
    buf += d.toString();
    let m;
    while ((m = buf.match(/status \{([^}]*)\}/))) {
      buf = buf.slice(m.index + m[0].length);
      const f = {}; m[1].replace(/:(\w+)\s+([-\d.]+)/g, (_, k, v) => { f[k] = +v; });
      lastStatus = { type: 'status', peers: f.peers | 0, bpm: f.bpm, beat: f.beat, start: f.start };
      const line = 'data: ' + JSON.stringify(lastStatus) + '\n\n';
      linkClients.forEach(r => r.write(line));
    }
    if (buf.length > 4096) buf = '';
  });
  const drop = () => { carabiner = null; carabinerOk = false; setTimeout(() => linkClients.size && connectCarabiner(), 1000); };
  s.on('error', drop); s.on('close', drop);
}
setInterval(() => {
  if (!linkClients.size) return;
  if (!carabiner) connectCarabiner();
  if (carabinerOk) carabiner.write('status\n');
  else linkClients.forEach(r => r.write('data: {"carabiner":false}\n\n'));
}, 100);

/* ------------------------------------------------------------------ server HTTP */
const TYPES = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.json': 'application/json', '.png': 'image/png', '.css': 'text/css' };
const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://x');
  if (url.pathname === '/link') {
    res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache', 'Access-Control-Allow-Origin': '*', Connection: 'keep-alive' });
    res.write('retry: 1000\n\n');
    linkClients.add(res); connectCarabiner();
    req.on('close', () => linkClients.delete(res));
    return;
  }
  if (url.pathname === '/status') {
    res.writeHead(200, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
    res.end(JSON.stringify({ app: 'MBVX bridge', version: VERSION, carabiner: carabinerOk, out }));
    return;
  }
  let file = url.pathname === '/' ? '/mbvx.html' : decodeURIComponent(url.pathname);
  file = path.join(ROOT, path.normalize(file).replace(/^(\.\.[/\\])+/, ''));
  if (!file.startsWith(ROOT) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) { res.writeHead(404); res.end('Non trovato'); return; }
  res.writeHead(200, { 'Content-Type': TYPES[path.extname(file)] || 'application/octet-stream', 'Cache-Control': 'no-cache' });
  fs.createReadStream(file).pipe(res);
});
server.on('upgrade', (req, socket) => {
  if (!req.url.startsWith('/dmx')) { socket.destroy(); return; }
  if (!wsAccept(req, socket)) return;
  console.log('MBVX collegato all\'uscita luci');
  wsSend(socket, JSON.stringify({ type: 'hello', version: VERSION }));
  const timer = setInterval(() => {
    const dt = (Date.now() - stats.t) / 1000;
    wsSend(socket, JSON.stringify({ type: 'stats', pps: Math.round(stats.packets / dt), kbps: Math.round(stats.bytes / dt / 1024), universes: stats.universes.size, err: stats.err || null }));
    stats.packets = 0; stats.bytes = 0; stats.universes.clear(); stats.t = Date.now(); stats.err = null;
  }, 1000);
  wsReader(socket, msg => {
    if (typeof msg === 'string') {
      let m; try { m = JSON.parse(msg); } catch (e) { return; }
      if (m.type === 'cfg') {
        if (typeof m.artnetIp === 'string' && m.artnetIp) out.artnetIp = m.artnetIp;
        if (typeof m.sacnIp === 'string') out.sacnIp = m.sacnIp;
        if (typeof m.oscHost === 'string' && m.oscHost) out.oscHost = m.oscHost;
        if (m.oscPort) out.oscPort = +m.oscPort;
      } else if (m.type === 'osc' && Array.isArray(m.msgs)) {
        m.msgs.forEach(([a, t, v]) => send(oscMessage(String(a), t, v), out.oscPort, out.oscHost));
      }
    } else handleDmxFrame(msg);
  });
  const end = () => { clearInterval(timer); };
  socket.on('close', end); socket.on('error', end);
});
server.listen(PORT, BIND, () => {
  const ifs = Object.values(os.networkInterfaces()).flat().filter(i => i && i.family === 'IPv4' && !i.internal).map(i => i.address);
  console.log(`MBVX bridge ${VERSION} — apri http://localhost:${PORT}`);
  console.log('Uscita luci: Art-Net UDP 6454, sACN UDP 5568, OSC UDP. Interfacce di rete: ' + (ifs.join(', ') || 'nessuna'));
  console.log('Link: avvia Carabiner (porta ' + CARABINER + ') e premi Link in MBVX.');
});
