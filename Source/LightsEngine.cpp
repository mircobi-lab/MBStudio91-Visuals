#include "LightsEngine.h"
#include <cmath>
#include <set>

namespace
{
    enum Tok { tNone = 0, tR, tG, tB, tW, tC, tM, tY, tD, tL, tF, tS };
    int tokenOf (const juce::String& s)
    {
        if (s.containsOnly ("0123456789")) return -juce::jlimit (0, 255, s.getIntValue()) - 1;   // valore fisso
        static const char* names = "RGBWCMYDLFS";
        if (s.length() == 1) { const auto* p = std::strchr (names, (char) s[0]); if (p != nullptr) return (int) (p - names) + 1; }
        return tNone;
    }
    std::vector<int> tokens (const juce::String& part)
    {
        std::vector<int> t;
        for (auto& w : juce::StringArray::fromTokens (part.toUpperCase().replaceCharacter (',', ' '), " \t", ""))
            if (w.isNotEmpty()) t.push_back (tokenOf (w));
        return t;
    }
    double nowSec() { return juce::Time::getMillisecondCounterHiRes() * .001; }

    void oscAppend (juce::MemoryOutputStream& m, const char* addr, bool isInt, double v)
    {
        auto pad = [&m] (const char* s) { const auto n = std::strlen (s) + 1; m.write (s, n); for (size_t i = n; i % 4 != 0; ++i) m.writeByte (0); };
        pad (addr); pad (isInt ? ",i" : ",f");
        if (isInt) m.writeIntBigEndian ((int) v); else m.writeFloatBigEndian ((float) v);
    }
}

LightsEngine::LightsEngine()
{
    const juce::Uuid u; std::memcpy (cid, u.getRawData(), 16);
    patch = build ({});
}

//==============================================================================
LightsEngine::Fixture LightsEngine::makeFixture (const juce::String& type)
{
    Fixture f; f.type = type;
    if (type == "bar")         { f.name = "Barra pixel"; f.x = .05f; f.y = .82f; f.w = .9f; f.h = .1f; f.cols = 8; f.rows = 1; f.ch = "[R G B]"; }
    else if (type == "par")    { f.name = "PAR RGBW"; f.x = .4f; f.y = .35f; f.w = .2f; f.h = .3f; f.ch = "D R G B W 0 0 0"; }
    else if (type == "strobe") { f.name = "Strobo"; f.x = 0; f.y = 0; f.w = 1; f.h = 1; f.ch = "F 0"; }
    else                       { f.type = "wall"; f.name = "LED wall"; f.x = .1f; f.y = .1f; f.w = .8f; f.h = .8f; f.cols = 32; f.rows = 16; f.ch = "[R G B]"; }
    f.id = juce::Uuid().toString().substring (0, 8);
    return f;
}

LightsEngine::Fixture LightsEngine::cleanFixture (Fixture f)
{
    f.x = juce::jlimit (0.f, 1.f, f.x); f.y = juce::jlimit (0.f, 1.f, f.y);
    f.w = juce::jlimit (.005f, 1.f, f.w); f.h = juce::jlimit (.005f, 1.f, f.h);
    if (f.x + f.w > 1) f.w = juce::jmax (.005f, 1 - f.x);
    if (f.y + f.h > 1) f.h = juce::jmax (.005f, 1 - f.y);
    f.cols = juce::jlimit (1, 512, f.cols); f.rows = juce::jlimit (1, 512, f.rows);
    f.universe = juce::jlimit (0, 63999, f.universe); f.address = juce::jlimit (1, 512, f.address);
    f.dim = juce::jlimit (0.f, 1.f, f.dim);
    if (! juce::StringArray { "strobe", "beat", "bass", "level" }.contains (f.flash)) f.flash = "strobe";
    if (! juce::StringArray { "wall", "bar", "par", "strobe" }.contains (f.type)) f.type = "wall";
    if (f.id.isEmpty()) f.id = juce::Uuid().toString().substring (0, 8);
    f.name = f.name.substring (0, 40); f.ch = f.ch.substring (0, 400);
    return f;
}

