#!/usr/bin/env python3
"""Genera mbvx-iphone.html da mbvx.html: stessa engine WebGL2, interfaccia a schede per telefono,
modalità Live a schermo pieno, microfono dell'iPhone come ingresso, preset 3D in più."""
import base64, io, re, sys
from PIL import Image, ImageDraw

SRC, DST = sys.argv[1], sys.argv[2]
PRESET_DIR = sys.argv[3]
s = open(SRC, encoding='utf-8').read()

def must(old, new, count=1):
    global s
    assert s.count(old) >= 1, old[:60]
    s = s.replace(old, new, count)

# ---- icona per la schermata Home (PNG 180x180)
im = Image.new('RGBA', (180, 180), (6, 7, 9, 255))
d = ImageDraw.Draw(im)
d.ellipse((42, 42, 138, 138), outline=(255, 181, 71, 255), width=11)
d.ellipse((77, 77, 103, 103), fill=(255, 181, 71, 255))
buf = io.BytesIO(); im.save(buf, 'PNG'); icon = base64.b64encode(buf.getvalue()).decode()

# ---- head
must('<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">',
     '<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">\n'
     '<meta name="apple-mobile-web-app-capable" content="yes">\n'
     '<meta name="mobile-web-app-capable" content="yes">\n'
     '<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">\n'
     '<meta name="apple-mobile-web-app-title" content="MBVX">\n'
     '<meta name="theme-color" content="#060709">\n'
     f'<link rel="apple-touch-icon" href="data:image/png;base64,{icon}">')
must('<title>MBVX — Visual Mixer</title>', '<title>MBVX per iPhone</title>')
must('<div class="brand"><b>MBVX</b><span>visual mixer · djay Pro</span></div>',
     '<div class="brand"><b>MBVX</b><span>iPhone</span></div>')
must('<button id="fsBtn" title="Schermo intero (F)">Proietta</button>', '<button id="fsBtn" class="live-btn">Live</button>')
must('<option value="">Ingresso audio predefinito</option>', '<option value="">Microfono</option>')
must('<button id="audioBtn">Avvia audio</button>', '<button id="audioBtn" class="audio-btn">Avvia microfono</button><button id="trackBtn" class="audio-btn" title="Riproduce un brano dal telefono e lo analizza">Brano</button><input id="trackIn" type="file" accept="audio/*" hidden>')
s = s.replace("$('#audioBtn').textContent = 'Ferma audio'", "$('#audioBtn').textContent = 'Ferma microfono'")
s = s.replace("$('#audioBtn').textContent = 'Avvia audio'", "$('#audioBtn').textContent = 'Avvia microfono'")

# ---- preset 3D (stesso codice del plugin, già validato in GLSL ES 3.00)
import os
entries = []
for n in range(28, 35):
    f = [x for x in os.listdir(PRESET_DIR) if x.startswith(f'{n} ')][0]
    code = open(os.path.join(PRESET_DIR, f), encoding='utf-8').read().rstrip('\n')
    assert '`' not in code and '${' not in code
    name = f[3:-5]
    entries.append(f"'{name}': `{code}`")
must('const PRESET_NAMES = Object.keys(PRESETS);',
     'Object.assign(PRESETS, {\n' + ',\n'.join(entries) + '\n});\nconst PRESET_NAMES = Object.keys(PRESETS);')

