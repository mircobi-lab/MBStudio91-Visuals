#include "VisualEngine.h"
#include <cmath>

using namespace mbv;

VisualEngine::VisualEngine (juce::AudioProcessorValueTreeState& s, AudioFeed& f) : apvts (s), feed (f)
{
    window.resize (fftSize); fftBuf.resize (fftSize * 2); smoothed.assign (fftSize / 2, 0.f); samples.resize (fftSize);
    freqByte.assign (fftSize / 2, 0); timeByte.assign (fftSize, 128);
    for (int i = 0; i < fftSize; ++i)
    {
        // finestra di Blackman come nel Web Audio AnalyserNode (a0 = .42, a1 = .5, a2 = .08)
        const double x = (double) i / fftSize;
        window[(size_t) i] = (float) (0.42 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * x) + 0.08 * std::cos (4 * juce::MathConstants<double>::pi * x));
    }
    for (int i = 0; i < audioW; ++i) { aux[(size_t) i] = 128; aux[(size_t) (audioW + i)] = 128; }
}

bool VisualEngine::momentary (const char* id, bool& prev)
{
    const bool now = paramF (id) > .5f;
    const bool edge = now && ! prev; prev = now;
    if (now)
        if (auto* p = apvts.getParameter (id))
            juce::MessageManager::callAsync ([p] { p->setValueNotifyingHost (0.f); });
    return edge;
}

