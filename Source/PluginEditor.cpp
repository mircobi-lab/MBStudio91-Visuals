#include "PluginEditor.h"
#include "StageComponent.h"
#include "ShaderEditorWindow.h"

using namespace mbv;

namespace
{
    const juce::Colour kBg (0xff0b0d12), kPanel (0xff141821), kLine (0xff252b38), kText (0xffe8ecf3), kDim (0xff8a93a6),
                       kCyan (0xff00d4ff), kAmber (0xffffb547), kPink (0xffff5470);
    juce::Font uiFont (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }
}

//==============================================================================
MBLookAndFeel::MBLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, kBg);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::Slider::backgroundColourId, kLine);
    setColour (juce::Slider::trackColourId, kCyan);
    setColour (juce::Slider::thumbColourId, kText);
    setColour (juce::ComboBox::backgroundColourId, kPanel);
    setColour (juce::ComboBox::outlineColourId, kLine);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kDim);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kCyan.withAlpha (.25f));
    setColour (juce::TextButton::buttonColourId, kPanel);
    setColour (juce::TextButton::buttonOnColourId, kCyan.withAlpha (.35f));
    setColour (juce::TextButton::textColourOffId, kText);
    setColour (juce::TextButton::textColourOnId, kText);
    setColour (juce::TextEditor::backgroundColourId, kBg);
    setColour (juce::TextEditor::textColourId, kText);
    setColour (juce::TextEditor::outlineColourId, kLine);
    setColour (juce::ScrollBar::thumbColourId, kLine);
    setColour (juce::TooltipWindow::backgroundColourId, kPanel);
    setColour (juce::TooltipWindow::textColourId, kText);
}

void MBLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    const auto track = s.findColour (juce::Slider::trackColourId);
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        const float cy = (float) y + (float) h * .5f;
        g.setColour (kLine); g.fillRoundedRectangle ((float) x, cy - 2, (float) w, 4.f, 2.f);
        const double mn = s.getMinimum(), mx = s.getMaximum();
        const bool bipolar = mn < 0 && mx > 0;
        const float zero = bipolar ? (float) (x + w * (-mn / (mx - mn))) : (float) x;
        g.setColour (track); g.fillRoundedRectangle (juce::jmin (zero, pos), cy - 2, std::abs (pos - zero), 4.f, 2.f);
        g.setColour (s.findColour (juce::Slider::thumbColourId)); g.fillEllipse (pos - 6, cy - 6, 12, 12);
        g.setColour (kBg); g.drawEllipse (pos - 6, cy - 6, 12, 12, 1.f);
    }
    else
    {
        const float cx = (float) x + (float) w * .5f;
        g.setColour (kLine); g.fillRoundedRectangle (cx - 2, (float) y, 4.f, (float) h, 2.f);
        g.setColour (track); g.fillRoundedRectangle (cx - 2, pos, 4.f, (float) (y + h) - pos, 2.f);
        g.setColour (s.findColour (juce::Slider::thumbColourId)); g.fillEllipse (cx - 6, pos - 6, 12, 12);
    }
}
juce::Font MBLookAndFeel::getLabelFont (juce::Label&) { return uiFont (12.f); }
juce::Font MBLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (12.f); }
juce::Font MBLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return uiFont (12.f, true); }

void MBVisualsEditor::Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (kLine); g.fillRoundedRectangle (r, 2.f);
    g.setColour (c); g.fillRoundedRectangle (r.withWidth (r.getWidth() * juce::jlimit (0.f, 1.f, v)), 2.f);
}
void MBVisualsEditor::Dot::paint (juce::Graphics& g)
{
    g.setColour (v > .4f ? kPink : kLine); g.fillEllipse (getLocalBounds().toFloat().reduced (2));
}