# ---- CSS per telefono
css = r'''
/* ===== iPhone ===== */
body{overflow-x:hidden;padding-bottom:calc(64px + env(safe-area-inset-bottom,0px))}
body::after{display:none}
.top{gap:8px 12px;padding:10px max(12px,env(safe-area-inset-right,0px)) 10px max(12px,env(safe-area-inset-left,0px));padding-top:max(10px,env(safe-area-inset-top,0px))}
.top .group{gap:6px}
.top .brand span{display:none}
.top .spacer{display:none}
#linkBtn,#midiBtn,#learnBtn,#outBtn,#qualSel,#resSel,#half,#dbl,#nudgeL,#nudgeR,.top label.ctl{display:none!important}
.top button,.top select{min-height:38px;padding:6px 12px;font-size:13px}
.live-btn{background:rgba(255,181,71,0.18)!important;border-color:rgba(255,181,71,0.5)!important;color:var(--amber)!important;font-weight:700}
.audio-btn.on{background:rgba(0,212,255,0.18)!important;border-color:rgba(0,212,255,0.5)!important}
#audioIn{max-width:150px}
.stage{display:flex;flex-direction:column;padding:10px max(10px,env(safe-area-inset-right,0px)) 10px max(10px,env(safe-area-inset-left,0px))}
.panel{padding:12px;border-radius:14px;margin-bottom:12px}
.master{order:-1}
.deck .thumb{width:46%;margin:0 auto;border-radius:8px}
body[data-tab="a"] #deck1,body[data-tab="a"] .master>:not(.canvas-wrap),
body[data-tab="b"] #deck0,body[data-tab="b"] .master>:not(.canvas-wrap),
body[data-tab="mix"] .deck{display:none}
body[data-tab="a"] .master,body[data-tab="b"] .master{padding:0;background:none;border:0;box-shadow:none}
.canvas-wrap{cursor:default;border-radius:10px}
@media (orientation:landscape){.canvas-wrap{max-height:48vh;aspect-ratio:auto;height:48vh}}
.fs-overlay{display:none!important}
.fx{grid-template-columns:1fr 1fr;gap:8px 12px}
.knobs{gap:8px 12px}
input[type=range]{height:34px}
input[type=range]::-webkit-slider-runnable-track{height:6px}
input[type=range]::-webkit-slider-thumb{width:26px;height:26px;margin-top:-10px}
input[type=range]:hover::-webkit-slider-thumb{transform:none}
.palette-bar{flex-wrap:wrap}
.palette-bar #palette{display:none}
.editor-wrap{display:none}
.editor-wrap.open{display:flex}
.deck .media-row{flex-wrap:wrap}
.tabs{position:fixed;left:0;right:0;bottom:0;z-index:50;display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;
  padding:8px max(10px,env(safe-area-inset-right,0px)) calc(8px + env(safe-area-inset-bottom,0px)) max(10px,env(safe-area-inset-left,0px));
  background:rgba(10,12,16,0.97);border-top:1px solid var(--line)}
.tabs button{min-height:44px;font-weight:700;letter-spacing:.06em;font-size:13px;border-radius:10px}
.tabs button[data-tab="a"].on{color:var(--deck-a);border-color:var(--deck-a);background:rgba(0,212,255,0.12)}
.tabs button[data-tab="mix"].on{color:var(--amber);border-color:var(--amber);background:rgba(255,181,71,0.12)}
.tabs button[data-tab="b"].on{color:var(--deck-b);border-color:var(--deck-b);background:rgba(255,84,112,0.12)}
.edit-toggle{width:100%}
#toast{bottom:calc(84px + env(safe-area-inset-bottom,0px))}
.help{margin:0 10px 12px;padding:10px 14px}
#bpm{font-size:22px;width:74px}
.top .meter{width:56px}
.top .brand b{font-size:18px}
/* modalità Live: solo l'immagine, a tutto schermo; un tocco riporta i controlli */
body.live{padding:0;overflow:hidden;background:#000}
body.live .top,body.live .tabs,body.live .deck,body.live .help,body.live .master>:not(.canvas-wrap),body.live #toast{display:none!important}
body.live .stage{padding:0}
body.live .master{padding:0;margin:0;border:0;background:none;box-shadow:none}
body.live .canvas-wrap{position:fixed;inset:0;border:0;border-radius:0;aspect-ratio:auto;height:auto;max-height:none;box-shadow:none;z-index:40}
body.live #out{width:100%;height:100%}
body.live::before{display:none}
.live-hint{position:fixed;left:50%;top:max(14px,env(safe-area-inset-top,0px));transform:translateX(-50%);z-index:60;font-size:12px;color:rgba(255,255,255,0.7);
  background:rgba(0,0,0,0.5);padding:6px 12px;border-radius:999px;pointer-events:none;opacity:0;transition:opacity .4s}
body.live .live-hint.show{opacity:1}
'''
must('</style>', css + '</style>')