const VisualEngine::Frame& VisualEngine::update (double now)
{
    const double realDt = lastT < 0 ? 1.0 / 60 : juce::jlimit (0.0, 1.0, now - lastT);
    const double dt = juce::jmin (0.1, realDt);
    lastT = now;
    fr.dt = dt;

    // ---- azioni momentanee (pulsanti dell'interfaccia o parametri automatizzabili)
    if (momentary (ids::tap, prevTap) || pendingTap.exchange (false))
    {
        auto& c = clock;
        if (! c.taps.empty() && now - c.taps.back() > 2) c.taps.clear();
        c.taps.push_back (now); if (c.taps.size() > 8) c.taps.erase (c.taps.begin());
        if (c.taps.size() >= 2)
        {
            const double n = (double) c.taps.size();
            const double bpm = 60.0 / ((c.taps.back() - c.taps.front()) / (n - 1));
            c.bpm = juce::jlimit (40.0, 240.0, bpm); lastTapTime = now;
            if (auto* p = apvts.getParameter (ids::bpm))
            {
                const float v = p->convertTo0to1 ((float) c.bpm);
                juce::MessageManager::callAsync ([p, v] { p->setValueNotifyingHost (v); });
            }
        }
        const double b = c.beat + c.offsetMs / 1000 * c.bpm / 60 - c.barOffset;
        c.beat += std::round (b) - b;
    }
    if (momentary (ids::downbeat, prevDown) || pendingDownbeat.exchange (false))
    {
        const double b = clock.beat + clock.offsetMs / 1000 * clock.bpm / 60;
        const double m = std::fmod (std::fmod (b, 4.0) + 4.0, 4.0);
        clock.barOffset = hostLocked ? std::fmod (std::round (m), 4.0) : m;
    }
    {
        const int go = pendingGo.exchange (-1);
        const bool a = momentary (ids::goA, prevGoA) || go == 0, b = momentary (ids::goB, prevGoB) || go == 1;
        if (a || b)
        {
            const double to = b ? 1.0 : 0.0, dur = paramF (ids::xfTime) * std::abs (to - xfValue);
            if (dur < .016) { xfAnim.active = false; xfValue = to; }
            else { xfAnim = { true, xfValue, to, now, dur }; }
            uiXfTarget = (int) to;
        }
    }

    // ---- crossfader: parametro dell'host o animazione
    {
        const float xp = paramF (ids::xf);
        if (lastXfParam >= 0 && std::abs (xp - lastXfParam) > 1e-4f) { xfAnim.active = false; xfValue = xp; }   // mosso dall'host/utente
        lastXfParam = xp;
        if (xfAnim.active)
        {
            double k = (now - xfAnim.t0) / xfAnim.dur;
            if (k >= 1) { k = 1; xfAnim.active = false; }
            const double e = k < .5 ? 2 * k * k : 1 - std::pow (-2 * k + 2, 2) / 2;
            xfValue = xfAnim.from + (xfAnim.to - xfAnim.from) * e;
            if (auto* p = apvts.getParameter (ids::xf))
            {
                const float v = (float) xfValue;
                juce::MessageManager::callAsync ([p, v] { p->setValueNotifyingHost (v); });
                lastXfParam = v;
            }
        }
        else xfValue = xp;
        uiXfRunning = xfAnim.active;
        fr.xf = (float) xfValue;
    }

    // ---- analisi audio
    analyse (now);

    // ---- clock
    {
        const int src = (int) paramF (ids::clockSrc);
        auto h = feed.host();
        clock.offsetMs = paramF (ids::beatOffset);
        hostLocked = false;
        if (src == 0 && (h.hasBpm || h.hasPpq))
        {
            if (h.hasBpm && h.bpm > 20) clock.bpm = juce::jlimit (40.0, 240.0, h.bpm);
            if (h.hasPpq)
            {
                // posizione in quarti dell'host + tempo trascorso dall'ultimo blocco audio (fluidità tra i blocchi)
                const double since = juce::jmax (0.0, feed.audioTimeNow() - h.atSample / feed.sampleRate());
                clock.beat = h.ppq + (h.playing ? since * clock.bpm / 60 : 0);
                hostLocked = true;
            }
            else clock.beat += realDt * clock.bpm / 60;
            clock.source = 0; clock.autoBpm = false;
        }
        else
        {
            if (src == 1 || src == 0) { clock.autoBpm = true; clock.source = feed.signalPresent() ? 1 : 2; }   // senza host: rilevamento dall'audio
            else
            {
                clock.autoBpm = false;
                clock.bpm = paramF (ids::bpm);
                clock.source = (now - lastTapTime < 2.0) ? 3 : 2;
            }
            clock.beat += realDt * clock.bpm / 60;
        }
        const double vb = clock.beat + clock.offsetMs / 1000 * clock.bpm / 60 - clock.barOffset;
        fr.vb = vb;
        fr.phase = (float) (std::fmod (std::fmod (vb, 1.0) + 1.0, 1.0));
        fr.bar = (float) (std::fmod (std::fmod (vb / 4, 1.0) + 1.0, 1.0));
        fr.pulse = std::pow (1.f - fr.phase, 4.f);
        fr.bpm = (float) clock.bpm;
        uiBpm = fr.bpm; uiSource = clock.source; uiPhase = fr.phase; uiBar = fr.bar;
    }

    // ---- deck: tempo e spin
    constexpr double TAU = juce::MathConstants<double>::twoPi;
    static const double SPIN_DIV[] = { 32, 16, 8, 4, 2, 1, 0.5 };
    for (int d = 0; d < 2; ++d)
    {
        fr.deckTime[d] += dt * paramF (ids::deck (d, ids::speed));
        const double sv = paramF (ids::deck (d, ids::spin));
        const bool dead = std::abs (sv) < .03;
        auto& S = spin[d];
        if (paramF (ids::deck (d, ids::spinTempo)) > .5f)
        {
            double bpt = 0;
            if (! dead) bpt = SPIN_DIV[juce::jlimit (0, 6, (int) std::ceil (std::abs (sv) * 7) - 1)];
            const double dir = sv > 0 ? 1 : -1, key = bpt * dir;
            const double lock = bpt > 0 ? dir * TAU * fr.vb / bpt : 0;
            if (! S.hasKey || std::abs (key - S.key) > 1e-9) { S.off = S.ang - lock; S.key = key; S.hasKey = true; }
            S.ang = lock + S.off;
        }
        else
        {
            S.hasKey = false;
            const double revs = dead ? 0 : (sv > 0 ? 1 : -1) * sv * sv * 2;
            S.ang = std::fmod (S.ang + dt * revs * TAU, TAU);
        }
        const double rot = paramF (ids::deck (d, ids::rot)) * juce::MathConstants<double>::pi / 180 + S.ang;
        fr.deckRot[d] = (float) std::fmod (std::fmod (rot, TAU) + TAU, TAU);
    }
    return fr;
}