//==============================================================================
MBVisualsEditor::MBVisualsEditor (MBVisualsProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    setResizable (true, true);
    setResizeLimits (980, 640, 3840, 2400);
    setSize (1180, 780);
    setWantsKeyboardFocus (true);

    auto label = [this] (juce::Label& l, const juce::String& t, float size, juce::Colour c, juce::Justification j = juce::Justification::centredLeft)
    { l.setText (t, juce::dontSendNotification); l.setFont (uiFont (size, size > 13)); l.setColour (juce::Label::textColourId, c); l.setJustificationType (j); addAndMakeVisible (l); };

    label (title, "MBStudio91 Visuals", 16.f, kText);
    label (bpmLabel, "120.0", 22.f, kCyan, juce::Justification::centred);
    label (srcLabel, "INT", 11.f, kDim, juce::Justification::centred);
    label (fpsLabel, "", 11.f, kDim, juce::Justification::centredRight);
    label (bpmInfo, "", 10.f, kDim);
    bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal); bpmSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    bpmSlider.setTooltip ("BPM manuale (clock Interno). Doppio clic: 120");
    bpmSlider.setDoubleClickReturnValue (true, 120.0);
    bpmAtt = std::make_unique<Apvts::SliderAttachment> (proc.apvts, ids::bpm, bpmSlider); addAndMakeVisible (bpmSlider);
    makeCombo (clockBox, clockAtt, ids::clockSrc, clockNames());
    clockBox.setTooltip ("Host: BPM e posizione dalla DAW. Audio: rilevati dalla musica. Interno: BPM manuale, Tap (spazio) e primo battito (.)");
    tapBtn.setTooltip ("Tap tempo (Spazio): batti sul tempo almeno due volte"); tapBtn.onClick = [this] { proc.engine.tap(); };
    downBtn.setTooltip ("Primo battito della battuta (tasto .)"); downBtn.onClick = [this] { proc.engine.downbeat(); };
    addAndMakeVisible (tapBtn); addAndMakeVisible (downBtn);

    const juce::Colour mc[4] = { kText, kCyan, kAmber, kPink }; const char* mn[4] = { "Liv", "Bassi", "Medi", "Alti" };
    Meter* ms[4] = { &mLevel, &mBass, &mMid, &mHigh };
    for (int i = 0; i < 4; ++i) { ms[i]->c = mc[i]; addAndMakeVisible (*ms[i]); label (mLabels[i], mn[i], 10.f, kDim); }
    addAndMakeVisible (onsetDot);

    displayBox.setTooltip ("Schermo su cui aprire il proiettore"); addAndMakeVisible (displayBox); refreshDisplays();
    projBtn.setTooltip ("Apre/chiude l'uscita a schermo intero sullo schermo scelto (F, o doppio clic sull'immagine). Esc per chiudere");
    projBtn.onClick = [this] { proc.showProjector (proc.projectorOpen() ? -1 : juce::jmax (0, displayBox.getSelectedId() - 1)); };
    addAndMakeVisible (projBtn);
    resBox.addItemList ({ "Deck 960x540", "Deck 1280x720", "Deck 1920x1080" }, 1);
    resBox.setSelectedId (proc.outW >= 1900 ? 3 : proc.outW >= 1200 ? 2 : 1, juce::dontSendNotification);
    resBox.setTooltip (juce::String::fromUTF8 ("Risoluzione interna dei deck (la Qualità automatica la riduce quando serve). L'uscita è sempre alla risoluzione dello schermo"));
    resBox.onChange = [this] { const int w[] = { 960, 1280, 1920 }, h[] = { 540, 720, 1080 }; const int i = juce::jlimit (0, 2, resBox.getSelectedId() - 1); proc.outW = w[i]; proc.outH = h[i]; proc.renderer.setOutputSize (w[i], h[i]); };
    addAndMakeVisible (resBox);
    learnBtn.setTooltip ("MIDI Learn: clic destro su un cursore o menu, scegli 'Assegna', poi muovi un controllo MIDI (CC). Clic qui per annullare. Note fisse: C1 tap, C#1 primo battito, D1/D#1 vai ad A/B, E1 strobo");
    learnBtn.onClick = [this] { proc.cancelMidiLearn(); };
    addAndMakeVisible (learnBtn);
    midiBtn.setTooltip ("Mostra le assegnazioni MIDI");
    midiBtn.onClick = [this]
    {
        juce::PopupMenu m; m.addSectionHeader ("Assegnazioni MIDI");
        for (auto line : juce::StringArray::fromLines (proc.midiMapDescription())) if (line.isNotEmpty()) m.addItem (line, false, false, nullptr);
        m.addSeparator(); m.addItem (1, "Cancella tutte");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (midiBtn), [this] (int r) { if (r == 1) proc.clearMidiMap(); });
    };
    addAndMakeVisible (midiBtn);
    helpBtn.setTooltip (juce::String::fromUTF8 ("Tasti: Spazio tap - . primo battito - Z/X/C crossfader A/metà/B - A/B transizione - F proiettore - S strobo (tieni premuto) - N buio - I inverti - P palette (Shift+P indietro) - 1-9 preset deck A, Shift+1-9 deck B - doppio clic su un cursore: valore predefinito"));
    addAndMakeVisible (helpBtn);

    // palco
    addAndMakeVisible (stageHolder);
    stageAway.setText ("Uscita sul proiettore (Esc o doppio clic per tornare qui)", juce::dontSendNotification);
    stageAway.setJustificationType (juce::Justification::centred); stageAway.setColour (juce::Label::textColourId, kDim);
    stageHolder.addChildComponent (stageAway);

    // deck
    setupDeck (0); setupDeck (1);

    // mix
    label (mixTitle, "MIX", 12.f, kAmber);
    xf.setSliderStyle (juce::Slider::LinearHorizontal); xf.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    xf.setColour (juce::Slider::trackColourId, kAmber); xf.setTooltip ("Crossfader A <-> B. I pulsanti A e B fanno la transizione nel tempo impostato");
    xf.onDragStart = [this] { proc.engine.stopXf(); };
    xfAtt = std::make_unique<Apvts::SliderAttachment> (proc.apvts, ids::xf, xf); addAndMakeVisible (xf);
    goA.onClick = [this] { proc.engine.goTo (0); }; goB.onClick = [this] { proc.engine.goTo (1); };
    goA.setColour (juce::TextButton::buttonOnColourId, kAmber.withAlpha (.5f)); goB.setColour (juce::TextButton::buttonOnColourId, kAmber.withAlpha (.5f));
    addAndMakeVisible (goA); addAndMakeVisible (goB);
    makeKnob (xfTime, ids::xfTime, "Transizione", [] (double v) { return juce::String (v, 1) + " s"; });
    makeKnob (feedback, ids::feedback, "Scia");
    makeKnob (hue, ids::hue, "Tinta", [] (double v) { return juce::String ((int) v) + juce::String::fromUTF8 ("°"); });
    makeKnob (zoom, ids::zoom, "Zoom sul beat");
    makeKnob (kaleido, ids::kaleido, "Caleido", [] (double v) { return v < 1.5 ? juce::String ("off") : juce::String ((int) v); });
    makeKnob (master, ids::master, "Master");
    makeKnob (pixel, ids::pixel, "Pixel", [] (double v) { return v < 1.5 ? juce::String ("pieno") : juce::String ((int) v) + "x"; });
    makeKnob (reactG, ids::react, juce::String::fromUTF8 ("Reattività"));
    makeKnob (beatOffset, ids::beatOffset, "Anticipo", [] (double v) { return juce::String ((int) v) + " ms"; });
    makeCombo (modeBox, modeAtt, ids::mode, modeNames()); label (modeL, "Fusione", 10.f, kDim);
    makeCombo (palBox, palAtt, ids::palette, paletteNames()); label (palL, "Palette (P)", 10.f, kDim);
    makeCombo (strobeBox, strobeAtt, ids::strobeRate, strobeNames()); label (strobeL, "Strobo", 10.f, kDim);
    makeCombo (qualBox, qualAtt, ids::quality, qualityNames()); label (qualL, juce::String::fromUTF8 ("Qualità"), 10.f, kDim);
    qualBox.setTooltip (juce::String::fromUTF8 ("Auto: la risoluzione dei deck scende da sola se gli fps calano, e risale quando può"));
    label (colL, "Colori: Pantone 1-2-3 e Monocolore", 10.f, kDim);
    for (int i = 0; i < 4; ++i)
    {
        colBtn[i].setButtonText (i < 3 ? juce::String (i + 1) : "Mono");
        colBtn[i].setTooltip (i < 3 ? "Colore " + juce::String (i + 1) + " della palette Pantone (scuro / medio / chiaro)" : "Colore della palette Monocolore");
        colBtn[i].onClick = [this, i] { pickColour (i); };
        addAndMakeVisible (colBtn[i]);
    }
    makeToggle (strobeBtn, strobeBAtt, ids::strobe, "Strobo"); strobeBtn.setTooltip ("Tieni premuto S");
    makeToggle (invertBtn, invertAtt, ids::invert, "Inverti (I)");
    makeToggle (mirrorBtn, mirrorAtt, ids::mirror, "Specchio");
    makeToggle (blackBtn, blackAtt, ids::black, "Buio (N)");

    takeStage();
    startTimerHz (30);
}

