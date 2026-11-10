#include "ShaderLibrary.h"
#include "BinaryData.h"
#include <algorithm>
#include <vector>

static const char* const kHeader = R"GLSL(#version 150
precision highp float;
precision highp int;
uniform vec3 iResolution;
uniform float iTime, iTimeDelta, iBass, iMid, iHigh, iLevel, iBeat, iPhase, iBar, iBPM, iParam1, iParam2, iChannel0Ready, iHistHead, iHistFrac;
uniform int iFrame;
uniform vec4 iMouse;
uniform float iRot;   // rotazione del deck in radianti (Rotazione + Spin)
uniform vec2 iRotCS;  // coseno e seno della rotazione, calcolati una volta per fotogramma
uniform vec2 iChannelRes;
uniform sampler2D iChannel0;
uniform sampler2D iAudio;
uniform sampler2D iSpecHist;
uniform sampler2D iAux;
out vec4 _fc;

float audioSpec(float x){return texture(iAudio,vec2(clamp(x,0.,1.),.25)).r;}
float audioWave(float x){return texture(iAudio,vec2(clamp(x,0.,1.),.75)).r;}
float glow(float w,float d){return 1.-smoothstep(0.,w,d);}
// Dati pronti dalla CPU (una lettura invece di un ciclo nello shader):
// minimo e massimo della forma d'onda nei dintorni di x (±16 campioni) e picco dello spettro con discesa lenta
float waveMin(float x){return texture(iAux,vec2(clamp(x,0.,1.),.1666667)).r;}
float waveMax(float x){return texture(iAux,vec2(clamp(x,0.,1.),.5)).r;}
float audioPeak(float x){return texture(iAux,vec2(clamp(x,0.,1.),.8333333)).r;}
// Storia dello spettro: x 0..1 (frequenza log), age 0 = riga piu' recente, max 63 (~2.1 s)
float specHist(float x,float age){float r=mod(iHistHead-floor(clamp(age,0.,63.))+64.,64.);return texture(iSpecHist,vec2(clamp(x,0.,1.),(r+.5)/64.)).r;}

// Palette arcobaleno — una sola funzione, sempre vec3.
vec3 rainbow3(vec3 t){return 0.5+0.5*cos(6.2831853*(t+vec3(0.0,0.33,0.67)));}
// Macro compatibile: accetta float (broadcast) o vec3 (identità).
#define rainbow(x) rainbow3(vec3(x))

vec2 coverUV(vec2 uv){float ra=iResolution.x/iResolution.y, ta=iChannelRes.x/max(iChannelRes.y,1.);vec2 s=ra>ta?vec2(1.,ta/ra):vec2(ra/ta,1.);return (uv-.5)*s+.5;}

vec2 rot2(vec2 p,float a){float c=cos(a),s=sin(a);return vec2(p.x*c-p.y*s,p.x*s+p.y*c);}
vec2 zoomRot(vec2 p,float s,float a){return rot2(p*s,a);}
vec3 hsv2rgb(vec3 c){vec3 r=clamp(abs(fract(c.xxx+vec3(0.,2./3.,1./3.))*6.-3.)-1.,0.,1.);return c.z*mix(vec3(1.),r,c.y);}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
  return mix(mix(hash(i),hash(i+vec2(1.,0.)),f.x),mix(hash(i+vec2(0.,1.)),hash(i+vec2(1.,1.)),f.x),f.y);}
)GLSL";

static const char* const kFooter = R"GLSL(
void main(){vec4 c=vec4(0.,0.,0.,1.);vec2 h=.5*iResolution.xy,d=gl_FragCoord.xy-h;vec2 fc=vec2(d.x*iRotCS.x-d.y*iRotCS.y,d.x*iRotCS.y+d.y*iRotCS.x)+h;mainImage(c,fc);_fc=vec4(clamp(c.rgb,0.,1.),1.);}
)GLSL";

