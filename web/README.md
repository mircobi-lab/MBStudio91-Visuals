# MBVX — versione web

- `mbvx.html` — MBVX per computer (Chrome/Brave/Edge/Firefox, WebGL2): il visual mixer da cui nasce il plugin.
- `mbvx-iphone.html` — MBVX per iPhone/iPad (Safari): stessa engine, interfaccia a schede per il telefono, modalità
  **Live** a schermo pieno, sorgenti **Microfono** e **Brano** (file audio del telefono), preset 3D del plugin.
- `bridge.js` — collega MBVX a ciò che il browser da solo non raggiunge (Node.js 18+, nessuna dipendenza):
  Art-Net e sACN verso LED wall, barre, fari, strobo e QLC+; OSC; Ableton Link tramite Carabiner.
  Avvio: `node bridge.js` nella cartella di `mbvx.html`, poi apri **http://localhost:17001** (da lì il microfono e
  BlackHole funzionano in qualsiasi browser). Opzioni: `--port`, `--carabiner`, `--bind 0.0.0.0` (uso da un altro
  computer della rete).
- `make_iphone.py` — genera `mbvx-iphone.html` da `mbvx.html` e dagli shader in `../Shaders/Presets`:
  `python3 make_iphone.py mbvx.html mbvx-iphone.html ../Shaders/Presets`

## Uso su iPhone
1. Carica `mbvx-iphone.html` su un sito **https** (Safari concede il microfono solo alle pagine sicure) e aprilo.
2. *Condividi → Aggiungi alla schermata Home*: si apre come app a schermo intero, lo schermo non si spegne.
3. *Avvia microfono* (o *Brano* per un file audio), poi *Live*. Telefono in orizzontale; verso TV/proiettore con
   adattatore HDMI o AirPlay (duplicazione schermo).

## Uscita luci in MBVX (computer)

Pulsante **Luci** in alto. Gli apparecchi, i canali e le zone funzionano come nel plugin (vedi il README principale);
la patch JSON è la stessa e si passa dall'uno all'altro con *Salva patch… / Importa…*. Uscite:

- **Rete**: Art-Net o sACN tramite `bridge.js` (IP del nodo, 127.0.0.1 per QLC+ sullo stesso Mac, oppure broadcast).
- **DMX USB** direttamente da Chrome/Edge (Web Serial): Enttec DMX USB Pro e compatibili (DMXKing ecc.), oppure
  le interfacce "Open DMX" FTDI (sperimentale). Safari e Firefox non hanno Web Serial.
- **QLC+** Web API: indica gli ID dei widget della Virtual Console per beat, strobo, livello e bassi.
- **OSC** tramite il bridge.

Con **Zone sullo schermo** le zone appaiono sopra l'immagine e si trascinano (angolo in basso a destra per
ridimensionare). **Identifica** fa lampeggiare un apparecchio per trovarlo.