MBVisualsEditor::~MBVisualsEditor()
{
    shaderWin.reset();
    if (stageHere) { proc.renderer.detach(); stageHolder.removeChildComponent (&proc.stage()); }
    setLookAndFeel (nullptr);
}

//==============================================================================
void MBVisualsEditor::makeKnob (Knob& k, const juce::String& id, const juce::String& text, std::function<juce::String (double)> fmt)
{
    k.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    if (auto* p = proc.apvts.getParameter (id))
    {
        const double def = p->getNormalisableRange().convertFrom0to1 (p->getDefaultValue());
        k.slider.setDoubleClickReturnValue (true, def);
        k.slider.setTooltip (p->getName (40) + " - doppio clic: valore predefinito - clic destro: MIDI");
    }
    k.att = std::make_unique<Apvts::SliderAttachment> (proc.apvts, id, k.slider);
    k.name.setText (text, juce::dontSendNotification); k.name.setFont (uiFont (10.f)); k.name.setColour (juce::Label::textColourId, kDim);
    k.value.setFont (uiFont (10.f)); k.value.setColour (juce::Label::textColourId, kText); k.value.setJustificationType (juce::Justification::centredRight);
    auto upd = [&k, fmt] { const double v = k.slider.getValue(); k.value.setText (fmt ? fmt (v) : juce::String (v, 2), juce::dontSendNotification); };
    k.slider.onValueChange = upd; upd();
    k.slider.setPopupMenuEnabled (false);
    k.slider.addMouseListener (this, false);
    k.slider.getProperties().set ("paramID", id);
    addAndMakeVisible (k.slider); addAndMakeVisible (k.name); addAndMakeVisible (k.value);
}

