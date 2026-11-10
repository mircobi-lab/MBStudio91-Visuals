#include "Renderer.h"
using namespace juce::gl;
using namespace mbv;

static constexpr int TW = 192, TH = 108;

Renderer::Renderer (juce::AudioProcessorValueTreeState& s, VisualEngine& e) : apvts (s), engine (e)
{
    ctx.setRenderer (this);
    ctx.setPreferredVersion (juce::OpenGLVersion (3, 2));
    ctx.setComponentPaintingEnabled (false);
    ctx.setContinuousRepainting (true);
    ctx.setSwapInterval (1);
    juce::OpenGLPixelFormat pf; pf.depthBufferBits = 0; pf.stencilBufferBits = 0; pf.multisamplingLevel = 0;
    ctx.setPixelFormat (pf);
}

Renderer::~Renderer() { detach(); }

void Renderer::attachTo (juce::Component& c)
{
    if (attached == &c) return;
    detach();
    attached = &c;
    ctx.attachTo (c);
}

void Renderer::detach()
{
    if (attached != nullptr) { ctx.detach(); attached = nullptr; }
}

void Renderer::setDeckCode (int d, const juce::String& code)
{
    const juce::ScopedLock sl (lock);
    decks[d].code = code;
}

juce::String Renderer::deckError (int d) const { const juce::ScopedLock sl (lock); return decks[d].error; }
juce::Image Renderer::deckThumbnail (int d) const { const juce::ScopedLock sl (lock); return decks[d].thumb.createCopy(); }
void Renderer::setOutputSize (int w, int h) { outW = juce::jlimit (320, 4096, w); outH = juce::jlimit (180, 2304, h); }

void Renderer::setPaletteColours (juce::Colour c1, juce::Colour c2, juce::Colour c3, juce::Colour mono)
{
    const juce::ScopedLock sl (lock);
    const juce::Colour cs[4] = { c1, c2, c3, mono };
    for (int i = 0; i < 4; ++i) { palC[i][0] = cs[i].getFloatRed(); palC[i][1] = cs[i].getFloatGreen(); palC[i][2] = cs[i].getFloatBlue(); }
}

