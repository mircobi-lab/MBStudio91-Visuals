#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>

/*  Ponte tra il thread audio e il motore visuale (thread dei messaggi / GL).
    - gli ultimi campioni mono finiscono in un anello senza lock (2048 campioni letti dall'analizzatore);
    - il rilevatore di colpi (cassa) lavora qui, a blocchi di 128 campioni, con la precisione del clock audio;
    - la posizione dell'host (BPM, posizione in quarti) viene copiata in atomici.
    Porta fedele del rilevatore AudioWorklet di MBVX: energia su 4 blocchi, flusso logaritmico rispetto a
    4 blocchi prima, soglia adattiva media + 2,5 deviazioni (k = 1/512), picco del flusso, min 0,25 s. */
class AudioFeed
{
public:
    static constexpr int ringSize  = 1 << 14;   // 16384 campioni (potenza di 2)
    static constexpr int onsetSize = 64;

    void prepare (double sampleRate);
    void process (const float* const* channels, int numChannels, int numSamples);

    // Thread di lettura: copia gli ultimi n campioni (il più recente per ultimo)
    void copyLatest (float* dest, int n) const;
    // Istanti dei colpi (in secondi dal prepare) non ancora letti; restituisce quanti ne ha scritti
    int popOnsets (double* dest, int maxN);

    double sampleRate() const noexcept        { return sr; }
    double audioTimeNow() const noexcept      { return (double) totalSamples.load (std::memory_order_relaxed) / sr; }
    bool   signalPresent() const noexcept     { return present.load (std::memory_order_relaxed); }

    // Posizione host (scritta da processBlock)
    struct HostInfo { double bpm = 0, ppq = 0; bool playing = false, hasBpm = false, hasPpq = false; double atSample = 0; };
    void setHost (const HostInfo& h)          { hostInfo[hostWrite ^ 1] = h; hostWrite ^= 1; }
    HostInfo host() const                     { return hostInfo[hostWrite]; }

private:
    void onsetBlock (const float* lowpassed, int n);

    double sr = 48000.0;
    std::array<float, ringSize> ring {};
    std::atomic<int> writePos { 0 };
    std::atomic<juce::int64> totalSamples { 0 };
    std::atomic<bool> present { false };
    int silentBlocks = 0;

    // passa-basso 150 Hz Q 0.7 (biquad RBJ), stato
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    // rilevatore
    std::array<float, 128> blockBuf {};
    int blockFill = 0;
    float sq[4] {}; int si = 0;
    float lh[4] { -6, -6, -6, -6 }; int li = 0;
    float m = 0, v = 1e-4f; int n = 0;
    float cand = 0; double candT = 0, last = -1;
    juce::int64 blockStartSample = 0;

    std::array<double, onsetSize> onsets {};
    std::atomic<int> onsetWrite { 0 };
    int onsetRead = 0;

    HostInfo hostInfo[2];
    std::atomic<int> hostWrite { 0 };
};