void MBVisualsEditor::makeCombo (juce::ComboBox& c, std::unique_ptr<Apvts::ComboBoxAttachment>& att, const juce::String& id, const juce::StringArray& items)
{
    c.addItemList (items, 1);
    att = std::make_unique<Apvts::ComboBoxAttachment> (proc.apvts, id, c);
    c.getProperties().set ("paramID", id);
    c.addMouseListener (this, false);
    addAndMakeVisible (c);
}

void MBVisualsEditor::makeToggle (juce::TextButton& b, std::unique_ptr<Apvts::ButtonAttachment>& att, const juce::String& id, const juce::String& text)
{
    b.setButtonText (text); b.setClickingTogglesState (true);
    att = std::make_unique<Apvts::ButtonAttachment> (proc.apvts, id, b);
    b.getProperties().set ("paramID", id);
    b.addMouseListener (this, false);
    addAndMakeVisible (b);
}

void MBVisualsEditor::setupDeck (int i)
{
    auto& d = decks[i];
    d.title.setText (i == 0 ? "DECK A" : "DECK B", juce::dontSendNotification); d.title.setFont (uiFont (12.f, true));
    d.title.setColour (juce::Label::textColourId, i == 0 ? kCyan : kPink); addAndMakeVisible (d.title);
    juce::StringArray names = proc.library.names();
    while (names.size() < maxPresets) names.add ("-");
    d.preset.addItemList (names, 1);
    for (int k = proc.library.size(); k < maxPresets; ++k) d.preset.setItemEnabled (k + 1, false);
    d.presetAtt = std::make_unique<Apvts::ComboBoxAttachment> (proc.apvts, ids::deck (i, ids::preset), d.preset);
    d.preset.setTooltip ("Preset del deck (1-9 / Shift+1-9 da tastiera). Aggiungi i tuoi .glsl in Documenti/MBStudio91 Visuals/Shaders");
    d.preset.getProperties().set ("paramID", ids::deck (i, ids::preset)); d.preset.addMouseListener (this, false);
    addAndMakeVisible (d.preset);
    d.thumb.setImagePlacement (juce::RectanglePlacement::stretchToFit); addAndMakeVisible (d.thumb);
    makeKnob (d.p1, ids::deck (i, ids::p1), "Param 1");
    makeKnob (d.p2, ids::deck (i, ids::p2), "Param 2");
    makeKnob (d.speed, ids::deck (i, ids::speed), juce::String::fromUTF8 ("Velocità"));
    makeKnob (d.react, ids::deck (i, ids::reactDeck), juce::String::fromUTF8 ("Reattività audio"));
    makeKnob (d.rot, ids::deck (i, ids::rot), "Rotazione", [] (double v) { return juce::String ((int) v) + juce::String::fromUTF8 ("°"); });
    makeKnob (d.spin, ids::deck (i, ids::spin), "Spin", [this, i] (double v)
    {
        const bool tempo = proc.apvts.getRawParameterValue (ids::deck (i, ids::spinTempo))->load() > .5f;
        if (std::abs (v) < .03) return juce::String ("fermo");
        const juce::String arrow = v > 0 ? juce::String::fromUTF8 ("↻ ") : juce::String::fromUTF8 ("↺ ");
        if (tempo) { static const double DIV[] = { 32, 16, 8, 4, 2, 1, .5 }; const double b = DIV[juce::jlimit (0, 6, (int) std::ceil (std::abs (v) * 7) - 1)];
                     return arrow + (b >= 1 ? "1 giro/" + juce::String ((int) b) + " beat" : juce::String ((int) (1 / b)) + " giri/beat"); }
        return arrow + juce::String (v * v * 2, 2) + " giri/s";
    });
    d.spinMode.setClickingTogglesState (true);
    d.spinAtt = std::make_unique<Apvts::ButtonAttachment> (proc.apvts, ids::deck (i, ids::spinTempo), d.spinMode);
    d.spinMode.setTooltip ("Spin agganciato al BPM (Tempo) o in giri al secondo (Free)");
    d.spinMode.onStateChange = [this, i] { auto& dk = decks[i]; dk.spinMode.setButtonText (dk.spinMode.getToggleState() ? "Tempo" : "Free"); if (dk.spin.slider.onValueChange) dk.spin.slider.onValueChange(); };
    addAndMakeVisible (d.spinMode);
    d.spinReset.setTooltip ("Azzera rotazione e spin");
    d.spinReset.onClick = [this, i] { decks[i].rot.slider.setValue (0); decks[i].spin.slider.setValue (0); };
    addAndMakeVisible (d.spinReset);
    d.edit.setTooltip ("Apre l'editor dello shader di questo deck (GLSL, mainImage)");
    d.edit.onClick = [this, i] { openShaderEditor (i); };
    addAndMakeVisible (d.edit);
    d.error.setFont (uiFont (10.f)); d.error.setColour (juce::Label::textColourId, kPink); d.error.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (d.error);
}

