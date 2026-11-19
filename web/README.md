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

## Mappe MIDI predefinite (tasto MIDI ▾)

Il tasto **MIDI** apre un elenco: in alto le mappe predefinite, in fondo le tue. Quando mappi un controller nuovo con
**Learn** (clic su un comando, poi muovi il controllo), la mappa viene salvata col nome del controller e aggiunta in
fondo all'elenco; il ✕ la elimina. Il MIDI nel browser funziona in Chrome ed Edge (Safari non ha Web MIDI).

| Mappa | Come impostare il controller | Assegnazioni |
|---|---|---|
| Launchkey Mini MK1/MK2/MK3 | MK1/MK2: InControl spento. MK3: pad in Drum (Shift + Drum) | Manopole CC 21-28: Param 1-2 deck A e B, crossfader, scia, zoom, tinta. Pad (canale 10, note 36-51): fila alta preset A, GO A, GO B, preset B, palette, caleidoscopi, fusione, strobo/beat; fila bassa tap, battito 1, strobo (tenuto), buio, inverti, specchio, 0° A e B. Frecce su/giù ×2 e ÷2; tasti tondi (MK1/MK2) GO A/B; Play (MK3) Auto BPM, Record buio |
| Launchkey 61 | Pad in Drum | Manopole CC 21-28: Param deck, scia, zoom, tinta, caleido. Fader CC 41-48 (originale/MK2): velocità, reattività e spin dei deck, transizione, master; fader master CC 7 = crossfader. Pad come il Mini. Sui MK3 i fader in modalità Custom vanno assegnati con Learn |
| Launchpad Mini MK3 | Programmer mode: tieni Session, poi Scene Launch in basso (arancione) | Fila 1 (dal basso): tap, 1, ÷2, ×2, fase ◀ ▶, Auto, buio. Fila 2: strobo, inverti, specchio, palette, caleidoscopi, fusione, strobo/beat. Fila 3: preset A, Tempo/Free A, 0° A … 0° B, Tempo/Free B, preset B. Fila 4: GO A a sinistra, GO B a destra. Tasti in alto e a destra: preset, GO, tap, effetti |
| Launchpad Mini MK1/MK2 | Layout di fabbrica | Stesse funzioni del MK3 (griglia X-Y, tasti tondi in alto CC 104-111) |
| M-Audio Keystation (tutta la serie) | Ottava a 0 | Rotella modulazione CC 1 = crossfader, slider/manopola volume CC 7 = master, pedale CC 64 = strobo. Tasti dal Do centrale (60): Do GO A, Do# inverti, Re GO B, Re# specchio, Mi preset A, Fa preset B, Fa# battito 1, Sol tap, Sol# palette, La buio, La# caleidoscopi, Si strobo |

Fonti dei messaggi MIDI: Novation, *Launchkey Mini* user guide (MIDI Messages Table, Basic mode: pad sempre sul canale 10,
tasti tondi CC 108/109); Novation, *Launchkey MK1 MIDI CC Messages*; Novation, *Launchkey MK3 user guide* (Drum mode,
note C1-D#2 sul canale 10); github.com/cowboy/LaunchkeyMiniMK3 (Play CC 115, Record CC 117, frecce); Novation,
*Launchpad Mini [MK3] Programmer's reference* (Programmer mode, note 11-88) e *Launchpad Programmer's reference*
(layout X-Y, CC 104-111); M-Audio, guide Keystation MK3 (modulazione CC 1, volume CC 7).