std::shared_ptr<LightsEngine::Patch> LightsEngine::build (const Config& cfgIn)
{
    auto P = std::make_shared<Patch>();
    P->cfg = cfgIn;
    for (auto& f : P->cfg.fixtures) f = cleanFixture (f);
    std::set<int> universes;
    std::map<int, int> owner;                                  // canale -> apparecchio, per le sovrapposizioni
    for (size_t fi = 0; fi < P->cfg.fixtures.size(); ++fi)
    {
        const auto& f = P->cfg.fixtures[fi];
        Plan pl; pl.f = f;
        // canali: "D [R G B] S" -> testa, pixel ripetuti, coda
        juce::String head = f.ch, pix, tail;
        const int lb = f.ch.indexOfChar ('[');
        if (lb >= 0) { head = f.ch.substring (0, lb); const int rb = f.ch.indexOfChar (lb, ']'); pix = f.ch.substring (lb + 1, rb < 0 ? f.ch.length() : rb); tail = rb < 0 ? juce::String() : f.ch.substring (rb + 1); }
        const auto H = tokens (head), Px = tokens (pix), Tl = tokens (tail);
        pl.pixels = Px.empty() ? 0 : f.cols * f.rows;

        auto cell = [] (float x0, float y0, float x1, float y1, int* out)
        {
            auto gx = [] (float v) { return juce::jlimit (0.f, (float) gridW, v * gridW); };
            auto gy = [] (float v) { return juce::jlimit (0.f, (float) gridH, v * gridH); };
            int a = (int) std::floor (gx (x0)), b = (int) std::ceil (gx (x1)), c = (int) std::floor (gy (y0)), d = (int) std::ceil (gy (y1));
            if (b <= a) b = juce::jmin (gridW, a + 1);
            if (a >= gridW) a = gridW - 1;
            if (d <= c) d = juce::jmin (gridH, c + 1);
            if (c >= gridH) c = gridH - 1;
            out[0] = a; out[1] = b; out[2] = c; out[3] = d;
        };
        cell (f.x, f.y, f.x + f.w, f.y + f.h, pl.zone);
        pl.cells.resize ((size_t) pl.pixels * 4);
        for (int k = 0; k < pl.pixels; ++k)
        {
            int i, j;
            if (f.vert) { i = k / f.rows; j = k % f.rows; if (f.serp && (i & 1)) j = f.rows - 1 - j; }
            else        { j = k / f.cols; i = k % f.cols; if (f.serp && (j & 1)) i = f.cols - 1 - i; }
            if (f.flipX) i = f.cols - 1 - i;
            if (f.flipY) j = f.rows - 1 - j;
            cell (f.x + f.w * (float) i / (float) f.cols, f.y + f.h * (float) j / (float) f.rows,
                  f.x + f.w * (float) (i + 1) / (float) f.cols, f.y + f.h * (float) (j + 1) / (float) f.rows, pl.cells.data() + k * 4);
        }
        int u = f.universe, a = f.address - 1;
        auto hasW = [] (const std::vector<int>& v) { return std::find (v.begin(), v.end(), (int) tW) != v.end(); };
        auto put = [&] (int tok, int px, bool w)
        {
            if (a > 511) { ++u; a = 0; }
            pl.U.push_back (u); pl.A.push_back (a); pl.T.push_back (tok); pl.X.push_back (px); pl.W.push_back (w ? 1 : 0); ++a;
        };
        for (int t : H) put (t, -1, hasW (H));
        for (int k = 0; k < pl.pixels; ++k)
        {
            if (f.whole && a + (int) Px.size() > 512 && Px.size() <= 512) { ++u; a = 0; }
            for (int t : Px) put (t, k, hasW (Px));
        }
        for (int t : Tl) put (t, -1, hasW (Tl));

        Info inf; inf.count = (int) pl.U.size(); inf.pixels = pl.pixels;
        if (! pl.U.empty()) { inf.endUniverse = pl.U.back(); inf.endAddress = pl.A.back() + 1; }
        for (size_t c = 0; c < pl.U.size(); ++c)
        {
            universes.insert (pl.U[c]);
            const int key = pl.U[c] * 512 + pl.A[c];
            auto it = owner.find (key);
            if (it == owner.end()) owner[key] = (int) fi;
            else if (it->second != (int) fi) { inf.overlap = true; P->info[(size_t) it->second].overlap = true; }
        }
        P->channels += inf.count;
        P->info.push_back (inf);
        P->plans.push_back (std::move (pl));
    }
    P->universes.assign (universes.begin(), universes.end());
    return P;
}

std::shared_ptr<LightsEngine::Patch> LightsEngine::current() const { const juce::SpinLock::ScopedLockType sl (patchLock); return patch; }
LightsEngine::Config LightsEngine::config() const { return current()->cfg; }
std::vector<LightsEngine::Info> LightsEngine::info() const { return current()->info; }
void LightsEngine::setConfig (const Config& c) { auto p = build (c); const juce::SpinLock::ScopedLockType sl (patchLock); patch = p; }

