# MBStudioUI per pagine web

`mbstudio-hw.css` contiene lo stile hardware MBStudio per HTML, con font e logo incorporati (funziona offline).

1. Copia il contenuto di `mbstudio-hw.css` in un `<style>` della pagina (o collegalo con `<link>`).
2. Metti `class="mbs"` sul `<body>`.
3. Marchio in alto: `<span class="mbs-brand"><i class="mbs-logo"></i><span class="mbs-badge">NOME</span></span>`
4. Logo impresso in basso: `<span class="mbs-deboss"><i class="mbs-logo"></i></span>`
5. Altri elementi: `.mbs-chip`, `.mbs-recess`, `.mbs-led.on` (`.blue`, `.amber`, `.red`), `.mbs-engraved`, `.mbs-num`;
   pulsanti, select, campi e slider vengono stilizzati da soli; `button.on` accende il LED verde.

Per rigenerare il CSS: `python3 make_hw_css.py .` dentro questa cartella (servono i file `.woff2` e `wordmark-520.png`).
Font: Bebas Neue (© Dharma Type) e Antonio (© Vernon Adams), SIL Open Font License 1.1 (vedi `../fonts/OFL-*.txt`), ridotti ai caratteri latini.
Usato in: StemClean DNR, MBStudio91 DAW.
