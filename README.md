# MBStudio91 Visuals

Plugin di **visualizzazione audio-reattiva** (VST3, Audio Unit e applicazione Standalone) nato da MBVX,
il visual mixer HTML/WebGL di MBStudio91. Due deck di shader, crossfader con transizione automatica,
fusioni, palette (originale, verde, blu, mono, **1-bit / 2-bit / 4-bit con dithering**, ambra, Pantone a 3
colori, **Monocolore**), **Pixel** (1-8x, pixel netti), scia, tinta, caleidoscopio, strobo, zoom sul beat,
rotazione/spin dei deck a tempo o libero, clock dai BPM della DAW oppure **rilevato dall'audio**
(stesso algoritmo di MBVX: colpi di cassa + istogramma + regressione sulla griglia), tap tempo, MIDI learn,
finestra **proiettore** a schermo intero su un secondo schermo, editor GLSL integrato.

37 preset: quelli di MBVX più **7 preset 3D a basso costo** pensati per il bianco e nero / 1-bit / monocolore.
Il vecchio *Neon Grid* (n. 18) è stato tolto e al suo posto c'è **Prismi Specchio**, così i numeri degli altri
preset (e le automazioni già registrate nella DAW) non cambiano.



| # | Preset | Tecnica |
|---|--------|---------|
| 28 | 3D Solidi Wireframe | icosaedro + ottaedro a fil di ferro, vertici deformati dallo spettro |
| 29 | 3D Ipercubo | tesseratto (4D) proiettato in 3D e poi sullo schermo |
| 30 | 3D Terreno Spettrale | terreno a linee nascoste costruito dalla storia dello spettro |
| 31 | 3D Tunnel Wireframe | tunnel a griglia, sezione deformata dallo spettro (nessun ciclo nello shader) |
| 32 | 3D Raytracing Sfere | 3 sfere + pavimento a scacchi, ombre nette, un rimbalzo di riflesso, scala di grigi |
| 33 | 3D Voxel Città | città di cubi alta quanto lo spettro, attraversamento a griglia (DDA) |
| 34 | 3D Toro Raymarch | toro attorcigliato (raymarching SDF), ombreggiatura grigia con contorno |
| 18 | Prismi Specchio | corridoio di blocchi a fil di ferro, simmetrico, con fasci di luce dal fondo (Param2 = colore d'accento) |
| 35 | Fili di Luce | fasci di fili bianchi che scorrono in diagonale e si piegano, qualche anello largo |
| 36 | Mosaico Barre | colonne di barre arrotondate a colori che scorrono, accese dallo spettro |
| 37 | Citta dall'Alto | volo sopra una città notturna (DDA con strade), finestre accese dalla musica |

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
al prossimo avvio (dopo i 37 integrati).

Il **Lab** di MBVX (pulsante *Lab*) crea preset nuovi partendo da una foto, un video o una descrizione:
*Esporta .glsl* scarica il file da mettere in quella cartella e il preset compare anche nel plugin.

## Caleidoscopi 1 / 2 / 4 / 8

**Caleidoscopi** (pannello MIX) divide l'immagine in 1, 2 (2×1), 4 (2×2) o 8 (4×2) caleidoscopi affiancati; le celle
vicine sono specchiate, così le giunture combaciano. **Caleido** sceglie a parte il numero di spicchi di ciascuno
(off = solo la griglia di copie specchiate). Entrambi sono automatizzabili e mappabili via MIDI.

## Uscita luci: LED wall, barre, fari, strobo

La finestra **Luci...** divide l'immagine d'uscita (palette, logo ed effetti compresi) in zone: ogni apparecchio prende
i colori della sua zona e li manda sui suoi canali DMX. Le zone si trascinano sull'anteprima dal vivo (l'angolo in
basso a destra le ridimensiona).

- **Apparecchi pronti**: LED wall (colonne × righe di pixel), barra pixel, PAR/faro, strobo. Per ognuno: posizione
  della zona, colonne/righe, universo e indirizzo DMX, e la riga dei **canali**: `R G B W` colori (il bianco viene
  tolto dal colore), `C M Y`, `D` dimmer, `L` luminosità della zona, `F` lampo (strobo di MBStudio91 Visuals, ogni beat,
  bassi o luminosità), `S` velocità strobo dai BPM, numeri = valore fisso. Ciò che sta tra `[ ]` si ripete per ogni
  pixel. Esempi: `[R G B]` per LED wall e barre RGB, `D R G B W 0 0 0` per un PAR RGBW a 8 canali, `F 0` per uno
  strobo a 2 canali (dimmer, velocità).
- **Cablaggio**: serpentina, per colonne, inverti X/Y; "Pixel interi" non spezza mai un pixel tra due universi
  (170 pixel RGB per universo, come chiede la maggior parte dei controller pixel).