void LightsEngine::identify (const juce::String& id, double seconds)
{
    const juce::ScopedLock sl (identLock); identId = id; identUntil = nowSec() + seconds;
}

juce::Image LightsEngine::preview() const { const juce::ScopedLock sl (previewLock); return previewImg.createCopy(); }

LightsEngine::Stats LightsEngine::stats() const
{
    const auto p = current(); Stats s;
    s.on = p->cfg.on; s.fixtures = (int) p->cfg.fixtures.size(); s.channels = p->channels; s.universes = (int) p->universes.size();
    s.packetsPerSecond = packetsPerSecond.load();
    { const juce::ScopedLock sl (const_cast<juce::CriticalSection&> (errLock)); s.error = lastError; }
    return s;
}

//==============================================================================
juce::String LightsEngine::toJson() const
{
    const auto c = config();
    auto* root = new juce::DynamicObject();
    root->setProperty ("app", "MBVX-luci"); root->setProperty ("v", 1);
    root->setProperty ("on", c.on); root->setProperty ("fps", c.fps); root->setProperty ("blackout", c.blackout);
    root->setProperty ("proto", c.proto); root->setProperty ("ip", c.ip);
    root->setProperty ("osc", c.osc); root->setProperty ("oscHost", c.oscHost); root->setProperty ("oscPort", c.oscPort);
    juce::Array<juce::var> fx;
    for (auto& f : c.fixtures)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", f.id); o->setProperty ("type", f.type); o->setProperty ("name", f.name);
        o->setProperty ("x", f.x); o->setProperty ("y", f.y); o->setProperty ("w", f.w); o->setProperty ("h", f.h);
        o->setProperty ("cols", f.cols); o->setProperty ("rows", f.rows); o->setProperty ("ch", f.ch);
        o->setProperty ("universe", f.universe); o->setProperty ("address", f.address);
        o->setProperty ("whole", f.whole); o->setProperty ("serp", f.serp); o->setProperty ("vert", f.vert);
        o->setProperty ("flipX", f.flipX); o->setProperty ("flipY", f.flipY); o->setProperty ("dim", f.dim); o->setProperty ("flash", f.flash);
        fx.add (juce::var (o));
    }
    root->setProperty ("fixtures", fx);
    return juce::JSON::toString (juce::var (root), true);
}

bool LightsEngine::loadJson (const juce::String& json, juce::String& error)
{
    juce::var v;
    const auto r = juce::JSON::parse (json, v);
    if (r.failed()) { error = r.getErrorMessage(); return false; }
    const auto* arr = v["fixtures"].getArray();
    if (arr == nullptr) { error = "manca l'elenco \"fixtures\""; return false; }
    Config c = config();
    auto getB = [&v] (const char* k, bool d) { return v.hasProperty (k) ? (bool) v[k] : d; };
    c.on = getB ("on", c.on); c.blackout = getB ("blackout", false); c.osc = getB ("osc", c.osc);
    if (v.hasProperty ("fps")) c.fps = juce::jlimit (5, 60, (int) v["fps"]);
    if (v.hasProperty ("proto")) c.proto = v["proto"].toString();
    if (v.hasProperty ("ip")) c.ip = v["ip"].toString();
    if (v.hasProperty ("oscHost")) c.oscHost = v["oscHost"].toString();
    if (v.hasProperty ("oscPort")) c.oscPort = juce::jlimit (1, 65535, (int) v["oscPort"]);
    c.fixtures.clear();
    for (auto& o : *arr)
    {
        if (! o.isObject()) continue;
        Fixture f = makeFixture (o.getProperty ("type", "wall").toString());
        auto num = [&o] (const char* k, double d) { return o.hasProperty (k) ? (double) o[k] : d; };
        auto bol = [&o] (const char* k, bool d) { return o.hasProperty (k) ? (bool) o[k] : d; };
        if (o.hasProperty ("id")) f.id = o["id"].toString();
        if (o.hasProperty ("name")) f.name = o["name"].toString();
        if (o.hasProperty ("ch")) f.ch = o["ch"].toString();
        if (o.hasProperty ("flash")) f.flash = o["flash"].toString();
        f.x = (float) num ("x", f.x); f.y = (float) num ("y", f.y); f.w = (float) num ("w", f.w); f.h = (float) num ("h", f.h); f.dim = (float) num ("dim", 1);
        f.cols = (int) num ("cols", f.cols); f.rows = (int) num ("rows", f.rows); f.universe = (int) num ("universe", 1); f.address = (int) num ("address", 1);
        f.whole = bol ("whole", true); f.serp = bol ("serp", false); f.vert = bol ("vert", false); f.flipX = bol ("flipX", false); f.flipY = bol ("flipY", false);
        c.fixtures.push_back (cleanFixture (f));
    }
    setConfig (c);
    return true;
}

