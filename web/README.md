# MBVX — versione web

- `mbvx.html` — MBVX per computer (Chrome/Brave/Edge/Firefox, WebGL2): il visual mixer da cui nasce il plugin.
- `mbvx-iphone.html` — MBVX per iPhone/iPad (Safari): stessa engine, interfaccia a schede per il telefono, modalità
  **Live** a schermo pieno, sorgenti **Microfono** e **Brano** (file audio del telefono), preset 3D del plugin.
- `make_iphone.py` — genera `mbvx-iphone.html` da `mbvx.html` e dagli shader in `../Shaders/Presets`:
  `python3 make_iphone.py mbvx.html mbvx-iphone.html ../Shaders/Presets`

## Uso su iPhone
1. Carica `mbvx-iphone.html` su un sito **https** (Safari concede il microfono solo alle pagine sicure) e aprilo.
2. *Condividi → Aggiungi alla schermata Home*: si apre come app a schermo intero, lo schermo non si spegne.
3. *Avvia microfono* (o *Brano* per un file audio), poi *Live*. Telefono in orizzontale; verso TV/proiettore con
   adattatore HDMI o AirPlay (duplicazione schermo).