//==============================================================================
void MBVisualsEditor::takeStage()
{
    if (stageHere || proc.projectorOpen()) { stageAway.setVisible (proc.projectorOpen()); return; }
    stageHolder.addAndMakeVisible (proc.stage());
    stageAway.setVisible (false);
    stageHere = true;
    resized();
    proc.renderer.attachTo (proc.stage());
}

void MBVisualsEditor::releaseStage()
{
    if (! stageHere) return;
    proc.renderer.detach();
    stageHolder.removeChildComponent (&proc.stage());
    stageHere = false;
    stageAway.setVisible (true);
    // il proiettore riaggancia il contesto quando ha il palco
    juce::MessageManager::callAsync ([this] { if (proc.projectorOpen()) proc.renderer.attachTo (proc.stage()); });
}

void MBVisualsEditor::refreshDisplays()
{
    displayBox.clear (juce::dontSendNotification);
    const auto& ds = juce::Desktop::getInstance().getDisplays().displays;
    for (int i = 0; i < ds.size(); ++i)
        displayBox.addItem ("Schermo " + juce::String (i + 1) + (ds[i].isMain ? " (principale)" : "") + " " + juce::String ((int) ds[i].logicalBounds.getWidth()) + "x" + juce::String ((int) ds[i].logicalBounds.getHeight()), i + 1);
    const int want = proc.projectorDisplay() >= 0 ? proc.projectorDisplay() : (ds.size() > 1 ? 1 : 0);
    displayBox.setSelectedId (juce::jlimit (1, juce::jmax (1, ds.size()), want + 1), juce::dontSendNotification);
}

void MBVisualsEditor::openShaderEditor (int deck)
{
    if (shaderWin == nullptr) shaderWin = std::make_unique<ShaderEditorWindow> (proc);
    shaderWin->showDeck (deck);
}

namespace
{
    struct ColourPick : juce::Component, juce::ChangeListener
    {
        ColourPick (MBVisualsProcessor& pr, int idx) : proc (pr), index (idx), sel (juce::ColourSelector::showColourspace | juce::ColourSelector::showSliders | juce::ColourSelector::showColourAtTop)
        { sel.setCurrentColour (proc.palColour (index), juce::dontSendNotification); sel.addChangeListener (this); addAndMakeVisible (sel); setSize (260, 300); }
        void resized() override { sel.setBounds (getLocalBounds()); }
        void changeListenerCallback (juce::ChangeBroadcaster*) override { proc.setPalColour (index, sel.getCurrentColour()); }
        MBVisualsProcessor& proc; int index; juce::ColourSelector sel;
    };
}

void MBVisualsEditor::pickColour (int index)
{
    juce::CallOutBox::launchAsynchronously (std::make_unique<ColourPick> (proc, index), colBtn[index].getScreenBounds(), nullptr);
    const float want = index < 3 ? 8.f : 9.f;
    if (std::abs (proc.apvts.getRawParameterValue (ids::palette)->load() - want) > .5f)
        if (auto* p = proc.apvts.getParameter (ids::palette)) p->setValueNotifyingHost (p->convertTo0to1 (want));
}

