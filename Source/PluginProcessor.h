#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "AudioFeed.h"
#include "VisualEngine.h"
#include "Renderer.h"
#include "ShaderLibrary.h"

class StageComponent;
class ProjectorWindow;

class MBVisualsProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::Timer
{
public:
    MBVisualsProcessor();
    ~MBVisualsProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                 { return true; }
    const juce::String getName() const override     { return JucePlugin_Name; }
    bool acceptsMidi() const override               { return true; }
    bool producesMidi() const override              { return false; }
    bool isMidiEffect() const override              { return false; }
    double getTailLengthSeconds() const override    { return 0; }
    int getNumPrograms() override                   { return 1; }
    int getCurrentProgram() override                { return 0; }
    void setCurrentProgram (int) override           {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    AudioFeed feed;
    VisualEngine engine;
    Renderer renderer;
    ShaderLibrary library;

    // Palco (il componente a cui è agganciato OpenGL) e finestra proiettore: vivono nel processore,
    // così il proiettore resta acceso anche con l'editor chiuso.
    StageComponent& stage()                         { return *stageComp; }
    void showProjector (int displayIndex);          // -1 = chiudi
    bool projectorOpen() const;
    int projectorDisplay() const                    { return projDisplay; }

    // Shader personalizzati per deck (vuoto = usa il preset scelto)
    void setCustomCode (int deck, const juce::String& code);
    juce::String customCode (int deck) const        { return customCodes[deck]; }
    juce::String effectiveCode (int deck) const;
    void refreshDeckShaders();

    // Colori
    juce::Colour palColour (int i) const;           // 0..2 Pantone, 3 Monocolore
    void setPalColour (int i, juce::Colour);

    // MIDI learn (per lo Standalone o host senza mappatura): CC -> parametro
    void startMidiLearn (const juce::String& paramID) { learnTarget = paramID; }
    void cancelMidiLearn()                            { learnTarget.clear(); }
    bool isLearning() const                           { return learnTarget.isNotEmpty(); }
    juce::String midiMapDescription() const;
    void clearMidiMap()                               { const juce::ScopedLock sl (midiLock); midiMap.clear(); }

    std::atomic<int> outW { 1280 }, outH { 720 };
    std::atomic<bool> testTone { false };           // solo per i test: segnale sintetico (cassa a 128 BPM) senza scheda audio

private:
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    void handleMidi (const juce::MidiBuffer&);
    void updateHostInfo();
    void loadStateProps();

    std::unique_ptr<StageComponent> stageComp;
    std::unique_ptr<ProjectorWindow> projector;
    int projDisplay = -1;
    juce::String customCodes[2];
    juce::Colour palCols[4] { juce::Colour (0xff00d4ff), juce::Colour (0xffffb547), juce::Colour (0xffff5470), juce::Colour (0xff00ff66) };
    std::atomic<bool> shadersDirty { true };

    juce::CriticalSection midiLock;
    std::map<int, juce::String> midiMap;            // (canale<<8 | cc) -> parametro
    juce::String learnTarget;
    std::atomic<int> lastCCSeen { -1 };
    struct ToneGen; std::unique_ptr<ToneGen> toneGen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MBVisualsProcessor)
};