void VisualEngine::analyse (double t)
{
    const double sr = feed.sampleRate();
    binHz = sr / fftSize;
    float* B = bands;
    if (! feed.signalPresent())
    {
        for (int k = 0; k < 4; ++k) B[k] *= .9f;
        std::fill (tex.begin(), tex.begin() + audioW, (juce::uint8) 0);
        std::fill (tex.begin() + audioW, tex.end(), (juce::uint8) 128);
        for (auto& s : smoothed) s *= .8f;
    }
    else
    {
        feed.copyLatest (samples.data(), fftSize);
        for (int i = 0; i < fftSize; ++i) { fftBuf[(size_t) i] = samples[(size_t) i] * window[(size_t) i]; }
        std::fill (fftBuf.begin() + fftSize, fftBuf.end(), 0.f);
        fft.performRealOnlyForwardTransform (fftBuf.data(), true);
        constexpr float minDb = -100.f, maxDb = -30.f, smoothing = .2f, scale = 255.f / (maxDb - minDb);
        for (int k = 0; k < fftSize / 2; ++k)
        {
            const float re = fftBuf[(size_t) (2 * k)], im = fftBuf[(size_t) (2 * k + 1)];
            const float mag = std::sqrt (re * re + im * im) / (float) fftSize;
            float& s = smoothed[(size_t) k]; s = smoothing * s + (1.f - smoothing) * mag;
            const float db = 20.f * std::log10 (s + 1e-20f);
            freqByte[(size_t) k] = (juce::uint8) juce::jlimit (0, 255, (int) (scale * (db - minDb)));
        }
        for (int i = 0; i < fftSize; ++i) timeByte[(size_t) i] = (juce::uint8) juce::jlimit (0, 255, (int) std::lround (128.f * (1.f + samples[(size_t) i])));

        auto band = [&] (double lo, double hi)
        {
            const int a = juce::jmax (1, (int) std::floor (lo / binHz)), b = juce::jmin (fftSize / 2 - 1, (int) std::ceil (hi / binHz));
            double s = 0; for (int i = a; i <= b; ++i) s += freqByte[(size_t) i];
            return (float) (s / ((b - a + 1) * 255.0));
        };
        double rms = 0; for (int i = 0; i < fftSize; i += 4) { const double v = samples[(size_t) i]; rms += v * v; }
        const float raw[4] = { band (20, 150), band (150, 2000), band (2000, 10000), (float) std::sqrt (rms / (fftSize / 4)) };
        for (int k = 0; k < 4; ++k)
        {
            peaks[k] = juce::jmax (raw[k], peaks[k] * .9985f, .03f);
            const float n = juce::jmin (1.f, raw[k] / peaks[k]);
            B[k] += (n - B[k]) * (n > B[k] ? .6f : .15f);
        }
        if ((int) specMap.size() != audioW || std::abs (specMapHz - binHz) > 1e-9)
        {
            specMap.resize (audioW); specMapHz = binHz;
            for (int i = 0; i < audioW; ++i) specMap[(size_t) i] = (float) juce::jmin ((double) (fftSize / 2 - 2), 40.0 * std::pow (16000.0 / 40.0, i / (audioW - 1.0)) / binHz);
        }
        for (int i = 0; i < audioW; ++i)
        {
            const float x = specMap[(size_t) i]; const int k = (int) x; const float frac = x - (float) k;
            tex[(size_t) i] = (juce::uint8) (freqByte[(size_t) k] * (1 - frac) + freqByte[(size_t) (k + 1)] * frac);
        }
        // forma d'onda con trigger sul fronte di salita (isteresi)
        int start = 0; bool armed = false;
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const int v = timeByte[(size_t) i];
            if (v < 124) armed = true; else if (armed && v >= 128) { start = i; break; }
        }
        for (int k = 0; k < audioW; ++k) tex[(size_t) (audioW + k)] = timeByte[(size_t) juce::jmin (fftSize - 1, start + k * 2)];
    }
    updateAux (t);

    // storia dello spettro: una riga ogni 1/30 s
    histAcc += fr.dt * histRate;
    fr.histPushes = 0;
    while (histAcc >= 1 && fr.histPushes < 4)
    {
        histAcc -= 1;
        histHead = (histHead + 1) % histH;
        auto& r = histRows[(size_t) fr.histPushes++];
        constexpr int step = audioW / histW;
        for (int j = 0; j < histW; ++j) { juce::uint8 m = 0; for (int q = 0; q < step; ++q) m = juce::jmax (m, tex[(size_t) (j * step + q)]); r[(size_t) j] = m; }
    }
    if (histAcc >= 1) histAcc = std::fmod (histAcc, 1.0);
    fr.histHead = histHead; fr.histFrac = (float) juce::jmin (histAcc, .999);

    // colpi dal thread audio e stima del BPM
    double buf[AudioFeed::onsetSize];
    const int n = feed.popOnsets (buf, AudioFeed::onsetSize);
    const double nowAudio = feed.audioTimeNow();
    for (int i = 0; i < n; ++i)
    {
        const double ta = buf[i];
        if (ta - lastOnset < .25) continue;
        lastOnset = ta; onsetFlash = 1;
        onsets.push_back (ta); while (! onsets.empty() && ta - onsets.front() > 20) onsets.erase (onsets.begin());
        clockOnOnset (juce::jmax (0.0, nowAudio - ta) + .012);
    }
    onsetFlash *= .85f;
    if (t - lastEst > .5 && onsets.size() >= 8) { lastEst = t; applyEstimate (estimate()); }

    const float R = paramF (ids::react);
    fr.bass = juce::jmin (1.f, B[0] * R); fr.mid = juce::jmin (1.f, B[1] * R); fr.high = juce::jmin (1.f, B[2] * R); fr.level = juce::jmin (1.f, B[3] * R);
    uiBass = B[0]; uiMid = B[1]; uiHigh = B[2]; uiLevel = feed.signalPresent() ? B[3] : 0.f; uiOnset = onsetFlash;
}