void MBVisualsEditor::learnMenu (const juce::String& id, juce::Component& anchor)
{
    juce::PopupMenu m;
    if (auto* p = proc.apvts.getParameter (id)) m.addSectionHeader (p->getName (40));
    m.addItem (1, "Assegna al prossimo controllo MIDI (CC)...");
    m.addItem (2, "Valore predefinito");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (anchor), [this, id] (int r)
    {
        if (r == 1) proc.startMidiLearn (id);
        else if (r == 2) if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
    });
}

//==============================================================================
void MBVisualsEditor::timerCallback()
{
    auto& e = proc.engine;
    bpmLabel.setText (juce::String (e.uiBpm.load(), 1), juce::dontSendNotification);
    static const char* src[] = { "HOST", "AUDIO", "INT", "TAP" };
    const int s = juce::jlimit (0, 3, e.uiSource.load());
    srcLabel.setText (src[s], juce::dontSendNotification);
    srcLabel.setColour (juce::Label::textColourId, s == 0 ? kAmber : s == 1 ? kCyan : kDim);
    const float fps = proc.renderer.fps();
    fpsLabel.setText (fps > 0 ? juce::String ((int) std::round (fps)) + " fps - deck " + juce::String ((int) std::round (e.qualityScale() * 100)) + "%" : "", juce::dontSendNotification);
    fpsLabel.setColour (juce::Label::textColourId, fps > 0 && fps < 30 ? kAmber : kDim);
    bpmInfo.setText (s == 1 ? e.bpmInfo : juce::String(), juce::dontSendNotification);
    mLevel.v = e.uiLevel; mBass.v = e.uiBass; mMid.v = e.uiMid; mHigh.v = e.uiHigh; onsetDot.v = e.uiOnset;
    for (auto* m : { &mLevel, &mBass, &mMid, &mHigh }) m->repaint();
    onsetDot.repaint();
    const bool running = e.uiXfRunning.load(); const int tgt = e.uiXfTarget.load();
    goA.setToggleState (running && tgt == 0, juce::dontSendNotification); goB.setToggleState (running && tgt == 1, juce::dontSendNotification);
    projBtn.setToggleState (proc.projectorOpen(), juce::dontSendNotification);
    projBtn.setButtonText (proc.projectorOpen() ? "Chiudi proiettore" : "Proiettore");
    learnBtn.setToggleState (proc.isLearning(), juce::dontSendNotification);
    learnBtn.setButtonText (proc.isLearning() ? "Attesa CC..." : "MIDI Learn");
    for (int i = 0; i < 2; ++i)
    {
        auto& d = decks[i];
        d.thumb.setImage (proc.renderer.deckThumbnail (i));
        const auto err = proc.renderer.deckError (i);
        d.error.setText (err.isNotEmpty() ? "Errore shader: " + err.upToFirstOccurrenceOf ("\n", false, false) : juce::String(), juce::dontSendNotification);
        d.spinMode.setButtonText (d.spinMode.getToggleState() ? "Tempo" : "Free");
    }
    if (! stageHere && ! proc.projectorOpen()) takeStage();
}

//==============================================================================
void MBVisualsEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    auto r = getLocalBounds().reduced (10);
    r.removeFromTop (70);
    auto panels = r.removeFromBottom (300);
    const int side = juce::jmax (260, panels.getWidth() * 3 / 10);
    juce::Rectangle<int> rects[3] = { panels.removeFromLeft (side).reduced (4), {}, panels.removeFromRight (side).reduced (4) };
    rects[1] = panels.reduced (4);
    for (auto& rc : rects) { g.setColour (kPanel); g.fillRoundedRectangle (rc.toFloat(), 8.f); g.setColour (kLine); g.drawRoundedRectangle (rc.toFloat(), 8.f, 1.f); }
    g.setColour (kLine); g.drawRoundedRectangle (stageHolder.getBounds().toFloat().expanded (1), 4.f, 1.f);
}

