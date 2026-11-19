#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/*  MBStudio UI — stile "hardware" per tutti i plugin e le app di MBStudio91.
    Telaio nero opaco, scritte bianche stampate in maiuscolo, chip invertiti (bianco su nero), LED blu/verdi,
    tasti in gomma rialzati, fader e cursori con scala a tacche, vani incassati, scritta del marchio con badge.

    Font (SIL Open Font License, nella cartella fonts/):
      Bebas Neue  — titoli, pulsanti, chip, scritta del marchio  (Fonts::display)
      Antonio     — Thin (numeri grandi sottili), Light (etichette e valori), Bold (cifre forti)

    Uso:  mbstudio_ui_add (MioPlugin) in CMake, poi nel costruttore dell'editor
          setLookAndFeel (&laf);   // mbstudio::ui::HardwareLookAndFeel laf;
          e nel paint():  mbstudio::ui::paintChassis (g, getLocalBounds());                                    */
namespace mbstudio::ui
{
namespace colours
{
    // telaio e superfici
    inline const juce::Colour chassisTop    { 0xff151617 };
    inline const juce::Colour chassisBottom { 0xff0a0a0b };
    inline const juce::Colour recess        { 0xff050506 };   // vani incassati (schermo, slot dei fader, campi)
    inline const juce::Colour recessEdge    { 0xff26282b };   // bordo chiaro in basso dei vani
    inline const juce::Colour key           { 0xff121314 };   // tasti in gomma
    inline const juce::Colour keyTop        { 0xff1e1f21 };
    inline const juce::Colour keyEdge       { 0xff34363a };
    inline const juce::Colour engrave       { 0xff1f2023 };   // linee incise tra le sezioni
    // stampa
    inline const juce::Colour print         { 0xfff3f3f1 };   // scritte principali
    inline const juce::Colour printDim      { 0xff9b9da2 };   // scritte secondarie
    inline const juce::Colour tick          { 0xffb9bbbf };   // tacche delle scale
    inline const juce::Colour chip          { 0xfff3f3f1 };   // chip invertito
    inline const juce::Colour chipText      { 0xff0b0b0c };
    // LED
    inline const juce::Colour ledBlue       { 0xff3d8bff };   // stato principale / selezione
    inline const juce::Colour ledGreen      { 0xff3ee05c };   // attivo / acceso
    inline const juce::Colour ledAmber      { 0xffffb020 };   // attenzione
    inline const juce::Colour ledRed        { 0xffff3b30 };   // registrazione / errore / colpo
    inline const juce::Colour ledOff        { 0xff1b1d1f };
}

struct Fonts
{
    static juce::Font display (float height);   // Bebas Neue
    static juce::Font label   (float height);   // Antonio Light
    static juce::Font thin    (float height);   // Antonio Thin
    static juce::Font bold    (float height);   // Antonio Bold
};

// ---- funzioni di disegno (usale nel paint() dei componenti)
void paintChassis (juce::Graphics&, juce::Rectangle<int> area);
void paintRecess (juce::Graphics&, juce::Rectangle<float> area, float radius = 3.f);
void paintEngravedLine (juce::Graphics&, juce::Point<float> a, juce::Point<float> b);
void paintChip (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, float fontHeight = 0);
float chipWidth (const juce::String& text, float height);
void paintPrinted (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, float height,
                   juce::Justification = juce::Justification::centredLeft, juce::Colour = colours::print);
void paintLeaderLabel (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, float height);   // "— ADVANCED"
void paintLed (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour, float level);
void paintTicks (juce::Graphics&, juce::Rectangle<float> area, int count, bool vertical, int majorEvery = 5);
// Logo MBStudio91 (dal file originale): "stampato" in bianco opaco, "impresso" nella plastica o "in rilievo".
enum class Imprint { printed, debossed, embossed };
// full = true: logo completo con "GRAPHIC & MUSIC"; false: solo la riga MBSTUDIO91™. Restituisce l'area occupata.
juce::Rectangle<float> paintLogo (juce::Graphics&, juce::Rectangle<float> area, bool full = false, Imprint = Imprint::printed,
                                  juce::Justification = juce::Justification::centredLeft);
// Intestazione del prodotto: logo MBSTUDIO91™ + badge col nome del prodotto in un riquadro contornato (es. "VISUALS").
void paintBrand (juce::Graphics&, juce::Rectangle<float> area, const juce::String& productBadge, Imprint = Imprint::printed);

// ---- LookAndFeel
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel();

    // Proprietà dei componenti riconosciute:
    //   "led"       (bool)      su un TextButton: mostra un LED anche se non è un interruttore
    //   "ledColour" (String)    colore del LED, es. "ff3d8bff" (predefinito verde)
    //   "bipolar"   (bool)      su uno Slider: il riempimento parte dal centro
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max, juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 8; }
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool separator, bool active, bool highlighted, bool ticked, bool hasSub,
                            const juce::String& text, const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;

    juce::Font getLabelFont (juce::Label&) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbPos, int thumbSize, bool over, bool down) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& text, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;
    void drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&, int w, int h, int titleSpaceX, int titleSpaceW, const juce::Image* icon, bool drawTitleTextOnLeft) override;
};

// ---- componenti
class Led : public juce::Component
{
public:
    explicit Led (juce::Colour c = colours::ledGreen) : colour (c) { setInterceptsMouseClicks (false, false); }
    void setLevel (float l) { if (std::abs (l - level) > .01f) { level = l; repaint(); } }
    void setColour (juce::Colour c) { colour = c; repaint(); }
    void paint (juce::Graphics& g) override;
private:
    juce::Colour colour; float level = 0;
};

class LedMeter : public juce::Component        // barra di LED a segmenti (verde, ambra in cima, rosso all'ultimo)
{
public:
    explicit LedMeter (int segments = 10, juce::Colour c = colours::ledGreen) : n (segments), colour (c) { setInterceptsMouseClicks (false, false); }
    void setLevel (float l) { l = juce::jlimit (0.f, 1.f, l); if (std::abs (l - level) > .01f) { level = l; repaint(); } }
    void paint (juce::Graphics& g) override;
private:
    int n; juce::Colour colour; float level = 0;
};
}
