# MBStudio91 Visuals

Plugin di **visualizzazione audio-reattiva** (VST3, Audio Unit e applicazione Standalone) nato da MBVX,
il visual mixer HTML/WebGL di MBStudio91. Due deck di shader, crossfader con transizione automatica,
fusioni, palette (originale, verde, blu, mono, **1-bit / 2-bit / 4-bit con dithering**, ambra, Pantone a 3
colori, **Monocolore**), **Pixel** (1-8x, pixel netti), scia, tinta, caleidoscopio, strobo, zoom sul beat,
rotazione/spin dei deck a tempo o libero, clock dai BPM della DAW oppure **rilevato dall'audio**
(stesso algoritmo di MBVX: colpi di cassa + istogramma + regressione sulla griglia), tap tempo, MIDI learn,
finestra **proiettore** a schermo intero su un secondo schermo, editor GLSL integrato.

34 preset: i 27 di MBVX più **7 preset 3D a basso costo** pensati per il bianco e nero / 1-bit / monocolore:

| # | Preset | Tecnica |
|---|--------|---------|
| 28 | 3D Solidi Wireframe | icosaedro + ottaedro a fil di ferro, vertici deformati dallo spettro |
| 29 | 3D Ipercubo | tesseratto (4D) proiettato in 3D e poi sullo schermo |
| 30 | 3D Terreno Spettrale | terreno a linee nascoste costruito dalla storia dello spettro |
| 31 | 3D Tunnel Wireframe | tunnel a griglia, sezione deformata dallo spettro (nessun ciclo nello shader) |
| 32 | 3D Raytracing Sfere | 3 sfere + pavimento a scacchi, ombre nette, un rimbalzo di riflesso, scala di grigi |
| 33 | 3D Voxel Città | città di cubi alta quanto lo spettro, attraversamento a griglia (DDA) |
| 34 | 3D Toro Raymarch | toro attorcigliato (raymarching SDF), ombreggiatura grigia con contorno |

Tutto gira in **OpenGL 3.2 core (GLSL 1.50)** con i deck calcolati a risoluzione ridotta e una qualità
automatica che scende (100 → 75 → 50 → 35 %) quando gli fps calano: va bene anche su computer poco potenti
(MacBook Pro 2019 con grafica integrata, Mac mini).

## Installazione su Mac

Scarica dalla pagina **Releases** (o dagli artefatti di GitHub Actions) i tre zip macOS, poi:

```bash
# VST3 (per Ableton Live, Reaper, Bitwig, Cubase...)
unzip MBStudio91-Visuals-macOS-VST3.zip -d ~/Library/Audio/Plug-Ins/VST3/
# Audio Unit (Logic, GarageBand, Live)
unzip MBStudio91-Visuals-macOS-AU.zip -d ~/Library/Audio/Plug-Ins/Components/
# Standalone (da usare con djay Pro + BlackHole)
unzip MBStudio91-Visuals-macOS-Standalone.zip -d ~/Applications/
```