void MBVisualsEditor::resized()
{
    auto r = getLocalBounds().reduced (10);
    auto top = r.removeFromTop (34);
    title.setBounds (top.removeFromLeft (150));
    bpmLabel.setBounds (top.removeFromLeft (70)); srcLabel.setBounds (top.removeFromLeft (46));
    bpmSlider.setBounds (top.removeFromLeft (120).reduced (0, 4));
    clockBox.setBounds (top.removeFromLeft (140).reduced (4, 5));
    tapBtn.setBounds (top.removeFromLeft (50).reduced (2, 5)); downBtn.setBounds (top.removeFromLeft (34).reduced (2, 5));
    top.removeFromLeft (10);
    Meter* ms[4] = { &mLevel, &mBass, &mMid, &mHigh };
    for (int i = 0; i < 4; ++i) { auto c = top.removeFromLeft (92); mLabels[i].setBounds (c.removeFromLeft (34)); ms[i]->setBounds (c.reduced (0, 12)); }
    onsetDot.setBounds (top.removeFromLeft (22).reduced (0, 9));
    bpmInfo.setBounds (top);
    auto row2 = r.removeFromTop (30);
    helpBtn.setBounds (row2.removeFromRight (26).reduced (0, 3)); row2.removeFromRight (6);
    fpsLabel.setBounds (row2.removeFromRight (130));
    midiBtn.setBounds (row2.removeFromRight (64).reduced (0, 3)); row2.removeFromRight (4);
    learnBtn.setBounds (row2.removeFromRight (96).reduced (0, 3)); row2.removeFromRight (12);
    resBox.setBounds (row2.removeFromRight (140).reduced (0, 3)); row2.removeFromRight (4);
    projBtn.setBounds (row2.removeFromRight (130).reduced (0, 3)); row2.removeFromRight (4);
    displayBox.setBounds (row2.removeFromRight (210).reduced (0, 3));
    r.removeFromTop (6);
    auto panels = r.removeFromBottom (300);
    r.removeFromBottom (6);
    // palco 16:9 centrato
    auto st = r;
    const int w = juce::jmin (st.getWidth(), st.getHeight() * 16 / 9), h = w * 9 / 16;
    stageHolder.setBounds (st.withSizeKeepingCentre (w, h));
    stageAway.setBounds (stageHolder.getLocalBounds());
    if (stageHere) proc.stage().setBounds (stageHolder.getLocalBounds());

    const int side = juce::jmax (260, panels.getWidth() * 3 / 10);
    layoutDeck (decks[0], panels.removeFromLeft (side).reduced (12));
    layoutDeck (decks[1], panels.removeFromRight (side).reduced (12));
    auto m = panels.reduced (12);
    mixTitle.setBounds (m.removeFromTop (16));
    auto xrow = m.removeFromTop (28);
    goA.setBounds (xrow.removeFromLeft (34).reduced (0, 2)); goB.setBounds (xrow.removeFromRight (34).reduced (0, 2)); xf.setBounds (xrow.reduced (4, 0));
    auto knobRow = [&] (Knob& k, juce::Rectangle<int> rc) { auto t = rc.removeFromTop (12); k.name.setBounds (t.removeFromLeft (t.getWidth() / 2)); k.value.setBounds (t); k.slider.setBounds (rc.reduced (0, 1)); };
    auto comboRow = [&] (juce::Label& l, juce::ComboBox& c, juce::Rectangle<int> rc) { l.setBounds (rc.removeFromTop (12)); c.setBounds (rc.reduced (0, 2)); };
    const int colW = (m.getWidth() - 20) / 3;
    auto c1 = m.removeFromLeft (colW); m.removeFromLeft (10); auto c2 = m.removeFromLeft (colW); m.removeFromLeft (10); auto c3 = m;
    const int rh = 40;
    knobRow (xfTime, c1.removeFromTop (rh)); comboRow (modeL, modeBox, c1.removeFromTop (rh)); knobRow (feedback, c1.removeFromTop (rh)); knobRow (hue, c1.removeFromTop (rh)); knobRow (zoom, c1.removeFromTop (rh)); knobRow (kaleido, c1.removeFromTop (rh));
    comboRow (palL, palBox, c2.removeFromTop (rh)); knobRow (pixel, c2.removeFromTop (rh));
    colL.setBounds (c2.removeFromTop (12)); { auto cr = c2.removeFromTop (26); const int bw = cr.getWidth() / 4; for (int i = 0; i < 4; ++i) colBtn[i].setBounds (cr.removeFromLeft (bw).reduced (2, 1)); }
    knobRow (master, c2.removeFromTop (rh)); knobRow (reactG, c2.removeFromTop (rh)); knobRow (beatOffset, c2.removeFromTop (rh));
    comboRow (strobeL, strobeBox, c3.removeFromTop (rh)); comboRow (qualL, qualBox, c3.removeFromTop (rh));
    { auto br = c3.removeFromTop (rh); const int bw = br.getWidth() / 2; strobeBtn.setBounds (br.removeFromLeft (bw).reduced (2, 6)); blackBtn.setBounds (br.reduced (2, 6)); }
    { auto br = c3.removeFromTop (rh); const int bw = br.getWidth() / 2; invertBtn.setBounds (br.removeFromLeft (bw).reduced (2, 6)); mirrorBtn.setBounds (br.reduced (2, 6)); }
    for (int i = 0; i < 4; ++i) colBtn[i].setColour (juce::TextButton::buttonColourId, proc.palColour (i));
}

