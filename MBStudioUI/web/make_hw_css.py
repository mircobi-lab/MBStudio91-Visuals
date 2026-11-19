#!/usr/bin/env python3
"""Genera mbstudio-hw.css: stile hardware MBStudio per pagine web, con font e logo incorporati (funziona offline).

Uso: python3 make_hw_css.py CARTELLA_CON_WOFF2_E_LOGO > mbstudio-hw.css
Font: Bebas Neue (Dharma Type) e Antonio (Vernon Adams), SIL Open Font License 1.1.
"""
import base64, pathlib, sys

d = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.')
b64 = lambda f: base64.b64encode((d / f).read_bytes()).decode()

faces = [('Bebas Neue', 'BebasNeue-Regular.woff2', 400),
         ('Antonio', 'Antonio-Thin.woff2', 100),
         ('Antonio', 'Antonio-Light.woff2', 300),
         ('Antonio', 'Antonio-Bold.woff2', 700)]

out = ['/* MBStudio hardware UI - stile web condiviso (generato da make_hw_css.py).',
       '   Font: Bebas Neue (c) Dharma Type, Antonio (c) Vernon Adams - SIL Open Font License 1.1.',
       '   Logo MBSTUDIO91: marchio di MBStudio91. */']
for fam, f, w in faces:
    out.append("@font-face{font-family:'%s';font-weight:%d;font-style:normal;font-display:swap;"
               "src:url(data:font/woff2;base64,%s) format('woff2')}" % (fam, w, b64(f)))

out.append(''':root{
  --chassis-top:#151617;--chassis-bottom:#0a0a0b;--recess:#050506;--recess-edge:#26282b;
  --key:#121314;--key-top:#1e1f21;--key-edge:#34363a;--engrave:#1f2023;
  --print:#f3f3f1;--print-dim:#9b9da2;--tick:#b9bbbf;--chip:#f3f3f1;--chip-text:#0b0b0c;
  --led-blue:#3d8bff;--led-green:#3ee05c;--led-amber:#ffb020;--led-red:#ff3b30;--led-off:#1b1d1f;
  --f-display:'Bebas Neue',Impact,sans-serif;--f-label:'Antonio','Arial Narrow',sans-serif;
  --mbs-logo:url(data:image/png;base64,%s);
}''' % b64('wordmark-520.png'))