# ---- aiuto per il telefono
a = s.index('<details class="help">'); b = s.index('</details>', a) + len('</details>')
s = s[:a] + """<details class="help">
  <summary>Come usare MBVX sull'iPhone</summary>
  <div class="help-grid">
    <div><b>Audio.</b> <i>Avvia microfono</i>: i visual seguono quello che sente il microfono (casse, sala); Safari lo concede solo alle pagine https. <i>Brano</i>: riproduce un file audio del telefono e lo analizza (funziona ovunque). Il BPM viene rilevato da solo (sorgente AUDIO); se la musica è bassa usa <i>Tap</i> e <i>1</i> sul primo battito.</div>
    <div><b>Schermo pieno.</b> <i>Live</i> toglie i controlli; un tocco li riporta. Gira il telefono in orizzontale. Per il proiettore o la TV: adattatore HDMI (Lightning/USB-C) o AirPlay con duplicazione schermo.</div>
    <div><b>Senza barra di Safari.</b> Condividi → <i>Aggiungi alla schermata Home</i>: la pagina si apre come un'app a schermo intero e lo schermo non si spegne mentre è attiva.</div>
    <div><b>Controlli.</b> Schede in basso per deck A, mix e deck B. Doppio tocco su un cursore = valore predefinito. <i>A</i>/<i>B</i> avviano la transizione nel tempo impostato. Le palette 1-bit e Mono con i preset 3D danno l'effetto pixel/monocromatico.</div>
    <div><b>Prestazioni.</b> La qualità dei deck si regola da sola in base agli fps (parte dal 50%). Preset più leggeri: Tunnel Wireframe, Ipercubo, Solidi, Onda Circolare, Oscilloscopio.</div>
    <div><b>Note.</b> MIDI, Ableton Link e la finestra d'uscita separata non esistono su iPhone. Se usi djay sullo stesso telefono, l'audio di djay resta sulle casse ma il microfono attivo può abbassarne il volume: meglio un secondo dispositivo per la musica.</div>
  </div>
</details>""" + s[b:]

# ---- barra delle schede e suggerimento Live
must('<div id="toast" role="status"></div>',
     '<nav class="tabs" aria-label="Sezioni"><button data-tab="a">DECK A</button><button data-tab="mix" class="on">MIX</button><button data-tab="b">DECK B</button></nav>\n'
     '<div class="live-hint" id="liveHint">Tocca per tornare ai controlli</div>\n'
     '<div id="toast" role="status"></div>')

# ---- editor GLSL nascosto dietro un pulsante
must('''  <div class="editor-wrap">
    <div class="editor-bar"><span class="lbl">Shader GLSL</span>''',
     '''  <button class="edit-toggle" id="${p}-editToggle">Shader GLSL…</button>
  <div class="editor-wrap">
    <div class="editor-bar"><span class="lbl">Shader GLSL</span>''')

# ---- avvio audio dentro il gesto (iOS vuole l'AudioContext creato/ripreso nel tocco)
must("$('#audioBtn').onclick = () => audio.running ? audio.stop() : audio.start($('#audioIn').value);",
     "$('#audioBtn').onclick = () => { if (audio.running) return audio.stop();\n"
     "    audio.ctx = audio.ctx || new (window.AudioContext || window.webkitAudioContext)({ latencyHint: 'interactive' });\n"
     "    audio.ctx.resume(); keepAwake(); audio.start($('#audioIn').value); };")
must("$('#fsBtn').onclick = toggleFullscreen;", "$('#fsBtn').onclick = toggleLive;")
must("$('#fsOverlay').onclick = toggleFullscreen;", "")
must("canvasWrap.addEventListener('dblclick', e => { if (e.target.closest('.fs-overlay')) return; toggleFullscreen(); });",
     "canvasWrap.addEventListener('click', () => { if (document.body.classList.contains('live')) toggleLive(); });")
must('setResolution(1280, 720);\n', "setResolution(960, 540); $('#resSel').value = '960x540'; setQualityScale(0.5);\n")

