#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>

/*  Uscita luci: l'immagine d'uscita, ridotta a una griglia di 256x144, viene divisa in zone; ogni apparecchio
    (LED wall, barra pixel, PAR, strobo) prende i colori della sua zona e li scrive nei suoi canali DMX.
    Invio in rete: Art-Net (UDP 6454), sACN / E1.31 (UDP 5568, multicast o diretto), OSC (beat, BPM, bande).
    Il formato della patch (JSON) è lo stesso di MBVX nel browser: una patch si può passare dall'uno all'altro.

    Thread: la configurazione si cambia dal thread dei messaggi; processGrid() gira sul thread OpenGL. */
class LightsEngine
{
public:
    static constexpr int gridW = 256, gridH = 144;

    struct Fixture
    {
        juce::String id, type { "wall" }, name { "Apparecchio" }, ch { "[R G B]" }, flash { "strobe" };
        float x = 0, y = 0, w = .2f, h = .2f, dim = 1;
        int cols = 1, rows = 1, universe = 1, address = 1;
        bool whole = true, serp = false, vert = false, flipX = false, flipY = false;
    };
    struct Config
    {
        bool on = false, blackout = false, osc = false;
        int fps = 40, oscPort = 7700;
        juce::String proto { "artnet" }, ip { "127.0.0.1" }, oscHost { "127.0.0.1" };
        std::vector<Fixture> fixtures;
    };
    struct FrameInfo { double vb = 0; float phase = 0, bpm = 120, bass = 0, mid = 0, high = 0, level = 0, strobeRate = 1; bool strobe = false, black = false; };
    struct Info { int count = 0, pixels = 0, endUniverse = 0, endAddress = 0; bool overlap = false; };
    struct Stats { bool on = false; int fixtures = 0, channels = 0, universes = 0, packetsPerSecond = 0; juce::String error; };

    LightsEngine();

    // ---- thread dei messaggi
    Config config() const;
    void setConfig (const Config&);
    static Fixture makeFixture (const juce::String& type);
    static Fixture cleanFixture (Fixture);
    juce::String toJson() const;
    bool loadJson (const juce::String& json, juce::String& error);
    std::vector<Info> info() const;
    void identify (const juce::String& fixtureId, double seconds = 3.0);
    juce::Image preview() const;                       // ultima griglia campionata (per l'editor delle zone)
    Stats stats() const;

    // ---- thread OpenGL
    bool wantsCapture (double nowSeconds);             // true quando è ora di campionare l'uscita
    void processGrid (const juce::uint8* rgbaBottomUp, const FrameInfo&);

private:
    struct Plan
    {
        Fixture f;
        int zone[4] {};                                // x0, x1, y0, y1 nella griglia (y dall'alto)
        std::vector<int> cells;                        // 4 valori per pixel
        int pixels = 0;
        std::vector<int> U, A, T, X; std::vector<juce::uint8> W;
    };
    struct Patch { Config cfg; std::vector<Plan> plans; std::vector<Info> info; std::vector<int> universes; int channels = 0; };

    static std::shared_ptr<Patch> build (const Config&);
    std::shared_ptr<Patch> current() const;
    void send (const Patch&, const FrameInfo&);

    mutable juce::SpinLock patchLock;
    std::shared_ptr<Patch> patch;

    std::map<int, std::array<juce::uint8, 512>> unis;
    juce::DatagramSocket socket { true };
    juce::uint8 cid[16] {};
    juce::uint8 artSeq = 1;
    std::map<int, juce::uint8> sacnSeq;
    double lastCapture = 0;
    int oscBeat = -1, oscStrobe = -1; double oscT = 0;

    mutable juce::CriticalSection previewLock;
    juce::Image previewImg { juce::Image::RGB, gridW, gridH, true };
    int previewCounter = 0;

    juce::CriticalSection identLock;
    juce::String identId; double identUntil = 0;

    std::atomic<int> packetCount { 0 }, packetsPerSecond { 0 };
    double statsT = 0;
    juce::CriticalSection errLock; juce::String lastError;
};