static const char* const kPostBody = R"GLSL(
precision highp float;
uniform sampler2D uA,uB,uPrev,uT;
uniform vec2 uRes;
uniform float uXf,uFeedback,uHue,uPZ,uKaleido,uStrobePhase,uMaster;
uniform vec3 uPalC1,uPalC2,uPalC3,uMono;
out vec4 o;
float luma(vec3 c){return dot(c,vec3(.299,.587,.114));}
#if KIND!=1
#if HUE
vec3 hueRot(vec3 c,float a){
  const mat3 toY=mat3(.299,.596,.211, .587,-.274,-.523, .114,-.322,.312);
  const mat3 toR=mat3(1.,1.,1., .956,-.272,-1.106, .621,-.647,1.703);
  vec3 y=toY*c; float h=atan(y.z,y.y)+a, ch=length(y.yz);
  y.y=ch*cos(h); y.z=ch*sin(h); return clamp(toR*y,0.,1.);
}
#endif
vec3 mixStage(){
  vec2 uv=gl_FragCoord.xy/uRes;
  vec2 c=uv-.5;
#if ZOOM
  c*=1.-uPZ*.15;
#endif
#if KAL
  { float ar=uRes.x/uRes.y; vec2 p=vec2(c.x*ar,c.y);
    float r=length(p), a=atan(p.y,p.x), s=6.2831853/uKaleido;
    a=abs(mod(a,s)-s*.5); p=vec2(cos(a),sin(a))*r; c=vec2(p.x/ar,p.y); }
#endif
#if MIRROR
  c.x=-abs(c.x);
#endif
  vec2 s=c+.5;
#if SRC==1
  vec3 col=texture(uA,s).rgb;
#elif SRC==2
  vec3 col=texture(uB,s).rgb;
#else
  vec3 A=texture(uA,s).rgb, B=texture(uB,s).rgb, col;
#if MODE==6
  col=uXf<.5?A:B;
#elif MODE==0
  col=mix(A,B,uXf);
#else
#if MODE==1
  vec3 X=min(A+B,1.);
#elif MODE==2
  vec3 X=1.-(1.-A)*(1.-B);
#elif MODE==3
  vec3 X=A*B;
#elif MODE==4
  vec3 X=abs(A-B);
#else
  vec3 X=mix(A,B,smoothstep(.15,.5,luma(B)));
#endif
  col=uXf<.5?mix(A,X,uXf*2.):mix(X,B,uXf*2.-1.);
#endif
#endif
#if FB
  vec2 fuv=(uv-.5)*(1.-.012*uFeedback)+.5;
  col=max(col,texture(uPrev,fuv).rgb*uFeedback*.97);
#endif
#if HUE
  col=hueRot(col,uHue);
#endif
  return col;
}
#endif
#if KIND!=0
#if PAL>=4 && PAL<=6
const float B4[16]=float[16](0.,8.,2.,10.,12.,4.,14.,6.,3.,11.,1.,9.,15.,7.,13.,5.);
float bayer4(vec2 p){ivec2 ip=ivec2(mod(p,4.));return (B4[ip.y*4+ip.x]+.5)/16.;}
#endif
vec3 outStage(vec3 c){
#if INVERT
  c=1.-c;
#endif
#if PAL>0
  float l=clamp(luma(c),0.,1.);
#endif
#if PAL==1
  c=vec3(l*.10,l,l*.35);
#elif PAL==2
  c=vec3(l*.30,l*.85,l);
#elif PAL==3
  c=vec3(l);
#elif PAL==4
  c=vec3(step(.5,l+(bayer4(gl_FragCoord.xy)-.5)*.4));
#elif PAL==5
  c=vec3(floor(clamp((l+(bayer4(gl_FragCoord.xy)-.5)/4.)*4.,0.,3.))/3.);
#elif PAL==6
  c=vec3(floor(clamp((l+(bayer4(gl_FragCoord.xy)-.5)/16.)*16.,0.,15.))/15.);
#elif PAL==7
  c=vec3(l,l*.55,l*.10);
#elif PAL==8
  c=l<.5?mix(uPalC1,uPalC2,l*2.):mix(uPalC2,uPalC3,(l-.5)*2.);
#elif PAL==9
  c=uMono*l;
#endif
#if STROBE
  c=uStrobePhase<.12?vec3(1.):c*.15;
#endif
  return c*uMaster;
}
#endif
void main(){
#if KIND==0
  o=vec4(mixStage(),1.);
#elif KIND==1
  o=vec4(outStage(texture(uT,gl_FragCoord.xy/uRes).rgb),1.);
#else
  o=vec4(outStage(mixStage()),1.);
#endif
}
)GLSL";