# ---- JS per telefono
js = r'''
// ===== iPhone =====
let wakeLock = null;
// Brano dal telefono: riprodotto dalla pagina e analizzato come il microfono (serve dove il microfono non è concesso)
audio.trackEl = null;
audio.startFile = async function (file) {
  this.stop();
  this.ctx = this.ctx || new (window.AudioContext || window.webkitAudioContext)({ latencyHint: 'interactive' });
  await this.ctx.resume();
  const el = new Audio(); el.src = URL.createObjectURL(file); el.loop = true; el.playsInline = true; el.setAttribute('playsinline', '');
  this.trackEl = el;
  this.src = this.ctx.createMediaElementSource(el);
  this.an = this.ctx.createAnalyser(); this.an.fftSize = 2048; this.an.smoothingTimeConstant = 0.2;
  this.src.connect(this.an); this.src.connect(this.ctx.destination);
  this.binHz = this.ctx.sampleRate / 2048;
  this.running = true; this.onsets = []; this.hist = [];
  await this.startWorklet();
  if (clock.auto && clock.source !== 'LINK') clock.source = 'AUDIO';
  try { await el.play(); } catch (e) { toast('Riproduzione non consentita: tocca di nuovo Brano.', 5000); this.stop(); return; }
  $('#audioBtn').textContent = 'Ferma'; $('#audioBtn').classList.add('on'); $('#trackBtn').classList.add('on');
  toast('Brano in riproduzione: ' + file.name);
};
const _stop = audio.stop.bind(audio);
audio.stop = function () { if (this.trackEl) { try { this.trackEl.pause(); URL.revokeObjectURL(this.trackEl.src); } catch (e) {} this.trackEl = null; } $('#trackBtn').classList.remove('on'); _stop(); };
$('#trackBtn').onclick = () => { if (audio.trackEl) return audio.stop(); audio.ctx = audio.ctx || new (window.AudioContext || window.webkitAudioContext)({ latencyHint: 'interactive' }); audio.ctx.resume(); $('#trackIn').click(); };
$('#trackIn').onchange = e => { const f = e.target.files[0]; e.target.value = ''; if (f) { keepAwake(); audio.startFile(f); } };
async function keepAwake() {
  try { if (navigator.wakeLock && !wakeLock) { wakeLock = await navigator.wakeLock.request('screen'); wakeLock.addEventListener('release', () => { wakeLock = null; }); } } catch (e) {}
}
document.addEventListener('visibilitychange', () => { if (document.visibilityState === 'visible' && (audio.running || document.body.classList.contains('live'))) keepAwake(); });
function toggleLive() {
  const on = !document.body.classList.contains('live');
  document.body.classList.toggle('live', on);
  if (on) { keepAwake(); const h = $('#liveHint'); h.classList.add('show'); setTimeout(() => h.classList.remove('show'), 2500); }
  window.dispatchEvent(new Event('resize'));
}
toggleFullscreen = toggleLive;
function setTab(t) { document.body.dataset.tab = t; $$('.tabs button').forEach(b => b.classList.toggle('on', b.dataset.tab === t)); LS.set('tab', t); window.scrollTo(0, 0); }
$$('.tabs button').forEach(b => b.onclick = () => setTab(b.dataset.tab));
setTab(LS.get('tab', 'mix'));
[0, 1].forEach(i => { const b = $(`#d${i}-editToggle`); if (b) b.onclick = () => { const w = b.nextElementSibling; w.classList.toggle('open'); b.textContent = w.classList.contains('open') ? 'Nascondi shader' : 'Shader GLSL…'; }; });
// doppio tocco su un cursore = valore predefinito (come il doppio clic sul computer)
let lastTapEl = null, lastTapT = 0;
document.addEventListener('touchend', e => {
  const el = e.target;
  if (!(el instanceof HTMLInputElement) || el.type !== 'range' || el.hasAttribute('data-noreset')) return;
  const now = performance.now();
  if (lastTapEl === el && now - lastTapT < 320) { el.value = el.defaultValue; el.dispatchEvent(new Event('input', { bubbles: true })); lastTapT = 0; return; }
  lastTapEl = el; lastTapT = now;
}, { passive: true });
// niente zoom con il doppio tocco sui controlli
document.addEventListener('dblclick', e => { if (e.target.closest('.top,.tabs,.panel')) e.preventDefault(); }, { passive: false });
// scorciatoie da tastiera esterna restano; in Live il tasto Esc esce
document.addEventListener('keydown', e => { if (e.key === 'Escape' && document.body.classList.contains('live')) toggleLive(); });
if (!audio.running) setTimeout(() => toast('Tocca "Avvia microfono" (o "Brano" per un file audio), poi "Live" per lo schermo pieno. Aggiungi la pagina alla schermata Home per usarla senza la barra di Safari.', 7000), 800);
'''
idx = s.rfind('</script>')
s = s[:idx] + js + '\n' + s[idx:]
open(DST, 'w', encoding='utf-8').write(s)
print('ok', len(s))
