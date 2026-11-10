#pragma once
#include <juce_opengl/juce_opengl.h>
#include "VisualEngine.h"
#include "ShaderLibrary.h"

/*  Rendering OpenGL 3.2 core (GLSL 1.50): due deck su FBO a risoluzione ridotta, mix + palette su un
    buffer "a pixel grossi" (dimensione Pixel), ingrandimento NEAREST a schermo. Tutto gira sul thread GL
    di JUCE; il resto del plugin parla con il renderer tramite atomici e una sezione critica per le stringhe. */
class Renderer : public juce::OpenGLRenderer
{
public:
    Renderer (juce::AudioProcessorValueTreeState& apvts, VisualEngine& engine);
    ~Renderer() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void attachTo (juce::Component& c);     // crea e aggancia il contesto GL al componente
    void detach();
    juce::OpenGLContext& context()          { return ctx; }

    // Thread dei messaggi
    void setDeckCode (int deck, const juce::String& mainImageCode);
    juce::String deckError (int deck) const;
    juce::Image deckThumbnail (int deck) const;
    void setOutputSize (int w, int h);
    void setPaletteColours (juce::Colour c1, juce::Colour c2, juce::Colour c3, juce::Colour mono);
    float fps() const                       { return fpsValue.load(); }

private:
    struct Fbo { GLuint fb = 0, tex = 0; int w = 0, h = 0; };
    struct Program { GLuint prog = 0; std::map<juce::String, GLint> u; GLint loc (const char* n) { auto it = u.find (n); return it == u.end() ? -1 : it->second; } };
    struct Deck
    {
        Program prog; bool ok = false;
        juce::String code, compiledCode, error;
        Fbo fb, tfb; bool visible = true;
        juce::Image thumb { juce::Image::ARGB, 192, 108, true };
    };

    Fbo makeFbo (int w, int h);
    void freeFbo (Fbo&);
    GLuint makeTex (int w, int h, GLenum internal, GLenum format, const void* data, bool linear);
    bool buildProgram (Program& out, const juce::String& frag, juce::String& error);
    void compileDeck (int d);
    void drawDeck (int d, const VisualEngine::Frame& fr, const Fbo& target);
    Program* postProgram (const ShaderLibrary::PostConfig&);
    void setPostUniforms (Program& p, const VisualEngine::Frame& fr, int w, int h);
    void readThumb (int d);
    void ensureSizes();
    float paramF (const juce::String& id) const { if (auto* p = apvts.getRawParameterValue (id)) return p->load(); return 0.f; }

    juce::AudioProcessorValueTreeState& apvts;
    VisualEngine& engine;
    juce::OpenGLContext ctx;
    juce::Component* attached = nullptr;

    mutable juce::CriticalSection lock;    // protegge code/error/thumb/colori
    Deck decks[2];
    std::atomic<int> outW { 1280 }, outH { 720 };
    float palC[4][3] { { 0, .83f, 1 }, { 1, .71f, .28f }, { 1, .33f, .44f }, { 0, 1, .4f } };

    GLuint vao = 0, vs = 0, audioTex = 0, histTex = 0, auxTex = 0, blackTex = 0;
    Fbo mixFb[2], pixFb, thumbFbo; int cur = 0;
    int resW = 0, resH = 0, deckW = 0, deckH = 0, pixW = 0, pixH = 0;
    bool fbWasOn = false;
    std::map<juce::String, Program> postCache;
    juce::uint32 frameN = 0;
    double startTime = 0; int fpsN = 0; double fpsT = 0;
    std::atomic<float> fpsValue { 0 };
    bool glReady = false;
};