//==============================================================================
GLuint Renderer::makeTex (int w, int h, GLenum internal, GLenum format, const void* data, bool linear)
{
    GLuint t = 0; glGenTextures (1, &t); glBindTexture (GL_TEXTURE_2D, t);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D (GL_TEXTURE_2D, 0, (GLint) internal, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

Renderer::Fbo Renderer::makeFbo (int w, int h)
{
    Fbo f; f.w = w; f.h = h;
    f.tex = makeTex (w, h, GL_RGBA8, GL_RGBA, nullptr, true);
    glGenFramebuffers (1, &f.fb); glBindFramebuffer (GL_FRAMEBUFFER, f.fb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, f.tex, 0);
    glClearColor (0, 0, 0, 1); glClear (GL_COLOR_BUFFER_BIT);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
    return f;
}

void Renderer::freeFbo (Fbo& f)
{
    if (f.fb) glDeleteFramebuffers (1, &f.fb);
    if (f.tex) glDeleteTextures (1, &f.tex);
    f = {};
}

bool Renderer::buildProgram (Program& out, const juce::String& frag, juce::String& error)
{
    GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
    const char* src = frag.toRawUTF8(); glShaderSource (fs, 1, &src, nullptr); glCompileShader (fs);
    GLint ok = 0; glGetShaderiv (fs, GL_COMPILE_STATUS, &ok);
    if (! ok)
    {
        GLint len = 0; glGetShaderiv (fs, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log ((size_t) juce::jmax (1, len)); glGetShaderInfoLog (fs, len, nullptr, log.data());
        error = juce::String (log.data()); glDeleteShader (fs); return false;
    }
    GLuint p = glCreateProgram(); glAttachShader (p, vs); glAttachShader (p, fs); glLinkProgram (p);
    glGetProgramiv (p, GL_LINK_STATUS, &ok); glDeleteShader (fs);
    if (! ok)
    {
        GLint len = 0; glGetProgramiv (p, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log ((size_t) juce::jmax (1, len)); glGetProgramInfoLog (p, len, nullptr, log.data());
        error = juce::String (log.data()); glDeleteProgram (p); return false;
    }
    if (out.prog) glDeleteProgram (out.prog);
    out.prog = p; out.u.clear();
    GLint n = 0; glGetProgramiv (p, GL_ACTIVE_UNIFORMS, &n);
    for (GLint i = 0; i < n; ++i)
    {
        char name[128]; GLsizei l = 0; GLint size = 0; GLenum type = 0;
        glGetActiveUniform (p, (GLuint) i, 127, &l, &size, &type, name); name[l] = 0;
        out.u[juce::String (name)] = glGetUniformLocation (p, name);
    }
    error.clear();
    return true;
}

//==============================================================================
void Renderer::newOpenGLContextCreated()
{
    glGenVertexArrays (1, &vao); glBindVertexArray (vao);
    vs = glCreateShader (GL_VERTEX_SHADER);
    const juce::String vsrc = ShaderLibrary::vertexSource(); const char* s = vsrc.toRawUTF8();
    glShaderSource (vs, 1, &s, nullptr); glCompileShader (vs);

    std::vector<juce::uint8> z ((size_t) VisualEngine::audioW * 3, 0);
    audioTex = makeTex (VisualEngine::audioW, 2, GL_R8, GL_RED, z.data(), true);
    auxTex   = makeTex (VisualEngine::audioW, 3, GL_R8, GL_RED, z.data(), true);
    std::vector<juce::uint8> zh ((size_t) VisualEngine::histW * VisualEngine::histH, 0);
    histTex  = makeTex (VisualEngine::histW, VisualEngine::histH, GL_R8, GL_RED, zh.data(), true);
    const juce::uint8 black[4] = { 0, 0, 0, 255 };
    blackTex = makeTex (1, 1, GL_RGBA8, GL_RGBA, black, true);
    thumbFbo = makeFbo (TW, TH);
    for (auto& d : decks) { d.compiledCode.clear(); d.ok = false; d.prog = {}; d.tfb = makeFbo (TW, TH); }
    postCache.clear();
    resW = resH = deckW = deckH = pixW = pixH = 0; fbWasOn = false;
    startTime = juce::Time::getMillisecondCounterHiRes() * .001; fpsT = startTime; fpsN = 0;
    glDisable (GL_DEPTH_TEST); glDisable (GL_BLEND); glDisable (GL_CULL_FACE);
    glReady = true;
}

void Renderer::openGLContextClosing()
{
    glReady = false;
    for (auto& d : decks) { if (d.prog.prog) glDeleteProgram (d.prog.prog); d.prog = {}; d.ok = false; d.compiledCode.clear(); freeFbo (d.fb); freeFbo (d.tfb); }
    for (auto& kv : postCache) glDeleteProgram (kv.second.prog);
    postCache.clear();
    freeFbo (mixFb[0]); freeFbo (mixFb[1]); freeFbo (pixFb); freeFbo (thumbFbo);
    GLuint ts[4] = { audioTex, histTex, auxTex, blackTex }; glDeleteTextures (4, ts);
    audioTex = histTex = auxTex = blackTex = 0;
    if (vs) glDeleteShader (vs);
    vs = 0;
    if (vao) glDeleteVertexArrays (1, &vao);
    vao = 0;
}

void Renderer::ensureSizes()
{
    const int w = outW.load(), h = outH.load();
    const float q = engine.qualityScale();
    const int dw = juce::jmax (64, juce::roundToInt ((float) w * q)), dh = juce::jmax (36, juce::roundToInt ((float) h * q));
    const int px = juce::jlimit (1, 8, (int) paramF (ids::pixel));
    const int pw = juce::jmax (16, w / px), ph = juce::jmax (9, h / px);
    if (w != resW || h != resH)
    {
        resW = w; resH = h; engine.setResolution (w, h);
        freeFbo (mixFb[0]); freeFbo (mixFb[1]); mixFb[0] = makeFbo (w, h); mixFb[1] = makeFbo (w, h);
    }
    if (dw != deckW || dh != deckH)
    {
        deckW = dw; deckH = dh;
        for (auto& d : decks) { freeFbo (d.fb); d.fb = makeFbo (dw, dh); }
    }
    if (pw != pixW || ph != pixH)
    {
        pixW = pw; pixH = ph; freeFbo (pixFb);
        if (px > 1) pixFb = makeFbo (pw, ph);
    }
}

void Renderer::compileDeck (int i)
{
    auto& d = decks[i];
    juce::String code;
    { const juce::ScopedLock sl (lock); code = d.code; }
    if (code == d.compiledCode) return;
    d.compiledCode = code;
    juce::String err;
    Program np;
    const bool ok = buildProgram (np, ShaderLibrary::deckHeader() + code + ShaderLibrary::deckFooter(), err);
    if (ok) { if (d.prog.prog) glDeleteProgram (d.prog.prog); d.prog = np; d.ok = true; }
    else
    {
        // in GLSL 1.50 le righe dell'intestazione precedono il codice: riporto i numeri di riga al codice utente
        const int headerLines = juce::StringArray::fromLines (ShaderLibrary::deckHeader()).size() - 1;
        juce::String fixed;
        for (auto line : juce::StringArray::fromLines (err))
        {
            // formati tipici: "0:123(4): error" (Mesa), "ERROR: 0:123:" (Apple), "0(123) :" (NVIDIA)
            auto adj = [&] (const juce::String& l, int pos) { const int ln = l.substring (pos).getIntValue(); return ln > 0 ? juce::String (ln - headerLines) : juce::String(); };
            if (line.startsWith ("ERROR: 0:")) { auto n = adj (line, 9); if (n.isNotEmpty()) line = "riga " + n + ":" + line.fromFirstOccurrenceOf (":", false, false).fromFirstOccurrenceOf (":", false, false); }
            else if (line.startsWith ("0:")) { auto n = adj (line, 2); if (n.isNotEmpty()) line = "riga " + n + line.fromFirstOccurrenceOf (":", false, false).fromFirstOccurrenceOf (":", false, false); }
            else if (line.startsWith ("0(")) { auto n = adj (line, 2); if (n.isNotEmpty()) line = "riga " + n + line.fromFirstOccurrenceOf (")", false, false); }
            if (line.trim().isNotEmpty()) fixed << line << '\n';
        }
        err = fixed;
    }
    const juce::ScopedLock sl (lock);
    d.error = ok ? juce::String() : err;
}

void Renderer::drawDeck (int i, const VisualEngine::Frame& fr, const Fbo& T)
{
    auto& d = decks[i];
    glBindFramebuffer (GL_FRAMEBUFFER, T.fb); glViewport (0, 0, T.w, T.h);
    if (! d.ok) { glClearColor (0, 0, 0, 1); glClear (GL_COLOR_BUFFER_BIT); return; }
    auto& P = d.prog;
    glUseProgram (P.prog);
    const float R = paramF (ids::deck (i, ids::reactDeck));
    auto u1f = [&] (const char* n, float v) { const GLint l = P.loc (n); if (l >= 0) glUniform1f (l, v); };
    auto u1i = [&] (const char* n, int v)   { const GLint l = P.loc (n); if (l >= 0) glUniform1i (l, v); };
    { const GLint l = P.loc ("iResolution"); if (l >= 0) glUniform3f (l, (float) T.w, (float) T.h, 1.f); }
    u1f ("iTime", (float) fr.deckTime[i]); u1f ("iTimeDelta", (float) fr.dt); u1i ("iFrame", (int) frameN);
    u1f ("iBass", juce::jmin (1.f, fr.bass * R)); u1f ("iMid", juce::jmin (1.f, fr.mid * R)); u1f ("iHigh", juce::jmin (1.f, fr.high * R)); u1f ("iLevel", juce::jmin (1.f, fr.level * R));
    u1f ("iBeat", fr.pulse * juce::jmin (1.f, R)); u1f ("iPhase", fr.phase); u1f ("iBar", fr.bar); u1f ("iBPM", fr.bpm);
    u1f ("iParam1", paramF (ids::deck (i, ids::p1))); u1f ("iParam2", paramF (ids::deck (i, ids::p2)));
    const float rot = fr.deckRot[i];
    u1f ("iRot", rot); { const GLint l = P.loc ("iRotCS"); if (l >= 0) glUniform2f (l, std::cos (rot), std::sin (rot)); }
    { const GLint l = P.loc ("iMouse"); if (l >= 0) glUniform4f (l, 0, 0, 0, 0); }
    u1f ("iChannel0Ready", 0.f); { const GLint l = P.loc ("iChannelRes"); if (l >= 0) glUniform2f (l, 1, 1); }
    glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, blackTex); u1i ("iChannel0", 0);
    glActiveTexture (GL_TEXTURE1); glBindTexture (GL_TEXTURE_2D, audioTex); u1i ("iAudio", 1);
    glActiveTexture (GL_TEXTURE2); glBindTexture (GL_TEXTURE_2D, histTex);  u1i ("iSpecHist", 2);
    glActiveTexture (GL_TEXTURE3); glBindTexture (GL_TEXTURE_2D, auxTex);   u1i ("iAux", 3);
    u1f ("iHistHead", (float) fr.histHead); u1f ("iHistFrac", fr.histFrac);
    glDrawArrays (GL_TRIANGLES, 0, 3);
}

Renderer::Program* Renderer::postProgram (const ShaderLibrary::PostConfig& c)
{
    const auto key = c.key();
    auto it = postCache.find (key);
    if (it != postCache.end()) return it->second.prog ? &it->second : nullptr;
    Program p; juce::String err;
    if (! buildProgram (p, ShaderLibrary::postSource (c), err)) { DBG ("post shader: " << err); postCache[key] = {}; return nullptr; }
    if (postCache.size() > 96) { for (auto& kv : postCache) if (kv.second.prog) glDeleteProgram (kv.second.prog); postCache.clear(); }
    return &(postCache[key] = p);
}

void Renderer::setPostUniforms (Program& p, const VisualEngine::Frame& fr, int w, int h)
{
    glUseProgram (p.prog);
    auto u1f = [&] (const char* n, float v) { const GLint l = p.loc (n); if (l >= 0) glUniform1f (l, v); };
    auto u1i = [&] (const char* n, int v)   { const GLint l = p.loc (n); if (l >= 0) glUniform1i (l, v); };
    { const GLint l = p.loc ("uRes"); if (l >= 0) glUniform2f (l, (float) w, (float) h); }
    u1f ("uXf", fr.xf); u1f ("uFeedback", paramF (ids::feedback)); u1f ("uHue", paramF (ids::hue) * juce::MathConstants<float>::pi / 180.f);
    u1f ("uPZ", fr.pulse * paramF (ids::zoom)); u1f ("uKaleido", paramF (ids::kaleido));
    static const float rates[] = { 1.f, 2.f, 4.f, .25f };
    const float rate = rates[juce::jlimit (0, 3, (int) paramF (ids::strobeRate))];
    const double sp = std::fmod (std::fmod (fr.vb * rate, 1.0) + 1.0, 1.0);
    u1f ("uStrobePhase", (float) sp);
    u1f ("uMaster", paramF (ids::black) > .5f ? 0.f : paramF (ids::master));
    {
        const juce::ScopedLock sl (lock);
        const char* names[4] = { "uPalC1", "uPalC2", "uPalC3", "uMono" };
        for (int i = 0; i < 4; ++i) { const GLint l = p.loc (names[i]); if (l >= 0) glUniform3f (l, palC[i][0], palC[i][1], palC[i][2]); }
    }
    u1i ("uA", 0); u1i ("uB", 1); u1i ("uPrev", 2); u1i ("uT", 0);
}

void Renderer::readThumb (int i)
{
    auto& d = decks[i];
    if (d.visible)
    {
        glBindFramebuffer (GL_READ_FRAMEBUFFER, d.fb.fb); glBindFramebuffer (GL_DRAW_FRAMEBUFFER, thumbFbo.fb);
        glBlitFramebuffer (0, 0, d.fb.w, d.fb.h, 0, 0, TW, TH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer (GL_READ_FRAMEBUFFER, thumbFbo.fb);
    }
    else glBindFramebuffer (GL_READ_FRAMEBUFFER, d.tfb.fb);
    std::vector<juce::uint8> px ((size_t) TW * TH * 4);
    glPixelStorei (GL_PACK_ALIGNMENT, 1);
    glReadPixels (0, 0, TW, TH, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);
    juce::Image img (juce::Image::ARGB, TW, TH, false);
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < TH; ++y)
        {
            const juce::uint8* srcRow = px.data() + (size_t) (TH - 1 - y) * TW * 4;
            for (int x = 0; x < TW; ++x) bd.setPixelColour (x, y, juce::Colour (srcRow[x * 4], srcRow[x * 4 + 1], srcRow[x * 4 + 2]));
        }
    }
    const juce::ScopedLock sl (lock);
    d.thumb = img;
}

void Renderer::renderOpenGL()
{
    if (! glReady) return;
    glBindVertexArray (vao);
    const double now = juce::Time::getMillisecondCounterHiRes() * .001;
    const auto& fr = engine.update (now);
    ensureSizes();

    // texture audio
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glBindTexture (GL_TEXTURE_2D, audioTex); glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, VisualEngine::audioW, 2, GL_RED, GL_UNSIGNED_BYTE, engine.audioTex());
    glBindTexture (GL_TEXTURE_2D, auxTex);   glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, VisualEngine::audioW, 3, GL_RED, GL_UNSIGNED_BYTE, engine.auxTex());
    if (fr.histPushes > 0)
    {
        glBindTexture (GL_TEXTURE_2D, histTex);
        int head = fr.histHead - fr.histPushes + 1;
        for (int k = 0; k < fr.histPushes; ++k, ++head)
            glTexSubImage2D (GL_TEXTURE_2D, 0, 0, ((head % VisualEngine::histH) + VisualEngine::histH) % VisualEngine::histH, VisualEngine::histW, 1, GL_RED, GL_UNSIGNED_BYTE, engine.histRow (k));
    }

    // deck
    const int mode = (int) paramF (ids::mode);
    const bool showA = mode == 6 ? fr.xf < .5f : fr.xf < .999f;
    const bool showB = mode == 6 ? fr.xf >= .5f : fr.xf > .001f;
    const int slot = (int) (frameN % 8);
    for (int i = 0; i < 2; ++i)
    {
        compileDeck (i);
        auto& d = decks[i];
        d.visible = i ? showB : showA;
        const bool thumbNow = slot == (i ? 4 : 0);
        if (d.visible) drawDeck (i, fr, d.fb);
        else if (thumbNow) drawDeck (i, fr, d.tfb);
        if (thumbNow) readThumb (i);
    }

    // mix + uscita
    const int px = juce::jlimit (1, 8, (int) paramF (ids::pixel));
    const bool fbOn = paramF (ids::feedback) > .001f;
    ShaderLibrary::PostConfig c;
    c.src = showA && showB ? 0 : showA ? 1 : 2;
    c.mode = c.src ? 0 : mode; c.fb = fbOn; c.hue = paramF (ids::hue) > .05f; c.kal = paramF (ids::kaleido) > 1.5f;
    c.mirror = paramF (ids::mirror) > .5f; c.zoom = paramF (ids::zoom) > 0.f; c.invert = paramF (ids::invert) > .5f;
    c.pal = (int) paramF (ids::palette); c.strobe = paramF (ids::strobe) > .5f;
    if (fbOn && ! fbWasOn)
        for (auto& f : mixFb) { glBindFramebuffer (GL_FRAMEBUFFER, f.fb); glClearColor (0, 0, 0, 1); glClear (GL_COLOR_BUFFER_BIT); }
    fbWasOn = fbOn;

    // destinazione dello stadio di uscita: buffer a pixel grossi oppure direttamente lo schermo
    const GLuint screenFb = ctx.getFrameBufferID();
    const double scale = ctx.getRenderingScale();
    const int cw = attached ? juce::roundToInt (attached->getWidth() * scale) : resW, chh = attached ? juce::roundToInt (attached->getHeight() * scale) : resH;
    GLuint outFb = screenFb; int ow = cw, oh = chh;
    if (px > 1) { outFb = pixFb.fb; ow = pixW; oh = pixH; }

    glActiveTexture (GL_TEXTURE1); glBindTexture (GL_TEXTURE_2D, decks[1].fb.tex);
    bool drawn = false;
    if (fbOn)
    {
        auto c0 = c; c0.kind = 0; auto c1 = c; c1.kind = 1;
        Program* p0 = postProgram (c0); Program* p1 = postProgram (c1);
        if (p0 && p1)
        {
            Fbo& dst = mixFb[cur]; Fbo& prev = mixFb[1 - cur];
            glActiveTexture (GL_TEXTURE2); glBindTexture (GL_TEXTURE_2D, prev.tex);
            glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, decks[0].fb.tex);
            glBindFramebuffer (GL_FRAMEBUFFER, dst.fb); glViewport (0, 0, resW, resH);
            setPostUniforms (*p0, fr, resW, resH); glDrawArrays (GL_TRIANGLES, 0, 3);
            glBindFramebuffer (GL_FRAMEBUFFER, outFb); glViewport (0, 0, ow, oh);
            glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, dst.tex);
            setPostUniforms (*p1, fr, ow, oh); glDrawArrays (GL_TRIANGLES, 0, 3);
            cur = 1 - cur; drawn = true;
        }
    }
    else if (Program* p2 = postProgram (c))
    {
        glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, decks[0].fb.tex);
        glBindFramebuffer (GL_FRAMEBUFFER, outFb); glViewport (0, 0, ow, oh);
        setPostUniforms (*p2, fr, ow, oh); glDrawArrays (GL_TRIANGLES, 0, 3);
        drawn = true;
    }
    if (! drawn) { glBindFramebuffer (GL_FRAMEBUFFER, screenFb); glViewport (0, 0, cw, chh); glClearColor (0, 0, 0, 1); glClear (GL_COLOR_BUFFER_BIT); }
    else if (px > 1)
    {
        // ingrandimento a pixel netti (nessun filtro): il "pixel" resta un quadrato pieno
        glBindFramebuffer (GL_READ_FRAMEBUFFER, pixFb.fb); glBindFramebuffer (GL_DRAW_FRAMEBUFFER, screenFb);
        glBlitFramebuffer (0, 0, pixW, pixH, 0, 0, cw, chh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer (GL_FRAMEBUFFER, screenFb);

    ++frameN; ++fpsN;
    if (now - fpsT > .5)
    {
        const float fps = (float) (fpsN / (now - fpsT)); fpsN = 0; fpsT = now;
        fpsValue = fps; engine.reportFps (fps);
    }
}