void VisualEngine::updateAux (double t)
{
    constexpr int W = audioW, R = 16;
    const float dt = (float) juce::jlimit (0.0, 0.1, t - auxT); auxT = t;
    for (int i = 0; i < W; ++i)
    {
        juce::uint8 mn = 255, mx = 0;
        const int j0 = juce::jmax (0, i - R), j1 = juce::jmin (W - 1, i + R);
        for (int j = j0; j <= j1; ++j) { const auto v = tex[(size_t) (W + j)]; mn = juce::jmin (mn, v); mx = juce::jmax (mx, v); }
        aux[(size_t) i] = mn; aux[(size_t) (W + i)] = mx;
        const float age = peakAge[(size_t) i] + dt, pv = peakTop[(size_t) i] * juce::jmax (0.f, 1.f - .75f * age), sv = tex[(size_t) i];
        if (sv >= pv) { peakTop[(size_t) i] = sv; peakAge[(size_t) i] = 0; aux[(size_t) (2 * W + i)] = (juce::uint8) sv; }
        else { peakAge[(size_t) i] = age; aux[(size_t) (2 * W + i)] = (juce::uint8) pv; }
    }
}

void VisualEngine::clockOnOnset (double age)
{
    if (! clock.autoBpm || hostLocked) return;
    const double b = clock.beat - age * clock.bpm / 60;
    const double frac = b - std::round (b);
    clock.beat -= frac * (std::abs (frac) < .22 ? .3 : .12);
}

