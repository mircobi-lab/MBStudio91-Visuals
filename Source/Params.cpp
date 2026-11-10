#include "Params.h"

namespace mbv
{
const juce::StringArray& modeNames()
{
    static const juce::StringArray n { "Dissolvenza", "Somma", "Schermo", "Moltiplica", "Differenza", "Luma key", "Nessuna (taglio A/B)" };
    return n;
}
const juce::StringArray& paletteNames()
{
    static const juce::StringArray n { "Originale", "Verde", "Blu chiaro", "Monocromatico", "Mono 1-bit", "Mono 2-bit", "Mono 4-bit", "Ambra", "Pantone (3 colori)", "Monocolore" };
    return n;
}
const juce::StringArray& strobeNames()
{
    static const juce::StringArray n { "1 per beat", "2 per beat", "4 per beat", "1 per battuta" };
    return n;
}
const juce::StringArray& qualityNames()
{
    static const juce::StringArray n { "Auto", "100%", "75%", "50%", "35%" };
    return n;
}
const juce::StringArray& clockNames()
{
    static const juce::StringArray n { "Host (DAW)", "Audio (rilevato)", "Interno / Tap" };
    return n;
}

using P = juce::AudioProcessorValueTreeState;

static std::unique_ptr<juce::RangedAudioParameter> fl (const juce::String& id, const juce::String& name, float lo, float hi, float def, float step = 0.0f, const juce::String& unit = {})
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, step), def,
                                                        juce::AudioParameterFloatAttributes().withLabel (unit));
}
static std::unique_ptr<juce::RangedAudioParameter> bo (const juce::String& id, const juce::String& name, bool def)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}
static std::unique_ptr<juce::RangedAudioParameter> ch (const juce::String& id, const juce::String& name, const juce::StringArray& names, int def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, names, def);
}

P::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (fl (ids::xf, "Crossfader A/B", 0.f, 1.f, 0.f));
    p.push_back (fl (ids::xfTime, "Transizione A/B", 0.f, 3.f, 1.f, 0.1f, "s"));
    p.push_back (ch (ids::mode, "Fusione", modeNames(), 0));
    p.push_back (fl (ids::feedback, "Scia", 0.f, 1.f, 0.f));
    p.push_back (fl (ids::hue, "Tinta", 0.f, 360.f, 0.f, 1.f, juce::String::fromUTF8 ("°")));
    p.push_back (fl (ids::zoom, "Zoom sul beat", 0.f, 1.f, 0.f));
    p.push_back (fl (ids::kaleido, "Caleido", 0.f, 12.f, 0.f, 1.f));
    p.push_back (bo (ids::strobe, "Strobo", false));
    p.push_back (ch (ids::strobeRate, juce::String::fromUTF8 ("Strobo velocità"), strobeNames(), 0));
    p.push_back (bo (ids::invert, "Inverti", false));
    p.push_back (bo (ids::mirror, "Specchio", false));
    p.push_back (bo (ids::black, "Buio", false));
    p.push_back (fl (ids::master, "Master", 0.f, 1.f, 1.f));
    p.push_back (ch (ids::palette, "Palette", paletteNames(), 0));
    p.push_back (fl (ids::pixel, "Pixel", 1.f, 8.f, 1.f, 1.f));
    p.push_back (ch (ids::quality, juce::String::fromUTF8 ("Qualità"), qualityNames(), 0));

    p.push_back (ch (ids::clockSrc, "Clock", clockNames(), 0));
    p.push_back (fl (ids::bpm, "BPM", 40.f, 240.f, 120.f, 0.01f));
    p.push_back (fl (ids::beatOffset, "Anticipo visual", -200.f, 200.f, 0.f, 1.f, "ms"));
    p.push_back (fl (ids::react, juce::String::fromUTF8 ("Reattività globale"), 0.f, 2.f, 1.f));

    p.push_back (bo (ids::tap, "Tap tempo", false));
    p.push_back (bo (ids::downbeat, "Primo battito", false));
    p.push_back (bo (ids::goA, "Vai ad A", false));
    p.push_back (bo (ids::goB, "Vai a B", false));

    for (int d = 0; d < 2; ++d)
    {
        const juce::String L = d == 0 ? "A " : "B ";
        p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ids::deck (d, ids::preset), 1 }, L + "Preset", 0, maxPresets - 1, d == 0 ? 0 : 1));
        p.push_back (fl (ids::deck (d, ids::p1), L + "Param 1", 0.f, 1.f, .5f));
        p.push_back (fl (ids::deck (d, ids::p2), L + "Param 2", 0.f, 1.f, .5f));
        p.push_back (fl (ids::deck (d, ids::speed), L + juce::String::fromUTF8 ("Velocità"), 0.f, 4.f, 1.f));
        p.push_back (fl (ids::deck (d, ids::reactDeck), L + juce::String::fromUTF8 ("Reattività"), 0.f, 2.f, 1.f));
        p.push_back (fl (ids::deck (d, ids::rot), L + "Rotazione", -180.f, 180.f, 0.f, 1.f, juce::String::fromUTF8 ("°")));
        p.push_back (fl (ids::deck (d, ids::spin), L + "Spin", -1.f, 1.f, 0.f));
        p.push_back (bo (ids::deck (d, ids::spinTempo), L + "Spin a tempo", false));
    }
    return { p.begin(), p.end() };
}
}
