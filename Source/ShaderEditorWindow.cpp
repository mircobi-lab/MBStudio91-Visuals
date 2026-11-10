#include "ShaderEditorWindow.h"

using namespace mbv;

class ShaderEditorWindow::Content : public juce::Component
{
public:
    Content (MBVisualsProcessor& p) : proc (p), editor (doc, &tokeniser)
    {
        deckBox.addItem ("Deck A", 1); deckBox.addItem ("Deck B", 2); deckBox.setSelectedId (1, juce::dontSendNotification);
        deckBox.onChange = [this] { load (deckBox.getSelectedId() - 1); };
        addAndMakeVisible (deckBox);
        compileBtn.setTooltip ("Compila e applica al deck (Ctrl/Cmd+Invio)"); compileBtn.onClick = [this] { apply(); }; addAndMakeVisible (compileBtn);
        saveBtn.setTooltip ("Salva come preset utente in Documenti/MBStudio91 Visuals/Shaders"); saveBtn.onClick = [this] { save(); }; addAndMakeVisible (saveBtn);
        resetBtn.setTooltip ("Scarta le modifiche e torna al preset scelto nel deck"); resetBtn.onClick = [this] { proc.setCustomCode (deck, {}); load (deck); }; addAndMakeVisible (resetBtn);
        folderBtn.setTooltip ("Apre la cartella dei preset utente"); folderBtn.onClick = [] { auto f = ShaderLibrary::userFolder(); f.createDirectory(); f.revealToUser(); }; addAndMakeVisible (folderBtn);
        editor.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.f, juce::Font::plain)));
        editor.setTabSize (2, true);
        editor.setColour (juce::CodeEditorComponent::backgroundColourId, juce::Colour (0xff0b0d12));
        editor.setColour (juce::CodeEditorComponent::defaultTextColourId, juce::Colour (0xffe8ecf3));
        editor.setColour (juce::CodeEditorComponent::lineNumberBackgroundId, juce::Colour (0xff141821));
        editor.setColour (juce::CodeEditorComponent::lineNumberTextId, juce::Colour (0xff8a93a6));
        editor.setColour (juce::CodeEditorComponent::highlightColourId, juce::Colour (0x4000d4ff));
        editor.setColourScheme (tokeniser.getDefaultColourScheme());
        addAndMakeVisible (editor);
        status.setFont (juce::Font (juce::FontOptions (12.f))); status.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (status);
        help.setText ("Scrivi mainImage(out vec4 o, in vec2 f). Disponibili: iTime, iResolution, iBass/iMid/iHigh/iLevel, iBeat, iPhase, iBar, iBPM, iParam1/2, "
                      "audioSpec(x), audioWave(x), waveMin/waveMax(x), audioPeak(x), specHist(x, age 0..63), iHistFrac, rot2, glow, hash, noise, hsv2rgb, rainbow. GLSL 1.50 (OpenGL 3.2).",
                      juce::dontSendNotification);
        help.setFont (juce::Font (juce::FontOptions (11.f))); help.setColour (juce::Label::textColourId, juce::Colour (0xff8a93a6));
        help.setMinimumHorizontalScale (1.f); addAndMakeVisible (help);
        setSize (900, 640);
    }

    void load (int d)
    {
        deck = d; deckBox.setSelectedId (d + 1, juce::dontSendNotification);
        doc.replaceAllContent (proc.effectiveCode (d));
        doc.clearUndoHistory();
        status.setText (proc.customCode (d).isNotEmpty() ? "Shader personalizzato attivo su questo deck" : "Codice del preset: modificalo e premi Compila", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colour (0xff8a93a6));
    }
    void apply()
    {
        proc.setCustomCode (deck, doc.getAllContent());
        status.setText ("Compilazione...", juce::dontSendNotification);
        pendingCheck = 20;
    }
    void save()
    {
        auto folder = ShaderLibrary::userFolder(); folder.createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Salva preset shader", folder.getChildFile ("Mio shader.glsl"), "*.glsl");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult(); if (f == juce::File()) return;
            if (! f.hasFileExtension ("glsl")) f = f.withFileExtension ("glsl");
            f.replaceWithText (doc.getAllContent());
            proc.library.rescan();
            status.setText ("Salvato: " + f.getFileName() + " (riapri il plugin per vederlo nell'elenco dei preset)", juce::dontSendNotification);
        });
    }
    void poll()
    {
        if (pendingCheck > 0)
        {
            --pendingCheck;
            const auto err = proc.renderer.deckError (deck);
            if (err.isNotEmpty()) { status.setText (err, juce::dontSendNotification); status.setColour (juce::Label::textColourId, juce::Colour (0xffff5470)); pendingCheck = 0; }
            else if (pendingCheck == 0) { status.setText ("Compilato e applicato al deck " + juce::String (deck == 0 ? "A" : "B"), juce::dontSendNotification); status.setColour (juce::Label::textColourId, juce::Colour (0xff00d4ff)); }
        }
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k.getKeyCode() == juce::KeyPress::returnKey && k.getModifiers().isCommandDown()) { apply(); return true; }
        return false;
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (8);
        auto top = r.removeFromTop (28);
        deckBox.setBounds (top.removeFromLeft (110)); top.removeFromLeft (8);
        compileBtn.setBounds (top.removeFromLeft (120)); top.removeFromLeft (8);
        saveBtn.setBounds (top.removeFromLeft (130)); top.removeFromLeft (8);
        resetBtn.setBounds (top.removeFromLeft (120)); top.removeFromLeft (8);
        folderBtn.setBounds (top.removeFromLeft (110));
        r.removeFromTop (6);
        help.setBounds (r.removeFromTop (34));
        status.setBounds (r.removeFromBottom (60));
        editor.setBounds (r);
    }

    MBVisualsProcessor& proc;
    int deck = 0, pendingCheck = 0;
    juce::ComboBox deckBox;
    juce::TextButton compileBtn { "Compila" }, saveBtn { "Salva preset..." }, resetBtn { "Ripristina" }, folderBtn { "Cartella..." };
    juce::CodeDocument doc;
    juce::CPlusPlusCodeTokeniser tokeniser;
    juce::CodeEditorComponent editor;
    juce::Label status, help;
    std::unique_ptr<juce::FileChooser> chooser;
};

ShaderEditorWindow::ShaderEditorWindow (MBVisualsProcessor& p)
    : juce::DocumentWindow ("MBStudio91 Visuals - Editor shader", juce::Colour (0xff0b0d12), juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
{
    content = new Content (p);
    setContentOwned (content, true);
    setResizable (true, false);
    setUsingNativeTitleBar (true);
    centreWithSize (900, 640);
    startTimerHz (10);
}

ShaderEditorWindow::~ShaderEditorWindow() { stopTimer(); }

void ShaderEditorWindow::showDeck (int d)
{
    content->load (d);
    setVisible (true); toFront (true);
}

void ShaderEditorWindow::timerCallback() { if (content != nullptr && isVisible()) content->poll(); }