VisualEngine::Estimate VisualEngine::estimate() const
{
    const double lo = clock.foldLo, hi = lo * 2;
    std::vector<double> H ((size_t) std::ceil (hi - lo) + 1, 0.0);
    const auto& on = onsets; const int n = (int) on.size(); const double tNow = on.back();
    int i0 = 0; while (i0 < n && tNow - on[(size_t) i0] > 8) ++i0;
    for (int i = i0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
        {
            const double dt = on[(size_t) j] - on[(size_t) i]; if (dt > 2.2) break; if (dt < .25) continue;
            double b = 60 / dt; while (b < lo) b *= 2; while (b >= hi) b /= 2;
            const double w = std::exp (-(tNow - on[(size_t) j]) / 6), x = b - lo; const int k = (int) std::round (x);
            for (int d = -2; d <= 2; ++d) { const int kk = k + d; if (kk >= 0 && kk < (int) H.size()) H[(size_t) kk] += w * std::exp (-((kk - x) * (kk - x)) / 1.5); }
        }
    int k = 0; double sum = 0; for (int i = 0; i < (int) H.size(); ++i) { sum += H[(size_t) i]; if (H[(size_t) i] > H[(size_t) k]) k = i; }
    if (sum <= 0 || H[(size_t) k] / sum < .12) return {};
    const double a = k > 0 ? H[(size_t) (k - 1)] : 0, c = k + 1 < (int) H.size() ? H[(size_t) (k + 1)] : 0, den = a - 2 * H[(size_t) k] + c;
    const double delta = std::abs (den) > 1e-12 ? .5 * (a - c) / den : 0;
    return refine (lo + k + juce::jlimit (-.5, .5, delta));
}

VisualEngine::Estimate VisualEngine::refine (double coarse) const
{
    const auto& on = onsets; const int n = (int) on.size();
    double P = 60 / coarse, anchor = on.back(), jitter = -1; int beats = 0;
    for (double span : { 4.0, 8.0, 14.0, 20.0 })
    {
        std::vector<double> ks, ts;
        for (int i = n - 1; i >= 0; --i)
        {
            const double t = on[(size_t) i]; if (anchor - t > span) break;
            const double k = std::round ((anchor - t) / P), r = (anchor - t) - k * P;
            if (std::abs (r) < .1 * P) { ks.push_back (k); ts.push_back (t); }
        }
        const int c = (int) ks.size(); if (c < 6) break;
        double sk = 0, st = 0, skk = 0, skt = 0;
        for (int j = 0; j < c; ++j) { sk += ks[(size_t) j]; st += ts[(size_t) j]; skk += ks[(size_t) j] * ks[(size_t) j]; skt += ks[(size_t) j] * ts[(size_t) j]; }
        const double den = c * skk - sk * sk; if (den <= 0) break;
        const double slope = (c * skt - sk * st) / den, a = (st - slope * sk) / c, Pn = -slope;
        if (! (Pn > 0) || std::abs (Pn - P) / P > .03) break;
        P = Pn; anchor = a; beats = c;
        double e2 = 0; for (int j = 0; j < c; ++j) { const double d = ts[(size_t) j] - (a - ks[(size_t) j] * P); e2 += d * d; }
        jitter = std::sqrt (e2 / c);
    }
    Estimate e; e.ok = true; e.bpm = 60 / P; e.coarse = coarse; e.beats = beats; e.jitter = jitter;
    return e;
}

void VisualEngine::applyEstimate (const Estimate& est)
{
    if (! est.ok || ! clock.autoBpm || hostLocked) return;
    clock.source = 1;
    const double b = est.bpm; const bool good = est.beats >= 8 && est.jitter >= 0 && est.jitter < .02 * 60 / b;
    bpmInfo = "BPM dall'audio: " + juce::String (b, 2) + (est.beats ? " - " + juce::String (est.beats) + " battiti in griglia - scarto " + juce::String (est.jitter * 1000, 1) + " ms" : " - stima grezza") + (good ? " - affidabile" : " - in assestamento");
    if (std::abs (b - clock.bpm) < 1.5) clock.bpm += (b - clock.bpm) * (good ? .5 : .15);
    else if (pendingBpm > 0 && std::abs (pendingBpm - b) < 1) { clock.bpm = juce::jlimit (40.0, 240.0, b); pendingBpm = 0; }
    else pendingBpm = b;
}

void VisualEngine::reportFps (float fps)
{
    uiFps = fps;
    const int qmode = (int) paramF (ids::quality);
    static const float QSTEPS[] = { 1.f, .75f, .5f, .35f };
    if (qmode != 0) { qScale = QSTEPS[juce::jlimit (0, 3, qmode - 1)]; return; }
    int i = 0; for (int k = 0; k < 4; ++k) if (std::abs (QSTEPS[k] - qScale) < 1e-3f) i = k;
    ++qSinceUp;
    if (fps > qRef * 1.1f) qRef = juce::jmin (60.f, fps);
    if (qProbe.on)
    {
        if (++qProbe.n < 3) return;
        const float gain = fps / juce::jmax (1.f, qProbe.fps), from = qProbe.from; qProbe.on = false;
        if (gain < 1.08f) { qRef = juce::jmax (15.f, fps); qScale = from; qLow = qHigh = 0; qBlock = 60; return; }
    }
    if (qBlock > 0) --qBlock;
    if (fps < qRef * .88f)
    {
        qHigh = 0;
        if (++qLow >= 2 && i < 3 && qBlock == 0)
        {
            if (qSinceUp < 8) qUpWait = juce::jmin (120, qUpWait * 2);
            qProbe = { true, fps, qScale, 0 }; qScale = QSTEPS[i + 1]; qLow = 0;
        }
    }
    else if (fps >= qRef * .95f)
    {
        qLow = 0;
        if (++qHigh >= qUpWait && i > 0) { qScale = QSTEPS[i - 1]; qHigh = 0; qSinceUp = 0; }
    }
    else { qLow = 0; qHigh = 0; }
}
