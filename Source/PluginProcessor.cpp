#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "StageComponent.h"
#include "ProjectorWindow.h"

using namespace mbv;

MBVisualsProcessor::MBVisualsProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MBStudio91Visuals", createParameterLayout()),
      engine (apvts, feed),
      renderer (apvts, engine)
{
    stageComp = std::make_unique<StageComponent>();
    stageComp->onDoubleClick = [this] { showProjector (projectorOpen() ? -1 : juce::jmax (0, projDisplay)); };
    apvts.addParameterListener (ids::deck (0, ids::preset), this);
    apvts.addParameterListener (ids::deck (1, ids::preset), this);
    loadStateProps();
    refreshDeckShaders();
    renderer.setPaletteColours (palCols[0], palCols[1], palCols[2], palCols[3]);
    startTimerHz (10);
}

MBVisualsProcessor::~MBVisualsProcessor()
{
    stopTimer();
    toneGen.reset();
    projector.reset();
    renderer.detach();
}

//==============================================================================
void MBVisualsProcessor::prepareToPlay (double sr, int) { feed.prepare (sr); }

bool MBVisualsProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in.isDisabled()) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    return out == in || out.isDisabled();
}

void MBVisualsProcessor::updateHostInfo()
{
    AudioFeed::HostInfo h;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) { h.bpm = *bpm; h.hasBpm = true; }
            if (auto ppq = pos->getPpqPosition()) { h.ppq = *ppq; h.hasPpq = true; }
            h.playing = pos->getIsPlaying();
        }
    h.atSample = (double) feed.audioTimeNow() * feed.sampleRate();
    feed.setHost (h);
}

void MBVisualsProcessor::handleMidi (const juce::MidiBuffer& midi)
{
    if (midi.isEmpty()) return;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController())
        {
            const int key = (m.getChannel() << 8) | m.getControllerNumber();
            lastCCSeen = key;
            juce::String target;
            { const juce::ScopedTryLock sl (midiLock); if (sl.isLocked()) { auto it = midiMap.find (key); if (it != midiMap.end()) target = it->second; } }
            if (target.isNotEmpty())
                if (auto* p = apvts.getParameter (target))
                {
                    const float v = m.getControllerValue() / 127.f;
                    if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr) p->setValueNotifyingHost (v > .5f ? 1.f : 0.f);
                    else p->setValueNotifyingHost (v);
                }
        }
        else if (m.isNoteOn())
        {
            // note fisse: C1 (36) tap, C#1 (37) primo battito, D1 (38) vai ad A, D#1 (39) vai a B, E1 (40) strobo
            switch (m.getNoteNumber())
            {
                case 36: engine.tap(); break;
                case 37: engine.downbeat(); break;
                case 38: engine.goTo (0); break;
                case 39: engine.goTo (1); break;
                case 40: if (auto* p = apvts.getParameter (ids::strobe)) p->setValueNotifyingHost (1.f); break;
                default: break;
            }
        }
        else if (m.isNoteOff() && m.getNoteNumber() == 40)
        {
            if (auto* p = apvts.getParameter (ids::strobe)) p->setValueNotifyingHost (0.f);
        }
    }
}

void MBVisualsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    updateHostInfo();
    const int inCh = getTotalNumInputChannels();
    feed.process (buffer.getArrayOfReadPointers(), juce::jmin (inCh, buffer.getNumChannels()), buffer.getNumSamples());
    handleMidi (midi);
    // l'audio passa inalterato (il plugin è un analizzatore): eventuali canali di uscita in più vengono azzerati
    for (int ch = inCh; ch < getTotalNumOutputChannels(); ++ch) buffer.clear (ch, 0, buffer.getNumSamples());
}

void MBVisualsProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    juce::AudioBuffer<float> tmp (buffer.getNumChannels(), buffer.getNumSamples());
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        for (int i = 0; i < buffer.getNumSamples(); ++i) tmp.setSample (c, i, (float) buffer.getSample (c, i));
    updateHostInfo();
    feed.process (tmp.getArrayOfReadPointers(), juce::jmin (getTotalNumInputChannels(), tmp.getNumChannels()), tmp.getNumSamples());
    handleMidi (midi);
}

//==============================================================================
/* Generatore di prova: cassa (seno 55 Hz con inviluppo) sul beat a 128 BPM + hi-hat (rumore) sugli ottavi.
   Spinge blocchi nel ponte audio da un timer ad alta risoluzione, come farebbe la scheda audio. */
struct MBVisualsProcessor::ToneGen : juce::HighResolutionTimer
{
    MBVisualsProcessor& p; double sr = 48000, t = 0; juce::Random rnd;
    explicit ToneGen (MBVisualsProcessor& pp) : p (pp) { p.feed.prepare (sr); startTimer (10); }
    ~ToneGen() override { stopTimer(); }
    void hiResTimerCallback() override
    {
        constexpr int n = 480; float buf[n];
        const double beat = 60.0 / 128.0;
        for (int i = 0; i < n; ++i, t += 1.0 / sr)
        {
            const double ph = std::fmod (t, beat), ph8 = std::fmod (t, beat / 2);
            const float kick = (float) (std::sin (2 * juce::MathConstants<double>::pi * (55.0 * ph + 30.0 * std::exp (-ph * 25))) * std::exp (-ph * 9) * .9);
            const float hat = (rnd.nextFloat() * 2 - 1) * (float) std::exp (-ph8 * 60) * .25f;
            const float pad = (float) (std::sin (2 * juce::MathConstants<double>::pi * 220 * t) * .08 + std::sin (2 * juce::MathConstants<double>::pi * 331 * t) * .05);
            buf[i] = kick + hat + pad;
        }
        const float* chans[1] = { buf };
        p.feed.process (chans, 1, n);
    }
};