void MBVisualsEditor::layoutDeck (Deck& d, juce::Rectangle<int> r)
{
    auto top = r.removeFromTop (22);
    d.title.setBounds (top.removeFromLeft (60)); d.edit.setBounds (top.removeFromRight (70).reduced (0, 1)); top.removeFromRight (4);
    d.preset.setBounds (top);
    r.removeFromTop (6);
    auto tr = r.removeFromTop (72);
    d.thumb.setBounds (tr.removeFromLeft (128));
    tr.removeFromLeft (8);
    auto knobRow = [&] (Knob& k, juce::Rectangle<int> rc) { auto t = rc.removeFromTop (12); k.name.setBounds (t.removeFromLeft (t.getWidth() * 2 / 3)); k.value.setBounds (t); k.slider.setBounds (rc.reduced (0, 1)); };
    knobRow (d.p1, tr.removeFromTop (36)); knobRow (d.p2, tr.removeFromTop (36));
    r.removeFromTop (4);
    const int rh = 36;
    knobRow (d.speed, r.removeFromTop (rh)); knobRow (d.react, r.removeFromTop (rh)); knobRow (d.rot, r.removeFromTop (rh));
    auto sp = r.removeFromTop (rh);
    auto btns = sp.removeFromRight (84);
    knobRow (d.spin, sp.withTrimmedRight (6));
    d.spinMode.setBounds (btns.removeFromLeft (52).withTrimmedTop (12).reduced (1)); d.spinReset.setBounds (btns.withTrimmedTop (12).reduced (1));
    d.error.setBounds (r);
}

//==============================================================================
bool MBVisualsEditor::keyStateChanged (bool)
{
    if (strobeKeyHeld && ! juce::KeyPress::isKeyCurrentlyDown ('S') && ! juce::KeyPress::isKeyCurrentlyDown ('s'))
    {
        strobeKeyHeld = false;
        if (auto* p = proc.apvts.getParameter (ids::strobe)) p->setValueNotifyingHost (0.f);
        return true;
    }
    return false;
}

void MBVisualsEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || e.eventComponent == nullptr) return;
    const auto id = e.eventComponent->getProperties()["paramID"].toString();
    if (id.isNotEmpty()) learnMenu (id, *e.eventComponent);
}

bool MBVisualsEditor::keyPressed (const juce::KeyPress& k)
{
    auto setP = [this] (const juce::String& id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    const auto c = k.getTextCharacter();
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::spaceKey) { proc.engine.tap(); return true; }
    if (c == '.') { proc.engine.downbeat(); return true; }
    if (c == 'z' || c == 'Z') { proc.engine.stopXf(); setP (ids::xf, 0.f); return true; }
    if (c == 'x' || c == 'X') { proc.engine.stopXf(); setP (ids::xf, .5f); return true; }
    if (c == 'c' || c == 'C') { proc.engine.stopXf(); setP (ids::xf, 1.f); return true; }
    if (c == 'a' || c == 'A') { proc.engine.goTo (0); return true; }
    if (c == 'b' || c == 'B') { if (k.getModifiers().isShiftDown()) return false; proc.engine.goTo (1); return true; }
    if (c == 'f' || c == 'F') { projBtn.triggerClick(); return true; }
    if (c == 'i' || c == 'I') { invertBtn.triggerClick(); return true; }
    if (c == 'n' || c == 'N') { blackBtn.triggerClick(); return true; }
    if (c == 's' || c == 'S') { if (! strobeKeyHeld) { strobeKeyHeld = true; setP (ids::strobe, 1.f); } return true; }
    if (c == 'p' || c == 'P') { const int n = paletteNames().size(); int cur = (int) proc.apvts.getRawParameterValue (ids::palette)->load(); cur = k.getModifiers().isShiftDown() ? (cur - 1 + n) % n : (cur + 1) % n; setP (ids::palette, (float) cur); return true; }
    if (code >= '1' && code <= '9') { const int d = k.getModifiers().isShiftDown() ? 1 : 0; setP (ids::deck (d, ids::preset), (float) (code - '1')); return true; }
    return false;
}
