#include "AudioFeed.h"
#include <cmath>

void AudioFeed::prepare (double sampleRate)
{
    sr = sampleRate > 1000 ? sampleRate : 48000.0;
    // biquad passa-basso RBJ: f0 = 150 Hz, Q = 0.7 (come il BiquadFilterNode di MBVX)
    const double w0 = 2.0 * juce::MathConstants<double>::pi * 150.0 / sr, cw = std::cos (w0), sw = std::sin (w0), alpha = sw / (2.0 * 0.7);
    const double a0 = 1.0 + alpha;
    b0 = b2 = (float) ((1.0 - cw) / 2.0 / a0); b1 = (float) ((1.0 - cw) / a0);
    a1 = (float) (-2.0 * cw / a0); a2 = (float) ((1.0 - alpha) / a0);
    z1 = z2 = 0;
    ring.fill (0.0f); writePos = 0; totalSamples = 0;
    blockFill = 0; si = li = 0; for (auto& x : sq) x = 0; for (auto& x : lh) x = -6;
    m = 0; v = 1e-4f; n = 0; cand = 0; candT = 0; last = -1;
    onsetWrite = 0; onsetRead = 0; present = false; silentBlocks = 0;
}

void AudioFeed::process (const float* const* ch, int numCh, int numSamples)
{
    if (numCh <= 0 || numSamples <= 0) return;
    int wp = writePos.load (std::memory_order_relaxed);
    const float g = 1.0f / (float) numCh;
    float peak = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0;
        for (int c = 0; c < numCh; ++c) s += ch[c][i];
        s *= g;
        ring[(size_t) wp] = s; wp = (wp + 1) & (ringSize - 1);
        peak = std::max (peak, std::abs (s));
        // passa-basso (transposed direct form II)
        const float y = b0 * s + z1;
        z1 = b1 * s - a1 * y + z2;
        z2 = b2 * s - a2 * y;
        blockBuf[(size_t) blockFill++] = y;
        if (blockFill == 128)
        {
            blockStartSample = totalSamples.load (std::memory_order_relaxed) + i + 1 - 128;
            onsetBlock (blockBuf.data(), 128);
            blockFill = 0;
        }
    }
    writePos.store (wp, std::memory_order_release);
    totalSamples.fetch_add (numSamples, std::memory_order_relaxed);
    if (peak > 1e-4f) { silentBlocks = 0; present.store (true, std::memory_order_relaxed); }
    else if (++silentBlocks > (int) (sr / numSamples)) present.store (false, std::memory_order_relaxed);   // ~1 s di silenzio
}

void AudioFeed::onsetBlock (const float* x, int len)
{
    float a = 0; for (int i = 0; i < len; ++i) a += x[i] * x[i];
    sq[si] = a / (float) len; si = (si + 1) & 3;
    const float e = std::sqrt ((sq[0] + sq[1] + sq[2] + sq[3]) * .25f);
    const float le = std::log10 (1e-6f + e), old = lh[li]; lh[li] = le; li = (li + 1) & 3;
    const float flux = std::max (0.0f, le - old);
    const float k = 1.0f / 512.0f, d = flux - m; m += k * d; v += k * (d * d - v); ++n;
    const float thr = m + 2.5f * std::sqrt (v);
    const double t = (double) (blockStartSample + len) / sr;
    if (cand > 0)
    {
        if (flux > cand) { cand = flux; candT = t; }
        else if (flux < cand * .9f || t - candT > .03)
        {
            if (candT - last > .25)
            {
                last = candT;
                const int w = onsetWrite.load (std::memory_order_relaxed);
                onsets[(size_t) (w % onsetSize)] = candT;
                onsetWrite.store (w + 1, std::memory_order_release);
            }
            cand = 0;
        }
    }
    else if (n > 256 && e > .004f && flux > thr && flux > .03f) { cand = flux; candT = t; }
}

void AudioFeed::copyLatest (float* dest, int count) const
{
    const int wp = writePos.load (std::memory_order_acquire);
    int start = (wp - count) & (ringSize - 1);
    for (int i = 0; i < count; ++i) { dest[i] = ring[(size_t) start]; start = (start + 1) & (ringSize - 1); }
}

int AudioFeed::popOnsets (double* dest, int maxN)
{
    const int w = onsetWrite.load (std::memory_order_acquire);
    if (w - onsetRead > onsetSize) onsetRead = w - onsetSize;    // il lettore è rimasto indietro
    int k = 0;
    while (onsetRead < w && k < maxN) dest[k++] = onsets[(size_t) (onsetRead++ % onsetSize)];
    return k;
}