I binari sono **universali** (Intel + Apple Silicon) e firmati solo *ad hoc* (nessun account sviluppatore
Apple), quindi macOS li mette in quarantena. Per sbloccarli, una volta sola:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"MBStudio91 Visuals.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"MBStudio91 Visuals.component"
xattr -dr com.apple.quarantine ~/Applications/"MBStudio91 Visuals.app"
```

Se preferisci compilare sul tuo Mac (Xcode o Command Line Tools + `brew install cmake`):
`./scripts/build_mac.sh` compila, firma ad hoc e installa tutto nelle cartelle dell'utente.
Live legge sia le cartelle di sistema (`/Library/Audio/Plug-Ins/...`) sia quelle dell'utente
(`~/Library/Audio/Plug-Ins/...`) [[Ableton]](https://help.ableton.com/hc/en-us/articles/209068929).

## In Ableton Live 11

1. Preferenze → Plug-Ins → attiva **VST3** (e/o **Audio Units**) e, se serve, **Rescan**.
   In Live 11.1+ la scansione VST3 avviene in background: il plugin può comparire dopo qualche istante
   [[Ableton]](https://help.ableton.com/hc/en-us/articles/4410323149074).
2. Trascina **MBStudio91 Visuals** su una traccia audio, su un return o sul **Master** (dopo gli altri
   effetti): l'audio passa inalterato, il plugin lo "ascolta" soltanto.
3. Clock = **Host (DAW)**: BPM, fase del beat e battuta arrivano da Live (anche con Link). Se Live è
   fermo il plugin continua a contare da solo.
4. **Proiettore**: scegli lo schermo e premi Proiettore (o **F**, o doppio clic sull'immagine). Esc chiude.
   Il proiettore resta acceso anche chiudendo la finestra del plugin.
5. Tutti i controlli sono **parametri automatizzabili**: si mappano con la mappatura MIDI di Live (Cmd+M)
   e si registrano come automazioni. I pulsanti *Tap*, *Primo battito*, *Vai ad A/B* sono parametri
   momentanei (si attivano portandoli a 1).

## Standalone con djay Pro e BlackHole

1. In djay Pro imposta l'uscita (o un **Multi-Output Device** creato in Configurazione MIDI Audio con
   casse + BlackHole 2ch) in modo che il mix arrivi anche a BlackHole.
2. Apri **MBStudio91 Visuals.app** → *Options → Audio/MIDI Settings*: ingresso **BlackHole 2ch**
   (al primo avvio viene scelto da solo se presente), uscita quella che vuoi (o nessuna).
   L'ingresso **non** è silenziato per impostazione predefinita, perché il plugin deve sentirlo.
3. Clock = **Audio (rilevato)** o **Host**: senza DAW il plugin passa da solo al rilevamento dall'audio.
   Il campo in alto a destra mostra la qualità della stima (battiti in griglia, scarto in ms).
4. macOS chiederà il permesso al **microfono** la prima volta (serve per qualsiasi ingresso audio).

## Tasti e controlli

Spazio tap · `.` primo battito · `Z` `X` `C` crossfader A / metà / B · `A` `B` transizione automatica
(durata "Transizione", 0-3 s) · `F` proiettore · `S` strobo (tieni premuto) · `N` buio · `I` inverti ·
`P` / Shift+`P` palette · `1`-`9` preset deck A, Shift+`1`-`9` deck B · doppio clic su un cursore =
valore predefinito · clic destro su un cursore = **assegna MIDI** (CC) · note MIDI fisse: C1 tap,
C#1 primo battito, D1/D#1 vai ad A/B, E1 strobo.

**Pixel + palette 1-bit/2-bit/4-bit/Monocolore**: il dithering (matrice di Bayer 4×4) viene calcolato sulla
griglia dei pixel grossi, quindi i pixel restano quadrati pieni. Consigliato con i preset 3D: Pixel 3-6,
palette 1-bit o Monocolore (il colore si sceglie col pulsante *Mono*).

## Shader personalizzati

*Shader...* su un deck apre l'editor: si scrive solo `mainImage(out vec4 o, in vec2 f)` (GLSL 1.50);
l'intestazione con le uniform (`iTime`, `iBass`, `iBeat`, `iPhase`, `audioSpec()`, `audioWave()`,
`specHist()`, ecc.) è fissa e uguale a quella di MBVX, quindi i preset di MBVX funzionano senza modifiche.
Ctrl/Cmd+Invio compila; gli errori riportano la riga del tuo codice. *Salva preset...* scrive un `.glsl`
in `~/Documents/MBStudio91 Visuals/Shaders/`: i file in quella cartella compaiono nell'elenco dei preset
al prossimo avvio (dopo i 34 integrati).

## Compilare

Richiede CMake ≥ 3.22 e un compilatore C++17. JUCE 9.0.5 viene scaricato automaticamente
(oppure `-DJUCE_DIR=/percorso/JUCE`).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artefatti in `build/MBStudio91Visuals_artefacts/Release/{VST3,AU,Standalone}`.
Il workflow `.github/workflows/build.yml` compila macOS (universale, firmato ad hoc), Windows e Linux e,
per i tag `v*`, pubblica una Release con gli zip.

Test eseguiti su Linux (Xvfb + Mesa llvmpipe, OpenGL 3.2 core): compilazione VST3 + Standalone,
tutti i 34 shader validati con glslangValidator (GLSL 1.50 e ES 3.00), **pluginval 1.0.4 livello 10
superato** (anche con l'editor aperto), rilevamento BPM su segnale di prova a 128 BPM: 128.00 BPM,
scarto 1,5 ms.

### Note tecniche

- **OpenGL su macOS** è deprecato da Apple dal 2018 ma ancora presente e funzionante (è lo stesso
  OpenGL che usa JUCE per tutti i plugin con grafica GL); il plugin chiede un contesto 3.2 core.
- Il rilevatore di colpi gira sul thread audio a blocchi di 128 campioni (precisione del clock audio);
  l'analizzatore replica l'`AnalyserNode` del Web Audio (FFT 2048, smoothing 0,2, -100/-30 dB,
  finestra di Blackman), così i preset reagiscono esattamente come in MBVX.
- I deck non visibili (crossfader a fondo corsa o fusione "Nessuna") vengono calcolati solo come
  anteprima a 192×108.

## Licenze

- Codice del plugin e shader: **AGPL-3.0** (file `LICENSE`).
- [JUCE 9](https://juce.com/legal/juce-9-licence/) è usato nei termini della licenza **AGPLv3**: per
  distribuire binari costruiti con JUCE in AGPL il codice sorgente deve restare pubblico (com'è qui).
  Per una distribuzione a sorgente chiuso serve invece la licenza commerciale JUCE
  (termini e livelli su juce.com/legal/juce-9-licence).
- VST3 SDK 3.8.0 (incluso in JUCE, vedi `JUCE.spdx.json`) con licenza MIT; "VST" è un marchio di Steinberg Media
  Technologies GmbH. L'uso del logo VST non è richiesto.