- **Uscite**: **Art-Net** (UDP 6454, unicast o broadcast) oppure **sACN / E1.31** (UDP 5568, multicast 239.255.x.y o
  diretto), a 20-44 pacchetti al secondo, più **OSC** (`/mbvx/beat`, `/mbvx/bar`, `/mbvx/bpm`, `/mbvx/level`,
  `/mbvx/bass`, `/mbvx/mid`, `/mbvx/high`, `/mbvx/strobe`). Canali sovrapposti: vince il valore più alto (HTP) e la
  finestra li segnala.
- **Identifica** fa lampeggiare in bianco un apparecchio per 3 secondi: utile per controllare indirizzi e cablaggio.
- La patch si **importa/esporta** in JSON ed è la stessa di MBVX nel browser; viene salvata anche nel progetto della
  DAW. Lo Standalone accetta `--lights patch.json` all'avvio.
- L'invio avviene mentre l'uscita viene disegnata: tieni aperto il plugin o il **Proiettore**.

**LED wall grandi** (Novastar, Colorlight e simili con scheda di invio): vanno pilotati come un monitor, con il
**Proiettore** sull'uscita video collegata alla scheda di invio. Art-Net/sACN servono per pannelli, barre e controller
pixel che leggono DMX in rete.

### Con QLC+ (open source)

QLC+ fa da "motore luci": riceve l'Art-Net/sACN di MBStudio91 Visuals e lo gira alle sue uscite (le interfacce DMX
USB di molte marche, nodi Art-Net, sACN), con la sua libreria di apparecchi.

1. In QLC+, scheda *Ingressi/Uscite*: sull'universo 1 scegli come **ingresso** Art-Net (127.0.0.1 se QLC+ è sullo
   stesso computer) e come **uscita** la tua interfaccia DMX, poi spunta **Passthrough**.
2. Attenzione alla numerazione: in QLC+ l'universo 1 corrisponde di default all'universo Art-Net 0. Metti
   **universo 0** negli apparecchi, oppure imposta l'universo 1 nella configurazione dell'ingresso Art-Net di QLC+.
   Con sACN: universo 1 = multicast 239.255.0.1.
3. **OSC**: QLC+ ascolta dalla porta 7700 (universo 1); i widget della Virtual Console imparano `/mbvx/beat`,
   `/mbvx/bass`, ecc. con il loro pulsante di apprendimento dell'ingresso esterno.

MBVX nel browser parla anche con la **Web API** di QLC+ (avvio con `-w`, WebSocket `ws://IP:9999/qlcplusWS`):
preme un pulsante a ogni beat, tiene premuto lo strobo, muove due cursori con livello e bassi.

### Dante e Kling-Net

- **Dante** (Audinate) è audio in rete, non controllo luci. Sul Mac, *Dante Virtual Soundcard* o *Dante Via* appaiono
  come scheda audio: sceglili come ingresso nello Standalone (o nella DAW) e i visual reagiscono all'audio Dante.
- **Kling-Net** è un protocollo di ArKaos concesso solo su licenza ai produttori: non c'è una specifica pubblica e non
  può entrare in un progetto open source. Per apparecchi Kling-Net usa, se lo hanno, il modo DMX/Art-Net, oppure il
  software ArKaos (Kling-Net Mapper / MediaMaster) per la parte Kling-Net.

## Aspetto "hardware" MBStudio (MBStudioUI)

L'interfaccia usa il kit riutilizzabile `MBStudioUI/`, pensato per tutti i plugin MBStudio:
telaio nero opaco, scritte bianche stampate in maiuscolo, chip bianchi invertiti, LED blu/verdi/ambra,
tasti in gomma con LED, fader con scala a tacche, zone incassate, logo MBSTUDIO91™ stampato o impresso.

Per usarlo in un nuovo plugin JUCE:

```cmake
include(MBStudioUI/MBStudioUI.cmake)
mbstudio_ui_add(NomeTarget)      # font + logo in binary data, LookAndFeel compilato nel target
```

```cpp
#include "MBStudioLookAndFeel.h"
mbstudio::ui::HardwareLookAndFeel lnf;   // setLookAndFeel(&lnf) nell'editor
// nel paint(): ui::paintChassis, ui::paintBrand(area, "NOME"), ui::paintRecess, ui::paintChip, ui::paintLed
// pulsante con LED:  btn.getProperties().set("led", true); btn.getProperties().set("ledColour", "ff3ee05c");
```

Font inclusi: **Bebas Neue** (titoli) e **Antonio** Thin/Light/Bold (etichette e numeri), entrambi SIL OFL 1.1.

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

Test eseguiti su Linux (Xvfb + Mesa llvmpipe, OpenGL 3.2 core): compilazione VST3 + Standalone, uscita luci verificata
con un ricevitore Art-Net/OSC (14 universi, colori e strobo corretti),
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
- Font in `MBStudioUI/fonts/`: Bebas Neue (© Dharma Type) e Antonio (© Vernon Adams; istanze statiche 100/300/700
  generate dal font variabile con fontTools), entrambi **SIL Open Font License 1.1** (testi in `OFL-*.txt`).
- Logo MBSTUDIO91™ in `MBStudioUI/brand/`: marchio di MBStudio91, non coperto dalla licenza AGPL del codice.