//==============================================================================
bool LightsEngine::wantsCapture (double now)
{
    const auto p = current();
    if (! p->cfg.on || p->plans.empty()) return false;
    if (now - lastCapture < 1.0 / juce::jlimit (5, 60, p->cfg.fps) - .003) return false;
    lastCapture = now;
    return true;
}

void LightsEngine::processGrid (const juce::uint8* G, const FrameInfo& fr)
{
    const auto p = current();
    const auto& cfg = p->cfg;

    // anteprima per l'editor (5 volte al secondo circa)
    if (++previewCounter % 8 == 0)
    {
        const juce::ScopedLock sl (previewLock);
        juce::Image::BitmapData bd (previewImg, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < gridH; ++y)
        {
            const juce::uint8* row = G + (size_t) (gridH - 1 - y) * gridW * 4;
            for (int x = 0; x < gridW; ++x) bd.setPixelColour (x, y, juce::Colour (row[x * 4], row[x * 4 + 1], row[x * 4 + 2]));
        }
    }

    for (auto it = unis.begin(); it != unis.end();)
        if (! std::binary_search (p->universes.begin(), p->universes.end(), it->first)) it = unis.erase (it); else ++it;
    for (int u : p->universes) unis[u].fill (0);

    if (! cfg.blackout)
    {
        juce::String ident; double identEnd = 0;
        { const juce::ScopedLock sl (identLock); ident = identId; identEnd = identUntil; }
        const double now = nowSec();
        const double sph = std::fmod (std::fmod (fr.vb * fr.strobeRate, 1.0) + 1.0, 1.0);
        const int speed = juce::jlimit (0, 255, juce::roundToInt (fr.bpm / 60.f * fr.strobeRate / 25.f * 255.f));
        auto avg = [G] (int a, int b, int y0, int y1, float* out)
        {
            double r = 0, g = 0, bl = 0; int n = 0;
            for (int y = y0; y < y1; ++y)
            {
                const juce::uint8* o = G + ((size_t) (gridH - 1 - y) * gridW + (size_t) a) * 4;
                for (int x = a; x < b; ++x, o += 4) { r += o[0]; g += o[1]; bl += o[2]; ++n; }
            }
            n = juce::jmax (1, n);
            out[0] = (float) (r / n); out[1] = (float) (g / n); out[2] = (float) (bl / n);
        };
        float zc[3], pc[3];
        for (auto& pl : p->plans)
        {
            const auto& f = pl.f;
            const bool id = ident == f.id && now < identEnd;
            avg (pl.zone[0], pl.zone[1], pl.zone[2], pl.zone[3], zc);
            const float zl = zc[0] * .299f + zc[1] * .587f + zc[2] * .114f;
            const float flash = fr.black ? 0.f : f.flash == "beat" ? (fr.phase < .1f ? 255.f : 0.f) : f.flash == "bass" ? fr.bass * 255.f
                              : f.flash == "level" ? zl : (fr.strobe && sph < .12 ? 255.f : 0.f);
            int lastPix = -2;
            for (size_t i = 0; i < pl.U.size(); ++i)
            {
                const int t = pl.T[i], px = pl.X[i];
                const float* col = zc;
                if (px >= 0) { if (px != lastPix) { const int k = px * 4; avg (pl.cells[(size_t) k], pl.cells[(size_t) k + 1], pl.cells[(size_t) k + 2], pl.cells[(size_t) k + 3], pc); lastPix = px; } col = pc; }
                float r = col[0] * f.dim, g = col[1] * f.dim, b = col[2] * f.dim, w = 0;
                if (id) r = g = b = (((int) (now * 4)) & 1) ? 255.f : 0.f;
                if (pl.W[i]) { w = juce::jmin (r, g, b); r -= w; g -= w; b -= w; }
                float v;
                switch (t)
                {
                    case tR: v = r; break; case tG: v = g; break; case tB: v = b; break; case tW: v = w; break;
                    case tC: v = 255 - r; break; case tM: v = 255 - g; break; case tY: v = 255 - b; break;
                    case tD: v = fr.black ? 0.f : 255.f * f.dim; break; case tL: v = zl; break;
                    case tF: v = id ? 255.f : flash; break; case tS: v = fr.strobe ? (float) speed : 0.f; break;
                    default: v = t < 0 ? (float) (-t - 1) : 0.f;
                }
                auto& buf = unis[pl.U[i]];
                const auto vv = (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (v));
                if (vv > buf[(size_t) pl.A[i]]) buf[(size_t) pl.A[i]] = vv;       // HTP
            }
        }
    }
    send (*p, fr);
}

