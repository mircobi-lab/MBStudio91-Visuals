#pragma once
#include <juce_core/juce_core.h>

/*  Libreria degli shader dei deck: preset integrati (BinaryData) + cartella utente
    (~/Documents/MBStudio91 Visuals/Shaders/, file .glsl). Fornisce anche l'intestazione comune
    (uniform e funzioni di aiuto, GLSL 1.50) e il generatore dello shader di uscita. */
class ShaderLibrary
{
public:
    struct Preset { juce::String name, code; bool user = false; juce::File file; };

    ShaderLibrary();
    void rescan();

    int size() const                              { return presets.size(); }
    const Preset& get (int i) const               { return presets.getReference (juce::jlimit (0, juce::jmax (0, presets.size() - 1), i)); }
    juce::StringArray names() const;
    int indexOf (const juce::String& name) const;

    static juce::File userFolder();
    static juce::String deckHeader();             // "#version 150" + uniform + helper
    static juce::String deckFooter();             // main() con rotazione del deck

    // Configurazione dello shader di uscita (ogni combinazione è compilata una volta e messa in cache)
    struct PostConfig
    {
        int kind = 2;      // 0 = solo mix (su buffer, per la scia), 1 = solo uscita, 2 = mix + uscita
        int src = 0;       // 0 = A e B, 1 = solo A, 2 = solo B
        int mode = 0, fb = 0, hue = 0, kal = 0, mirror = 0, zoom = 0, invert = 0, pal = 0, strobe = 0;
        juce::String key() const;
    };
    static juce::String postSource (const PostConfig&);
    static juce::String vertexSource();

private:
    juce::Array<Preset> presets;
};