ShaderLibrary::ShaderLibrary() { rescan(); }

juce::File ShaderLibrary::userFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("MBStudio91 Visuals").getChildFile ("Shaders");
}

void ShaderLibrary::rescan()
{
    presets.clear();
    struct Entry { juce::String fname; Preset p; };
    std::vector<Entry> builtin;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        int size = 0;
        const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        juce::String fname = BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]);
        if (data == nullptr || ! fname.endsWithIgnoreCase (".glsl")) continue;
        Entry e; e.fname = fname;
        e.p.name = fname.upToLastOccurrenceOf (".", false, false);
        if (e.p.name.length() > 3 && e.p.name.substring (0, 2).containsOnly ("0123456789") && e.p.name[2] == ' ')
            e.p.name = e.p.name.substring (3);                      // toglie il prefisso "NN " usato per l'ordine
        e.p.code = juce::String::fromUTF8 (data, size);
        builtin.push_back (std::move (e));
    }
    std::sort (builtin.begin(), builtin.end(), [] (const Entry& a, const Entry& b) { return a.fname.compareNatural (b.fname) < 0; });
    for (auto& e : builtin) presets.add (e.p);
    auto folder = userFolder();
    if (folder.isDirectory())
    {
        juce::Array<juce::File> files = folder.findChildFiles (juce::File::findFiles, false, "*.glsl");
        files.sort();
        for (auto& f : files)
        {
            Preset p; p.user = true; p.file = f;
            p.name = f.getFileNameWithoutExtension();
            p.code = f.loadFileAsString();
            presets.add (p);
        }
    }
    if (presets.isEmpty())
    {
        Preset p; p.name = "Vuoto";
        p.code = "void mainImage(out vec4 o,in vec2 f){o=vec4(0.,0.,0.,1.);}";
        presets.add (p);
    }
}

juce::StringArray ShaderLibrary::names() const
{
    juce::StringArray n;
    for (auto& p : presets) n.add (p.user ? p.name + " (utente)" : p.name);
    return n;
}

int ShaderLibrary::indexOf (const juce::String& name) const
{
    for (int i = 0; i < presets.size(); ++i) if (presets.getReference (i).name == name) return i;
    return -1;
}

juce::String ShaderLibrary::deckHeader() { return kHeader; }
juce::String ShaderLibrary::deckFooter() { return kFooter; }

juce::String ShaderLibrary::vertexSource()
{
    // un solo triangolo che copre lo schermo: niente VBO, solo gl_VertexID
    return "#version 150\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2)*2.-1.;gl_Position=vec4(p,0.,1.);}\n";
}

juce::String ShaderLibrary::PostConfig::key() const
{
    juce::String k; k << kind << '|';
    if (kind != 1) k << src << ',' << mode << ',' << fb << ',' << hue << ',' << kal << ',' << mirror << ',' << zoom;
    k << '|';
    if (kind != 0) k << invert << ',' << pal << ',' << strobe;
    return k;
}

juce::String ShaderLibrary::postSource (const PostConfig& c)
{
    juce::String s = "#version 150\n";
    auto D = [&] (const char* n, int v) { s << "#define " << n << ' ' << v << '\n'; };
    D ("KIND", c.kind); D ("SRC", c.src); D ("MODE", c.mode); D ("FB", c.fb); D ("HUE", c.hue); D ("KAL", c.kal);
    D ("MIRROR", c.mirror); D ("ZOOM", c.zoom); D ("INVERT", c.invert); D ("PAL", c.pal); D ("STROBE", c.strobe);
    s << kPostBody;
    return s;
}
