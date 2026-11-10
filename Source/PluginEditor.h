#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ShaderEditorWindow;

class MBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MBLookAndFeel();
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
};

class MBVisualsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MBVisualsEditor (MBVisualsProcessor&);
    ~MBVisualsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool) override;
    void mouseDown (const juce::MouseEvent&) override;

    void takeStage();       // il palco torna nell'editor (proiettore chiuso)
    void releaseStage();    // il palco va al proiettore

private:
    using Apvts = juce::AudioProcessorValueTreeState;
    struct Knob
    {
        juce::Slider slider; juce::Label name, value;
        std::unique_ptr<Apvts::SliderAttachment> att;
    };
    struct Deck
    {
        juce::Label title; juce::ComboBox preset; std::unique_ptr<Apvts::ComboBoxAttachment> presetAtt;
        juce::ImageComponent thumb;
        Knob p1, p2, speed, react, rot, spin;
        juce::TextButton spinMode { "Free" }, spinReset { "0" }, edit { "Shader..." };
        std::unique_ptr<Apvts::ButtonAttachment> spinAtt;
        juce::Label error;
    };

    void makeKnob (Knob&, const juce::String& id, const juce::String& label, std::function<juce::String (double)> fmt = nullptr);
    void makeCombo (juce::ComboBox&, std::unique_ptr<Apvts::ComboBoxAttachment>&, const juce::String& id, const juce::StringArray& items);
    void makeToggle (juce::TextButton&, std::unique_ptr<Apvts::ButtonAttachment>&, const juce::String& id, const juce::String& text);
    void setupDeck (int i);
    void layoutDeck (Deck&, juce::Rectangle<int>);
    void timerCallback() override;
    void pickColour (int index);
    void learnMenu (const juce::String& paramID, juce::Component& anchor);
    void refreshDisplays();
    void openShaderEditor (int deck);

    MBVisualsProcessor& proc;
    MBLookAndFeel lnf;
    juce::Component stageHolder;           // ospita il palco (o una scritta quando è sul proiettore)
    juce::Label stageAway;

    // barra superiore
    juce::Label title, bpmLabel, srcLabel, fpsLabel, bpmInfo;
    juce::TextButton tapBtn { "Tap" }, downBtn { "1" }, projBtn { "Proiettore" }, learnBtn { "MIDI Learn" }, midiBtn { "MIDI..." }, helpBtn { "?" };
    juce::ComboBox clockBox, displayBox, resBox;
    std::unique_ptr<Apvts::ComboBoxAttachment> clockAtt;
    juce::Slider bpmSlider; std::unique_ptr<Apvts::SliderAttachment> bpmAtt;
    struct Meter : juce::Component { float v = 0; juce::Colour c; void paint (juce::Graphics&) override; } mLevel, mBass, mMid, mHigh;
    juce::Label mLabels[4];
    struct Dot : juce::Component { float v = 0; void paint (juce::Graphics&) override; } onsetDot;

    // mix
    Deck decks[2];
    juce::Label mixTitle;
    juce::Slider xf; std::unique_ptr<Apvts::SliderAttachment> xfAtt;
    juce::TextButton goA { "A" }, goB { "B" };
    Knob xfTime, feedback, hue, zoom, kaleido, master, pixel, reactG, beatOffset;
    juce::ComboBox modeBox, palBox, strobeBox, qualBox;
    std::unique_ptr<Apvts::ComboBoxAttachment> modeAtt, palAtt, strobeAtt, qualAtt;
    juce::Label modeL, palL, strobeL, qualL, colL;
    juce::TextButton strobeBtn { "Strobo" }, invertBtn { "Inverti" }, mirrorBtn { "Specchio" }, blackBtn { "Buio" };
    std::unique_ptr<Apvts::ButtonAttachment> strobeBAtt, invertAtt, mirrorAtt, blackAtt;
    juce::TextButton colBtn[4];

    std::unique_ptr<ShaderEditorWindow> shaderWin;
    bool stageHere = false, strobeKeyHeld = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MBVisualsEditor)
};