void LightsEngine::send (const Patch& p, const FrameInfo& fr)
{
    const auto& cfg = p.cfg;
    int sent = 0;
    if (cfg.proto == "artnet" || cfg.proto == "sacn")
    {
        for (auto& [u, data] : unis)
        {
            if (cfg.proto == "artnet")
            {
                juce::uint8 b[18 + 512];
                std::memcpy (b, "Art-Net\0", 8); b[8] = 0x00; b[9] = 0x50; b[10] = 0; b[11] = 14;    // OpCode 0x5000 little endian, versione 14
                b[12] = artSeq; b[13] = 0; b[14] = (juce::uint8) (u & 0xff); b[15] = (juce::uint8) ((u >> 8) & 0x7f); b[16] = 2; b[17] = 0;
                std::memcpy (b + 18, data.data(), 512);
                if (socket.write (cfg.ip.isEmpty() ? "127.0.0.1" : cfg.ip, 6454, b, (int) sizeof (b)) > 0) ++sent;
            }
            else
            {
                juce::uint8 b[638] {};
                auto be16 = [&b] (int o, int v) { b[o] = (juce::uint8) (v >> 8); b[o + 1] = (juce::uint8) v; };
                auto be32 = [&b] (int o, juce::uint32 v) { b[o] = (juce::uint8) (v >> 24); b[o + 1] = (juce::uint8) (v >> 16); b[o + 2] = (juce::uint8) (v >> 8); b[o + 3] = (juce::uint8) v; };
                static const juce::uint8 acn[12] { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0, 0, 0 };
                be16 (0, 0x0010); be16 (2, 0); std::memcpy (b + 4, acn, 12);
                be16 (16, 0x7000 | (638 - 16)); be32 (18, 4); std::memcpy (b + 22, cid, 16);
                be16 (38, 0x7000 | (638 - 38)); be32 (40, 2); std::memcpy (b + 44, "MBStudio91 Visuals", 18); b[108] = 100; be16 (109, 0);
                auto& seq = sacnSeq[u]; b[111] = ++seq; b[112] = 0; be16 (113, u);
                be16 (115, 0x7000 | (638 - 115)); b[117] = 0x02; b[118] = 0xa1; be16 (119, 0); be16 (121, 1); be16 (123, 513); b[125] = 0;
                std::memcpy (b + 126, data.data(), 512);
                const juce::String host = cfg.ip.isNotEmpty() ? cfg.ip : "239.255." + juce::String ((u >> 8) & 0xff) + "." + juce::String (u & 0xff);
                if (socket.write (host, 5568, b, (int) sizeof (b)) > 0) ++sent;
            }
        }
        artSeq = (juce::uint8) (artSeq >= 255 ? 1 : artSeq + 1);
    }
    if (cfg.osc)
    {
        const double now = nowSec();
        const int beat = (int) std::floor (fr.vb);
        auto osc = [this, &cfg, &sent] (const char* a, bool isInt, double v)
        { juce::MemoryOutputStream m; oscAppend (m, a, isInt, v); if (socket.write (cfg.oscHost, cfg.oscPort, m.getData(), (int) m.getDataSize()) > 0) ++sent; };
        if (beat != oscBeat) { oscBeat = beat; osc ("/mbvx/beat", true, beat); osc ("/mbvx/bar", true, ((beat % 4) + 4) % 4 + 1); }
        if ((fr.strobe ? 1 : 0) != oscStrobe) { oscStrobe = fr.strobe ? 1 : 0; osc ("/mbvx/strobe", true, oscStrobe); }
        if (now - oscT > .04)
        {
            oscT = now;
            osc ("/mbvx/bpm", false, fr.bpm); osc ("/mbvx/level", false, fr.level); osc ("/mbvx/bass", false, fr.bass);
            osc ("/mbvx/mid", false, fr.mid); osc ("/mbvx/high", false, fr.high);
        }
    }
    packetCount += sent;
    const double now = nowSec();
    if (now - statsT >= 1.0) { packetsPerSecond = (int) (packetCount.exchange (0) / (now - statsT)); statsT = now; }
}
