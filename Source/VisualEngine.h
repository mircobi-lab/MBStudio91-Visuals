#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "AudioFeed.h"
#include "Params.h"

/*  Motore "logico" dei visual: analisi audio per i fotogrammi, clock dei beat, stima del BPM, automazione
    del crossfader, spin dei deck. Gira sul thread di rendering (una chiamata a update() per fotogramma) e
    pubblica in atomici i valori che l'interfaccia mostra. Porta fedele della logica JavaScript di MBVX. */
class VisualEngine
{
public:
    static constexpr int audioW = 512, histW = 128, histH = 64, fftOrder = 11, fftSize = 1 << fftOrder;
    static constexpr double histRate = 30.0;

    struct Frame
    {
        double dt = 0, vb = 0;
        float phase = 0, bar = 0, pulse = 0, bpm = 120;
        float bass = 0, mid = 0, high = 0, level = 0;
        float xf = 0;
        float deckRot[2] { 0, 0 };
        double deckTime[2] { 0, 0 };
        int histHead = 0; float histFrac = 0; int histPushes = 0;
    };

    VisualEngine (juce::AudioProcessorValueTreeState& apvts, AudioFeed& feed);

    // Da chiamare una volta per fotogramma sul thread di rendering
    const Frame& update (double nowSeconds);

    // Dati per le texture (validi dopo update)
    const juce::uint8* audioTex() const   { return tex.data(); }        // 2 righe x 512: spettro log, forma d'onda
    const juce::uint8* auxTex() const     { return aux.data(); }        // 3 righe x 512: min onda, max onda, picchi
    const juce::uint8* histRow (int k) const { return histRows[(size_t) k].data(); }   // righe spinte in questo frame

    // Azioni (da qualsiasi thread: diventano effettive al fotogramma successivo)
    void tap()                             { pendingTap = true; }
    void downbeat()                        { pendingDownbeat = true; }
    void goTo (int deck)                   { pendingGo = deck; }
    void stopXf()                          { xfAnim.active = false; }

    // Letture per l'interfaccia
    std::atomic<float> uiBpm { 120 }, uiBass { 0 }, uiMid { 0 }, uiHigh { 0 }, uiLevel { 0 }, uiOnset { 0 }, uiPhase { 0 }, uiBar { 0 }, uiFps { 0 };
    std::atomic<int> uiSource { 2 };       // 0 host, 1 audio, 2 interno, 3 tap
    std::atomic<bool> uiXfRunning { false };
    std::atomic<int> uiXfTarget { 0 };
    juce::String bpmInfo;                  // solo lettura dal thread dei messaggi (testo informativo, tollerante)

    // Qualità automatica: fattore di scala dei deck (1, .75, .5, .35)
    float qualityScale() const             { return qScale; }
    void reportFps (float fps);
    void setResolution (int w, int h)      { resW = w; resH = h; }

private:
    void analyse (double t);
    void updateAux (double t);
    void bpmFromOnsets (double t);
    struct Estimate { double bpm = 0, coarse = 0, jitter = -1; int beats = 0; bool ok = false; };
    Estimate estimate() const;
    Estimate refine (double coarse) const;
    void applyEstimate (const Estimate&);
    void clockOnOnset (double age);
    float paramF (const juce::String& id) const { if (auto* p = apvts.getRawParameterValue (id)) return p->load(); return 0.f; }
    bool momentary (const char* id, bool& prev);

    juce::AudioProcessorValueTreeState& apvts;
    AudioFeed& feed;
    Frame fr;
    double lastT = -1, auxT = 0;

    // analizzatore (porta dell'AnalyserNode: fftSize 2048, smoothing 0.2, -100..-30 dB, finestra Blackman)
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window, fftBuf, smoothed, samples;
    std::vector<juce::uint8> freqByte, timeByte;
    std::array<juce::uint8, audioW * 2> tex {};
    std::array<juce::uint8, audioW * 3> aux {};
    std::array<float, audioW> peakTop {}, peakAge {};
    std::vector<float> specMap; double specMapHz = 0;
    double binHz = 23.4;
    float bands[4] {}, peaks[4] { .2f, .2f, .2f, .2f };

    // storia dello spettro
    double histAcc = 0; int histHead = 0;
    std::array<std::array<juce::uint8, histW>, 4> histRows {};

    // colpi e BPM
    std::vector<double> onsets; double lastOnset = 0, lastEst = 0, pendingBpm = 0; float onsetFlash = 0;
    double fallbackPrevE = 0; std::vector<double> fallbackHist;

    // clock
    struct Clock { double bpm = 120, beat = 0; int source = 2; bool autoBpm = true; double foldLo = 88; std::vector<double> taps; double barOffset = 0, offsetMs = 0; } clock;
    double lastTapTime = -10; bool hostLocked = false;

    // crossfader automatico
    struct { bool active = false; double from = 0, to = 0, t0 = 0, dur = 0; } xfAnim;
    double xfValue = 0; float lastXfParam = -1;

    // spin dei deck
    struct Spin { double ang = 0, off = 0, key = 0; bool hasKey = false; } spin[2];

    std::atomic<bool> pendingTap { false }, pendingDownbeat { false };
    std::atomic<int> pendingGo { -1 };
    bool prevTap = false, prevDown = false, prevGoA = false, prevGoB = false;

    // qualità
    float qScale = 1.f; int qLow = 0, qHigh = 0, qUpWait = 10, qSinceUp = 99, qBlock = 0; float qRef = 60; struct { bool on = false; float fps = 0, from = 1; int n = 0; } qProbe;
    int resW = 1280, resH = 720;
};