out.append('''
/* telaio */
html:has(body.mbs){background:var(--chassis-bottom)}
body.mbs{min-height:100vh;background:linear-gradient(var(--chassis-top),var(--chassis-bottom) 600px) var(--chassis-bottom);color:var(--print);
  font:300 15px/1.45 var(--f-label);letter-spacing:.03em;-webkit-font-smoothing:antialiased}
.mbs-print{font:300 13px/1.2 var(--f-label);letter-spacing:.08em;text-transform:uppercase;color:var(--print)}
.mbs-dim{color:var(--print-dim)}
.mbs-num{font-family:var(--f-label);font-weight:100;font-variant-numeric:tabular-nums;letter-spacing:.02em}
/* marchio: logo stampato + badge prodotto */
.mbs-brand{display:inline-flex;align-items:center;gap:12px;vertical-align:middle}
.mbs-logo{display:inline-block;height:26px;aspect-ratio:2071/246;background:var(--print);opacity:.96;
  -webkit-mask:var(--mbs-logo) center/contain no-repeat;mask:var(--mbs-logo) center/contain no-repeat}
.mbs-badge{font:400 21px/1 var(--f-display);letter-spacing:.12em;text-transform:uppercase;color:var(--print);
  border:1.5px solid var(--print);border-radius:3px;padding:4px 8px 2px}
/* logo impresso nella plastica (deboss) */
.mbs-deboss{display:inline-block;filter:drop-shadow(0 1px 0 rgba(255,255,255,.13)) drop-shadow(0 -1px 0 rgba(0,0,0,.9))}
.mbs-deboss .mbs-logo{background:#0c0c0d;opacity:1;height:22px}
/* chip invertito */
.mbs-chip{display:inline-block;background:var(--chip);color:var(--chip-text);font:400 16px/1 var(--f-display);
  letter-spacing:.08em;text-transform:uppercase;padding:3px 7px 1px;border-radius:2px}
/* vano incassato */
.mbs-recess{background:var(--recess);border:1px solid #000;border-bottom-color:var(--recess-edge);
  border-right-color:var(--recess-edge);border-radius:3px;box-shadow:inset 0 2px 5px rgba(0,0,0,.9)}
/* linea incisa */
.mbs-engraved{border:0;border-top:1px solid #000;border-bottom:1px solid var(--engrave);height:0;margin:0}
/* LED */
.mbs-led{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--led-off);
  box-shadow:inset 0 1px 1px rgba(0,0,0,.8);vertical-align:middle;flex:none}
.mbs-led.on{background:var(--led-green);box-shadow:0 0 6px var(--led-green),0 0 2px var(--led-green)}
.mbs-led.on.blue{background:var(--led-blue);box-shadow:0 0 6px var(--led-blue)}
.mbs-led.on.amber{background:var(--led-amber);box-shadow:0 0 6px var(--led-amber)}
.mbs-led.on.red{background:var(--led-red);box-shadow:0 0 6px var(--led-red)}
/* tasti in gomma */
body.mbs button,.mbs-key{font:400 16px/1 var(--f-display);letter-spacing:.08em;text-transform:uppercase;color:var(--print);
  background:linear-gradient(var(--key-top),var(--key));border:1px solid var(--key-edge);border-bottom-color:#000;
  border-radius:3px;padding:8px 13px 6px;cursor:pointer;
  box-shadow:0 2px 0 #000,inset 0 1px 0 rgba(255,255,255,.07);transition:transform .08s,box-shadow .08s,border-color .15s}
body.mbs button:hover{border-color:#4a4d52}
body.mbs button:active{transform:translateY(1px);box-shadow:0 1px 0 #000,inset 0 1px 0 rgba(255,255,255,.05)}
body.mbs button:disabled{opacity:.35;cursor:not-allowed;transform:none}
/* tasto con LED: classe .on accende il LED verde a sinistra */
body.mbs button.led,body.mbs button.on{position:relative;padding-left:24px}
body.mbs button.led::before,body.mbs button.on::before{content:"";position:absolute;left:9px;top:50%;width:7px;height:7px;
  margin-top:-4px;border-radius:50%;background:var(--led-off);box-shadow:inset 0 1px 1px rgba(0,0,0,.8)}
body.mbs button.on::before{background:var(--led-green);box-shadow:0 0 6px var(--led-green)}
/* campi e selettori */
body.mbs select,body.mbs input[type=text],body.mbs input[type=number],body.mbs input[type=search],body.mbs textarea{
  font:300 14px/1.2 var(--f-label);letter-spacing:.04em;color:var(--print);background-color:var(--recess);
  border:1px solid #000;border-bottom-color:var(--recess-edge);border-radius:3px;box-shadow:inset 0 2px 4px rgba(0,0,0,.9);color-scheme:dark}
body.mbs select{-webkit-appearance:none;appearance:none;padding:6px 26px 5px 8px;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='9' height='6'%3E%3Cpath d='M0 0h9L4.5 6z' fill='%23f3f3f1'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 9px center}
body.mbs select option{background:#111213;color:var(--print)}
body.mbs input[type=checkbox]{accent-color:var(--led-green)}
/* fader orizzontali: scanalatura incassata, tacche, cappuccio nero con linea bianca */
body.mbs input[type=range]{-webkit-appearance:none;appearance:none;height:26px;background:transparent;cursor:pointer;
  background-image:repeating-linear-gradient(90deg,var(--tick) 0 1px,transparent 1px 10%),repeating-linear-gradient(90deg,rgba(185,187,191,.45) 0 1px,transparent 1px 2.5%);
  background-size:calc(100% - 1px) 6px,calc(100% - 1px) 3px;background-position:0 100%,0 100%;background-repeat:no-repeat;accent-color:var(--led-green)}
body.mbs input[type=range]::-webkit-slider-runnable-track{height:5px;background:var(--recess);border-radius:2px;
  border-bottom:1px solid var(--recess-edge);box-shadow:inset 0 1px 2px #000}
body.mbs input[type=range]::-moz-range-track{height:5px;background:var(--recess);border-radius:2px;box-shadow:inset 0 1px 2px #000}
body.mbs input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:12px;height:20px;margin-top:-8px;border-radius:2px;
  background:linear-gradient(90deg,#1b1c1e 0 45%,var(--print) 45% 55%,#1b1c1e 55%);border:1px solid #3a3c40;box-shadow:0 2px 3px #000}
body.mbs input[type=range]::-moz-range-thumb{width:10px;height:18px;border-radius:2px;
  background:linear-gradient(90deg,#1b1c1e 0 45%,var(--print) 45% 55%,#1b1c1e 55%);border:1px solid #3a3c40;box-shadow:0 2px 3px #000}
body.mbs kbd{font:400 13px var(--f-display);letter-spacing:.06em;background:linear-gradient(var(--key-top),var(--key));
  border:1px solid var(--key-edge);border-radius:2px;padding:1px 5px 0;color:var(--print)}
body.mbs ::selection{background:rgba(243,243,241,.25)}
body.mbs ::-webkit-scrollbar{width:9px;height:9px}
body.mbs ::-webkit-scrollbar-track{background:var(--recess)}
body.mbs ::-webkit-scrollbar-thumb{background:#2a2c30;border-radius:4px}
''')
print('\n'.join(out))
