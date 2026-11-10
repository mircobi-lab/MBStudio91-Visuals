#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/*  Identificatori dei parametri automatizzabili (APVTS).
    Tutti i parametri sono esposti all'host: automazione, MIDI learn dell'host, salvataggio nel progetto. */
namespace mbv
{
namespace ids
{
    // Mix / uscita
    inline constexpr const char* xf         = "xf";          // crossfader 0 = A, 1 = B
    inline constexpr const char* xfTime     = "xfTime";      // durata transizione A/B (s)
    inline constexpr const char* mode       = "mode";        // fusione
    inline constexpr const char* feedback   = "feedback";    // scia
    inline constexpr const char* hue        = "hue";         // tinta (gradi)
    inline constexpr const char* zoom       = "zoom";        // zoom sul beat
    inline constexpr const char* kaleido    = "kaleido";     // caleidoscopio (0 = off)
    inline constexpr const char* strobe     = "strobe";
    inline constexpr const char* strobeRate = "strobeRate";
    inline constexpr const char* invert     = "invert";
    inline constexpr const char* mirror     = "mirror";
    inline constexpr const char* black      = "black";
    inline constexpr const char* master     = "master";
    inline constexpr const char* palette    = "palette";
    inline constexpr const char* pixel      = "pixel";       // dimensione del pixel (1 = pieno)
    inline constexpr const char* quality    = "quality";     // risoluzione interna dei deck
    // Clock
    inline constexpr const char* clockSrc   = "clockSrc";    // Host / Audio / Interno
    inline constexpr const char* bpm        = "bpm";         // BPM manuale (Interno) e valore mostrato
    inline constexpr const char* beatOffset = "beatOffset";  // anticipo/ritardo dei visual (ms)
    inline constexpr const char* react      = "reactGlobal"; // reattività globale
    // Momentanei (si azzerano da soli)
    inline constexpr const char* tap        = "tap";
    inline constexpr const char* downbeat   = "downbeat";
    inline constexpr const char* goA        = "goA";
    inline constexpr const char* goB        = "goB";

    // Deck: prefisso "a" / "b"
    inline juce::String deck (int i, const char* name) { return juce::String (i == 0 ? "a" : "b") + name; }
    inline constexpr const char* preset    = "Preset";
    inline constexpr const char* p1        = "Param1";
    inline constexpr const char* p2        = "Param2";
    inline constexpr const char* speed     = "Speed";
    inline constexpr const char* reactDeck = "React";
    inline constexpr const char* rot       = "Rot";
    inline constexpr const char* spin      = "Spin";
    inline constexpr const char* spinTempo = "SpinTempo";
}

inline constexpr int maxPresets = 128;   // slot disponibili nel parametro "Preset" (presets integrati + utente)

const juce::StringArray& modeNames();
const juce::StringArray& paletteNames();
const juce::StringArray& strobeNames();
const juce::StringArray& qualityNames();
const juce::StringArray& clockNames();

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Stato non automatizzabile salvato accanto ai parametri (colori, shader modificati, finestra proiettore...)
namespace state
{
    inline constexpr const char* palC1     = "palC1";
    inline constexpr const char* palC2     = "palC2";
    inline constexpr const char* palC3     = "palC3";
    inline constexpr const char* monoColor = "monoColor";
    inline constexpr const char* codeA     = "codeA";       // shader modificato nell'editor (vuoto = preset)
    inline constexpr const char* codeB     = "codeB";
    inline constexpr const char* projDisplay = "projDisplay";
    inline constexpr const char* outW      = "outW";
    inline constexpr const char* outH      = "outH";
}
}