void MBVisualsProcessor::parameterChanged (const juce::String&, float) { shadersDirty = true; }

void MBVisualsProcessor::timerCallback()
{
    if (shadersDirty.exchange (false)) refreshDeckShaders();
    if (testTone && toneGen == nullptr) toneGen = std::make_unique<ToneGen> (*this);
    // MIDI learn: il primo CC che arriva viene assegnato al parametro in attesa
    if (learnTarget.isNotEmpty())
    {
        const int key = lastCCSeen.exchange (-1);
        if (key >= 0)
        {
            const juce::ScopedLock sl (midiLock);
            for (auto it = midiMap.begin(); it != midiMap.end();) { if (it->second == learnTarget) it = midiMap.erase (it); else ++it; }
            midiMap[key] = learnTarget;
            learnTarget.clear();
        }
    }
    else lastCCSeen = -1;
}

juce::String MBVisualsProcessor::effectiveCode (int d) const
{
    if (customCodes[d].isNotEmpty()) return customCodes[d];
    const int idx = (int) apvts.getRawParameterValue (ids::deck (d, ids::preset))->load();
    return library.get (idx).code;
}

void MBVisualsProcessor::refreshDeckShaders()
{
    for (int d = 0; d < 2; ++d) renderer.setDeckCode (d, effectiveCode (d));
}

void MBVisualsProcessor::setCustomCode (int d, const juce::String& code)
{
    customCodes[d] = code;
    apvts.state.setProperty (d == 0 ? state::codeA : state::codeB, code, nullptr);
    refreshDeckShaders();
}

juce::Colour MBVisualsProcessor::palColour (int i) const { return palCols[juce::jlimit (0, 3, i)]; }

void MBVisualsProcessor::setPalColour (int i, juce::Colour c)
{
    palCols[juce::jlimit (0, 3, i)] = c;
    static const char* keys[4] = { state::palC1, state::palC2, state::palC3, state::monoColor };
    apvts.state.setProperty (keys[juce::jlimit (0, 3, i)], c.toString(), nullptr);
    renderer.setPaletteColours (palCols[0], palCols[1], palCols[2], palCols[3]);
}

juce::String MBVisualsProcessor::midiMapDescription() const
{
    const juce::ScopedLock sl (midiLock);
    juce::String s;
    for (auto& kv : midiMap)
    {
        if (auto* p = apvts.getParameter (kv.second))
            s << "CC " << (kv.first & 0xff) << " (canale " << (kv.first >> 8) << ") -> " << p->getName (40) << "\n";
    }
    return s.isEmpty() ? "Nessuna assegnazione MIDI" : s;
}

//==============================================================================
void MBVisualsProcessor::showProjector (int displayIndex)
{
    if (displayIndex < 0)
    {
        if (projector != nullptr)
        {
            projector.reset();
            if (auto* ed = dynamic_cast<MBVisualsEditor*> (getActiveEditor())) ed->takeStage();
        }
        return;
    }
    projDisplay = displayIndex;
    apvts.state.setProperty (state::projDisplay, projDisplay, nullptr);
    if (projector != nullptr && projector->displayIndex() == displayIndex) return;
    projector.reset();
    if (auto* ed = dynamic_cast<MBVisualsEditor*> (getActiveEditor())) ed->releaseStage();
    projector = std::make_unique<ProjectorWindow> (*stageComp, displayIndex, [this] { juce::MessageManager::callAsync ([this] { showProjector (-1); }); });
}

bool MBVisualsProcessor::projectorOpen() const { return projector != nullptr; }

//==============================================================================
void MBVisualsProcessor::loadStateProps()
{
    auto& st = apvts.state;
    static const char* keys[4] = { state::palC1, state::palC2, state::palC3, state::monoColor };
    for (int i = 0; i < 4; ++i)
        if (st.hasProperty (keys[i])) palCols[i] = juce::Colour::fromString (st.getProperty (keys[i]).toString());
    customCodes[0] = st.getProperty (state::codeA, "").toString();
    customCodes[1] = st.getProperty (state::codeB, "").toString();
    projDisplay = (int) st.getProperty (state::projDisplay, -1);
    outW = (int) st.getProperty (state::outW, 1280); outH = (int) st.getProperty (state::outH, 720);
    renderer.setOutputSize (outW, outH);
    {
        const juce::ScopedLock sl (midiLock);
        midiMap.clear();
        auto mm = st.getChildWithName ("MidiMap");
        for (int i = 0; i < mm.getNumChildren(); ++i)
        {
            auto c = mm.getChild (i);
            midiMap[(int) c.getProperty ("key")] = c.getProperty ("param").toString();
        }
    }
}

void MBVisualsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto& st = apvts.state;
    st.setProperty (state::outW, outW.load(), nullptr); st.setProperty (state::outH, outH.load(), nullptr);
    {
        const juce::ScopedLock sl (midiLock);
        st.removeChild (st.getChildWithName ("MidiMap"), nullptr);
        juce::ValueTree mm ("MidiMap");
        for (auto& kv : midiMap) { juce::ValueTree c ("Map"); c.setProperty ("key", kv.first, nullptr); c.setProperty ("param", kv.second, nullptr); mm.appendChild (c, nullptr); }
        st.appendChild (mm, nullptr);
    }
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void MBVisualsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadStateProps();
            renderer.setPaletteColours (palCols[0], palCols[1], palCols[2], palCols[3]);
            shadersDirty = true;
        }
}

juce::AudioProcessorEditor* MBVisualsProcessor::createEditor() { return new MBVisualsEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MBVisualsProcessor(); }
