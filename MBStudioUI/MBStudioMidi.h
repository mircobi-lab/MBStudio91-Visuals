#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>

/*  MBStudio MIDI — controller MIDI per tutti i plugin MBStudio91.
    - MIDI Learn su qualsiasi parametro (clic destro sul controllo, oppure tasto LEARN e clic sul controllo)
    - modi assoluto (con soft takeover), relativo complemento a due (Push 2, encoder), relativo offset 64,
      commuta e momentaneo (tasti e pad)
    - profili pronti per controller diffusi (Novation, AKAI, M-Audio, Ableton Push 2): ogni plugin dichiara
      fino a 4 banchi (A-D) di 8 parametri, il profilo assegna i controlli fisici ai banchi
    - mappa salvata nel progetto e, a richiesta, come predefinita per le nuove istanze

    Uso nel processore:
        mbstudio::midi::MidiMapper midiMap { *this, "NomePlugin" };   // dopo la creazione dei parametri
        midiMap.setBanks ({ juce::StringArray { "cutoff", ... }, ... });
        processBlock:  midiMap.process (midi);   // prima di passare il MIDI al motore
        stato:         tree.appendChild (midiMap.toValueTree(), nullptr);  /  midiMap.fromValueTree (tree.getChildWithName ("MIDIMAP"))
    Nell'editor:
        mbstudio::midi::tag (slider, "cutoff");                     // per ogni controllo legato a un parametro
        mbstudio::midi::MidiLearnOverlay overlay { proc.midiMap };  // addAndMakeVisible per ultimo, a tutta area
        mbstudio::midi::MidiControls midiButtons { proc.midiMap };  // tasti LEARN e CONTROLLER nell'intestazione   */
namespace mbstudio::midi
{
enum class Mode : uint8_t { absolute = 0, relTwos, relOffset, toggle, momentary };
juce::String modeName (Mode);

struct Binding
{
    int channel = 0;            // 0 = qualsiasi canale, 1..16
    bool note = false;          // false = CC
    int number = 0;             // CC o nota 0..127
    juce::String paramId;
    Mode mode = Mode::absolute;
    juce::String describe() const;   // "CC 74 CH 1"
};

struct ProfileSlot { char bank; int index; int channel; bool note; int number; Mode mode; };
struct Profile
{
    juce::String name;
    juce::StringArray portMatch;     // parti del nome della porta MIDI (minuscolo) per la scelta automatica
    bool verified;                   // false: dati pubblici parziali, da controllare sull'esemplare
    juce::String setupNote;          // cosa impostare sul controller
    std::vector<ProfileSlot> slots;
};
const std::vector<Profile>& profiles();

void tag (juce::Component&, const juce::String& paramId);   // collega un controllo del pannello a un parametro
juce::String taggedParam (const juce::Component*);           // risale ai genitori

class MidiMapper : private juce::AsyncUpdater
{
public:
    MidiMapper (juce::AudioProcessor&, const juce::String& pluginFolder);
    ~MidiMapper() override;

    void setBanks (std::array<juce::StringArray, 4> banksAtoD);

    // ---- thread audio
    void process (juce::MidiBuffer&) noexcept;

    // ---- thread dei messaggi
    void startLearn (const juce::String& paramId);
    void cancelLearn();
    juce::String learningParam() const;
    void setLearnMode (bool);
    bool isLearnMode() const noexcept { return learnMode; }

    void forget (const juce::String& paramId);
    void clearAll();
    void setMode (const juce::String& paramId, Mode);
    std::vector<Binding> bindings() const;
    const Binding* bindingFor (const juce::String& paramId) const;   // valido fino alla prossima modifica
    juce::String describe (const juce::String& paramId) const;
    juce::String paramName (const juce::String& paramId) const;

    bool applyProfile (int index);                    // -1 = nessun profilo
    int currentProfile() const noexcept { return profileIndex; }
    bool autoSelectFromPorts (const juce::StringArray& portNames);   // solo se non ci sono assegnazioni

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);
    bool saveAsDefault();
    bool loadDefault();
    juce::File defaultFile() const;

    void showStatus (const juce::String&);
    juce::String status() const;                       // vuoto dopo 3 s
    std::function<void()> onChange;                   // thread dei messaggi
    juce::ChangeBroadcaster changes;

private:
    juce::AudioProcessor& proc;
    juce::String folder;
    juce::Array<juce::AudioProcessorParameter*> params;
    juce::HashMap<juce::String, int> indexOf;
    std::array<juce::StringArray, 4> banks;

    mutable juce::CriticalSection lock;
    std::vector<Binding> list;
    int profileIndex = -1;
    bool learnMode = false;
    juce::String learning;
    juce::String statusText;
    juce::uint32 statusMs = 0;

    // tabella del thread audio: [nota?][canale][numero] -> (indice parametro + 1) | (modo << 16)
    static constexpr int tableSize = 2 * 16 * 128;
    std::array<std::atomic<uint32_t>, tableSize> table {};
    std::array<float, tableSize> lastHw {};
    std::array<uint8_t, tableSize> picked {};
    std::atomic<int> learnParam { -1 };
    std::atomic<uint32_t> learned { 0 };               // bit 31 valido, 15 nota, 8..11 canale, 0..6 numero
    juce::MidiBuffer scratch;

    void rebuildTable();
    void apply (int slot, uint32_t entry, int value, bool noteOn) noexcept;
    void handleAsyncUpdate() override;
    void changed();
};

// Strato trasparente sopra il pannello: menu col clic destro, cornici ambra in modalità LEARN, avvisi.
class MidiLearnOverlay : public juce::Component, private juce::Timer, private juce::ChangeListener
{
public:
    explicit MidiLearnOverlay (MidiMapper&);
    ~MidiLearnOverlay() override;
    void parentHierarchyChanged() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;   // eventi di tutto il pannello (listener sul genitore)
private:
    MidiMapper& map;
    juce::Component* root = nullptr;
    juce::Array<juce::Component::SafePointer<juce::Component>> tagged;
    int ticks = 0;
    void collect();
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { collect(); repaint(); }
    void showMenu (const juce::String& paramId, juce::Component& target);
};

// Tasti LEARN e CONTROLLER per l'intestazione del pannello.
class MidiControls : public juce::Component, private juce::ChangeListener
{
public:
    explicit MidiControls (MidiMapper&);
    ~MidiControls() override;
    void resized() override;
private:
    MidiMapper& map;
    juce::TextButton learnBtn { "LEARN" }, ctrlBtn { "CONTROLLER" };
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void showControllerMenu();
};
}
