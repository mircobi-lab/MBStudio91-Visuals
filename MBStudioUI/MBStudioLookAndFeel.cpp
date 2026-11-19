#include "MBStudioLookAndFeel.h"
#include "MBStudioUIFonts.h"
#if JUCE_MODULE_AVAILABLE_juce_gui_extra
 #include <juce_gui_extra/juce_gui_extra.h>
#endif

namespace mbstudio::ui
{
namespace
{
    // Typeface cache released by JUCE at shutdown: a plain static would
    // outlive the font backend and crash when the plug-in is unloaded.
    struct FontCache : juce::DeletedAtShutdown
    {
        std::map<juce::String, juce::Typeface::Ptr> map;
        ~FontCache() override { clearSingletonInstance(); }
        JUCE_DECLARE_SINGLETON_INLINE (FontCache, false)
    };

    juce::Typeface::Ptr load (const char* resource)
    {
        int size = 0;
        const char* data = MBStudioUIFonts::getNamedResource (resource, size);
        return data != nullptr ? juce::Typeface::createSystemTypefaceFor (data, (size_t) size) : nullptr;
    }
    juce::Font make (const char* resource, float h, float kerning)
    {
        auto& tf = FontCache::getInstance()->map[resource];
        if (tf == nullptr) tf = load (resource);
        juce::Font f = tf != nullptr ? juce::Font (juce::FontOptions (tf).withHeight (h)) : juce::Font (juce::FontOptions (h));
        return f.withExtraKerningFactor (kerning);
    }
    juce::Colour ledColourOf (const juce::Component& c, juce::Colour fallback)
    {
        const auto v = c.getProperties()["ledColour"];
        return v.isString() && v.toString().isNotEmpty() ? juce::Colour::fromString (v.toString()) : fallback;
    }
}

juce::Font Fonts::display (float h) { return make ("BebasNeueRegular_ttf", h, .04f); }
juce::Font Fonts::label (float h)   { return make ("AntonioLight_ttf", h, .05f); }
juce::Font Fonts::thin (float h)    { return make ("AntonioThin_ttf", h, .02f); }
juce::Font Fonts::bold (float h)    { return make ("AntonioBold_ttf", h, .02f); }

//==============================================================================
void paintChassis (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto r = area.toFloat();
    g.setGradientFill (juce::ColourGradient (colours::chassisTop, r.getX(), r.getY(), colours::chassisBottom, r.getX(), r.getBottom(), false));
    g.fillRect (r);
    // grana opaca molto leggera: righe orizzontali alternate quasi invisibili
    g.setColour (juce::Colours::white.withAlpha (.012f));
    for (float y = r.getY(); y < r.getBottom(); y += 3.f) g.fillRect (r.getX(), y, r.getWidth(), 1.f);
    g.setColour (juce::Colours::white.withAlpha (.06f)); g.fillRect (r.getX(), r.getY(), r.getWidth(), 1.f);
}

void paintRecess (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour (colours::recess); g.fillRoundedRectangle (r, radius);
    // ombra interna in alto e a sinistra
    juce::Graphics::ScopedSaveState s (g);
    juce::Path clip; clip.addRoundedRectangle (r, radius); g.reduceClipRegion (clip);
    const float sh = juce::jmin (8.f, r.getHeight() * .35f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (.75f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + sh, false));
    g.fillRect (r.withHeight (sh));
    g.setColour (colours::recessEdge); g.fillRect (r.getX() + radius, r.getBottom() - 1.f, r.getWidth() - 2 * radius, 1.f);
}

void paintEngravedLine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b)
{
    const bool vertical = std::abs (a.x - b.x) < std::abs (a.y - b.y);
    g.setColour (juce::Colours::black.withAlpha (.8f)); g.drawLine ({ a, b }, 1.f);
    g.setColour (colours::engrave); g.drawLine ({ a.translated (vertical ? 1.f : 0.f, vertical ? 0.f : 1.f), b.translated (vertical ? 1.f : 0.f, vertical ? 0.f : 1.f) }, 1.f);
}

float chipWidth (const juce::String& text, float height)
{
    const auto f = Fonts::display (height * .82f);
    return juce::GlyphArrangement::getStringWidth (f, text.toUpperCase()) + height * .7f;
}

void paintChip (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float fontHeight)
{
    g.setColour (colours::chip); g.fillRoundedRectangle (r, 2.f);
    g.setColour (colours::chipText);
    g.setFont (Fonts::display (fontHeight > 0 ? fontHeight : r.getHeight() * .82f));
    g.drawText (text.toUpperCase(), r.translated (0, .5f), juce::Justification::centred, false);
}

void paintPrinted (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float h, juce::Justification j, juce::Colour c)
{
    g.setColour (c); g.setFont (Fonts::label (h));
    g.drawText (text.toUpperCase(), r, j, true);
}

void paintLeaderLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float h)
{
    const float lw = juce::jmin (14.f, r.getWidth() * .2f);
    g.setColour (colours::print); g.fillRect (r.getX(), r.getCentreY() - .5f, lw, 1.f);
    paintPrinted (g, r.withTrimmedLeft (lw + 4), text, h);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, float rad, juce::Colour col, float level)
{
    level = juce::jlimit (0.f, 1.f, level);
    if (level > .01f)
    {
        juce::ColourGradient glow (col.withAlpha (.55f * level), c.x, c.y, col.withAlpha (0.f), c.x + rad * 3.2f, c.y, true);
        g.setGradientFill (glow); g.fillEllipse (juce::Rectangle<float> (rad * 6.4f, rad * 6.4f).withCentre (c));
    }
    const auto body = juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c);
    g.setColour (juce::Colours::black); g.fillEllipse (body.expanded (1.f));
    const auto on = colours::ledOff.interpolatedWith (col, level);
    juce::ColourGradient core (on.interpolatedWith (juce::Colours::white, .55f * level), c.x - rad * .3f, c.y - rad * .3f, on, c.x + rad, c.y + rad, true);
    g.setGradientFill (core); g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (.12f + .25f * level)); g.fillEllipse (body.reduced (rad * .55f).translated (-rad * .2f, -rad * .25f));
}

void paintTicks (juce::Graphics& g, juce::Rectangle<float> r, int count, bool vertical, int majorEvery)
{
    g.setColour (colours::tick.withAlpha (.75f));
    for (int i = 0; i < count; ++i)
    {
        const float t = count > 1 ? (float) i / (float) (count - 1) : .5f;
        const bool major = majorEvery > 0 && (i % majorEvery == 0 || i == count - 1);
        if (vertical) { const float y = r.getBottom() - t * r.getHeight(); const float len = major ? r.getWidth() : r.getWidth() * .55f; g.fillRect (r.getRight() - len, y - .5f, len, 1.f); }
        else          { const float x = r.getX() + t * r.getWidth(); const float len = major ? r.getHeight() : r.getHeight() * .55f; g.fillRect (x - .5f, r.getY(), 1.f, len); }
    }
}

static juce::Image brandImage (bool full)
{
    int size = 0;
    const char* data = MBStudioUIFonts::getNamedResource (full ? "mbstudio91logo_png" : "mbstudio91wordmark_png", size);
    return data != nullptr ? juce::ImageCache::getFromMemory (data, size) : juce::Image();
}

juce::Rectangle<float> paintLogo (juce::Graphics& g, juce::Rectangle<float> r, bool full, Imprint style, juce::Justification j)
{
    const auto img = brandImage (full);
    if (! img.isValid()) return {};
    const float s = juce::jmin (r.getWidth() / (float) img.getWidth(), r.getHeight() / (float) img.getHeight());
    auto dest = juce::Rectangle<float> ((float) img.getWidth() * s, (float) img.getHeight() * s);
    dest = dest.withPosition (j.testFlags (juce::Justification::right) ? r.getRight() - dest.getWidth()
                            : j.testFlags (juce::Justification::horizontallyCentred) ? r.getCentreX() - dest.getWidth() * .5f : r.getX(),
                              r.getCentreY() - dest.getHeight() * .5f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    auto stamp = [&] (juce::Colour c, float dy)
    {
        g.setColour (c);
        g.drawImage (img, dest.translated (0, dy), juce::RectanglePlacement::stretchToFit, true);
    };
    switch (style)
    {
        case Imprint::printed:  stamp (colours::print.withAlpha (.96f), 0); break;                         // inchiostro bianco opaco
        case Imprint::debossed: stamp (juce::Colours::white.withAlpha (.13f), 1.f);                         // luce sul bordo basso
                                stamp (juce::Colours::black.withAlpha (.9f), -1.f);                         // ombra sul bordo alto
                                stamp (juce::Colour (0xff09090a), 0); break;                                // fondo inciso
        case Imprint::embossed: stamp (juce::Colours::black.withAlpha (.85f), 1.2f);
                                stamp (juce::Colours::white.withAlpha (.16f), -1.f);
                                stamp (colours::chassisTop.brighter (.15f), 0); break;
    }
    return dest;
}

void paintBrand (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& badge, Imprint style)
{
    const float h = r.getHeight();
    const auto logo = paintLogo (g, r.withHeight (h).withWidth (r.getWidth() * .7f), false, style);
    if (badge.isEmpty()) return;
    auto bf = Fonts::display (h * .78f).withExtraKerningFactor (.1f);
    const float bw = juce::GlyphArrangement::getStringWidth (bf, badge.toUpperCase()) + h * .6f;
    const juce::Rectangle<float> b (logo.getRight() + h * .35f, r.getCentreY() - h * .44f, bw, h * .88f);
    g.setColour (style == Imprint::printed ? colours::print : colours::printDim);
    g.drawRoundedRectangle (b.reduced (.75f), 3.f, 1.5f);
    g.setFont (bf); g.drawText (badge.toUpperCase(), b.translated (h * .02f, h * .04f), juce::Justification::centred, false);
}

//==============================================================================
HardwareLookAndFeel::HardwareLookAndFeel()
{
    using namespace colours;
    setColour (juce::ResizableWindow::backgroundColourId, chassisBottom);
    setColour (juce::DocumentWindow::backgroundColourId, chassisBottom);
    setColour (juce::Label::textColourId, print);
    setColour (juce::TextButton::buttonColourId, key);
    setColour (juce::TextButton::buttonOnColourId, key);
    setColour (juce::TextButton::textColourOffId, print);
    setColour (juce::TextButton::textColourOnId, print);
    setColour (juce::ToggleButton::textColourId, print);
    setColour (juce::ToggleButton::tickColourId, ledGreen);
    setColour (juce::Slider::backgroundColourId, recess);
    setColour (juce::Slider::trackColourId, print);
    setColour (juce::Slider::thumbColourId, print);
    setColour (juce::ComboBox::backgroundColourId, recess);
    setColour (juce::ComboBox::outlineColourId, recessEdge);
    setColour (juce::ComboBox::textColourId, print);
    setColour (juce::ComboBox::arrowColourId, print);
    setColour (juce::PopupMenu::backgroundColourId, chassisTop);
    setColour (juce::PopupMenu::textColourId, print);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, print.withAlpha (.12f));
    setColour (juce::PopupMenu::highlightedTextColourId, print);
    setColour (juce::PopupMenu::headerTextColourId, printDim);
    setColour (juce::TextEditor::backgroundColourId, recess);
    setColour (juce::TextEditor::textColourId, print);
    setColour (juce::TextEditor::highlightColourId, ledBlue.withAlpha (.35f));
    setColour (juce::TextEditor::outlineColourId, recessEdge);
    setColour (juce::TextEditor::focusedOutlineColourId, print.withAlpha (.5f));
    setColour (juce::CaretComponent::caretColourId, print);
    setColour (juce::ScrollBar::thumbColourId, print.withAlpha (.25f));
    setColour (juce::TooltipWindow::backgroundColourId, chip);
    setColour (juce::TooltipWindow::textColourId, chipText);
    setColour (juce::TooltipWindow::outlineColourId, chip);
    setColour (juce::AlertWindow::backgroundColourId, chassisTop);
    setColour (juce::AlertWindow::textColourId, print);
   #if JUCE_MODULE_AVAILABLE_juce_gui_extra
    setColour (juce::CodeEditorComponent::backgroundColourId, recess);
    setColour (juce::CodeEditorComponent::defaultTextColourId, print);
    setColour (juce::CodeEditorComponent::lineNumberBackgroundId, chassisTop);
    setColour (juce::CodeEditorComponent::lineNumberTextId, printDim);
    setColour (juce::CodeEditorComponent::highlightColourId, ledBlue.withAlpha (.3f));
   #endif
    setColour (juce::ListBox::backgroundColourId, recess);
}

void HardwareLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    using namespace colours;
    auto r = b.getLocalBounds().toFloat().reduced (1.f, 1.f);
    r.removeFromBottom (1.5f);
    const float rad = juce::jmin (4.f, r.getHeight() * .18f);
    // ombra sotto il tasto
    if (! down) { g.setColour (juce::Colours::black.withAlpha (.7f)); g.fillRoundedRectangle (r.translated (0, 1.5f), rad); }
    if (down) r = r.translated (0, 1.f);
    juce::ColourGradient body (down ? key.darker (.3f) : keyTop.brighter (highlighted ? .12f : 0.f), r.getX(), r.getY(), key, r.getX(), r.getBottom(), false);
    g.setGradientFill (body); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (r, rad, 1.f);
    if (! down) { g.setColour (keyEdge.withAlpha (.9f)); g.fillRect (r.getX() + rad, r.getY() + 1.f, r.getWidth() - 2 * rad, 1.f); }
    if (! b.isEnabled()) { g.setColour (juce::Colours::black.withAlpha (.45f)); g.fillRoundedRectangle (r, rad); }
    // LED per interruttori (o se richiesto)
    if (b.getClickingTogglesState() || (bool) b.getProperties()["led"])
    {
        const float lr = juce::jlimit (2.5f, 3.6f, r.getHeight() * .12f);
        paintLed (g, { r.getX() + 7.f + lr, r.getCentreY() }, lr, ledColourOf (b, ledGreen), b.getToggleState() ? 1.f : 0.f);
    }
}

void HardwareLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (4.f, 2.f);
    r.removeFromBottom (1.5f);
    if (down) r = r.translated (0, 1.f);
    if (b.getClickingTogglesState() || (bool) b.getProperties()["led"]) r.removeFromLeft (juce::jlimit (8.f, 12.f, r.getHeight() * .5f));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (colours::print.withMultipliedAlpha (b.isEnabled() ? 1.f : .4f));
    g.drawFittedText (b.getButtonText().toUpperCase(), r.toNearestInt(), juce::Justification::centred, 1, .7f);
}

juce::Font HardwareLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return Fonts::display (juce::jlimit (11.f, 18.f, (float) h * .58f)); }

void HardwareLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const float h = (float) b.getHeight(), lr = juce::jlimit (2.5f, 4.f, h * .16f);
    paintLed (g, { lr + 4.f, h * .5f }, lr, ledColourOf (b, colours::ledGreen), b.getToggleState() ? 1.f : 0.f);
    paintPrinted (g, b.getLocalBounds().toFloat().withTrimmedLeft (lr * 2 + 10.f), b.getButtonText(), juce::jlimit (11.f, 16.f, h * .6f),
                  juce::Justification::centredLeft, b.isEnabled() ? colours::print : colours::printDim);
}

void HardwareLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    using namespace colours;
    const bool vertical = style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical;
    const double mn = s.getMinimum(), mx = s.getMaximum();
    const bool bipolar = (bool) s.getProperties()["bipolar"] || (mn < 0 && mx > 0);
    const auto fillCol = s.findColour (juce::Slider::trackColourId);
    if (! vertical)
    {
        const float cy = (float) y + (float) h * .42f;
        const juce::Rectangle<float> slot ((float) x, cy - 3.f, (float) w, 6.f);
        paintRecess (g, slot, 3.f);
        const float zero = bipolar ? (float) x + (float) w * (float) (-mn / (mx - mn)) : (float) x;
        g.setColour (fillCol.withAlpha (.42f)); g.fillRect (juce::Rectangle<float> (juce::jmin (zero, pos), cy - 1.f, std::abs (pos - zero), 2.f));
        if (h >= 16) paintTicks (g, { (float) x, cy + 6.f, (float) w, juce::jmin (5.f, (float) (y + h) - cy - 6.f) }, 11, false, 5);
        // cappuccio del cursore: blocchetto nero con riga bianca
        const juce::Rectangle<float> cap (pos - 5.5f, cy - 8.f, 11.f, 16.f);
        g.setColour (juce::Colours::black.withAlpha (.7f)); g.fillRoundedRectangle (cap.translated (0, 1.5f), 2.f);
        g.setGradientFill (juce::ColourGradient (keyEdge, cap.getX(), cap.getY(), key, cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 2.f);
        g.setColour (juce::Colours::black); g.drawRoundedRectangle (cap, 2.f, 1.f);
        g.setColour (print); g.fillRect (cap.getCentreX() - .75f, cap.getY() + 3.f, 1.5f, cap.getHeight() - 6.f);
    }
    else
    {
        const float cx = (float) x + (float) w * .5f;
        const juce::Rectangle<float> slot (cx - 3.f, (float) y, 6.f, (float) h);
        paintRecess (g, slot, 3.f);
        paintTicks (g, { cx - 16.f, (float) y, 8.f, (float) h }, 11, true, 5);
        paintTicks (g, { cx + 8.f, (float) y, 8.f, (float) h }, 11, true, 5);
        const juce::Rectangle<float> cap (cx - 13.f, pos - 7.f, 26.f, 14.f);
        g.setColour (juce::Colours::black.withAlpha (.7f)); g.fillRoundedRectangle (cap.translated (0, 2.f), 2.f);
        g.setGradientFill (juce::ColourGradient (keyEdge, cap.getX(), cap.getY(), key, cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 2.f);
        g.setColour (juce::Colours::black); g.drawRoundedRectangle (cap, 2.f, 1.f);
        g.setColour (juce::Colours::white.withAlpha (.12f));
        for (int i = -2; i <= 2; ++i) if (i != 0) g.fillRect (cap.getX() + 4.f, cap.getCentreY() + (float) i * 2.4f, cap.getWidth() - 8.f, 1.f);
        g.setColour (print); g.fillRect (cap.getX() + 3.f, cap.getCentreY() - .75f, cap.getWidth() - 6.f, 1.5f);
    }
}

void HardwareLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    using namespace colours;
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float rad = juce::jmin (area.getWidth(), area.getHeight()) * .5f, ang = start + pos * (end - start);
    const auto c = area.getCentre();
    // scala a tacche intorno
    g.setColour (tick.withAlpha (.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (float) i / 10.f * (end - start);
        const float r0 = rad * (i % 5 == 0 ? .8f : .86f), r1 = rad;
        g.drawLine (c.x + r0 * std::sin (a), c.y - r0 * std::cos (a), c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), 1.f);
    }
    const float kr = rad * .72f;
    const auto knob = juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (.7f)); g.fillEllipse (knob.translated (0, 2.f));
    g.setGradientFill (juce::ColourGradient (keyEdge, c.x, knob.getY(), key, c.x, knob.getBottom(), false)); g.fillEllipse (knob);
    g.setColour (juce::Colours::black.withAlpha (.6f));
    for (int i = 0; i < 24; ++i) { const float a = (float) i / 24.f * juce::MathConstants<float>::twoPi; g.drawLine (c.x + kr * .86f * std::sin (a), c.y - kr * .86f * std::cos (a), c.x + kr * std::sin (a), c.y - kr * std::cos (a), 1.f); }
    g.setColour (juce::Colours::black); g.drawEllipse (knob, 1.f);
    g.setColour (s.findColour (juce::Slider::thumbColourId));
    g.drawLine (c.x + kr * .25f * std::sin (ang), c.y - kr * .25f * std::cos (ang), c.x + kr * .82f * std::sin (ang), c.y - kr * .82f * std::cos (ang), 2.f);
}

void HardwareLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    paintRecess (g, juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (.5f), 3.f);
    if (box.hasKeyboardFocus (false)) { g.setColour (colours::print.withAlpha (.35f)); g.drawRoundedRectangle (juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (.5f), 3.f, 1.f); }
    juce::Path tri; const float ax = (float) w - 12.f, ay = (float) h * .5f;
    tri.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (colours::print.withMultipliedAlpha (box.isEnabled() ? 1.f : .4f)); g.fillPath (tri);
}

juce::Font HardwareLookAndFeel::getComboBoxFont (juce::ComboBox& b) { return Fonts::label (juce::jlimit (12.f, 17.f, (float) b.getHeight() * .62f)); }

void HardwareLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void HardwareLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    paintChassis (g, { 0, 0, w, h });
    g.setColour (juce::Colours::black); g.drawRect (0, 0, w, h, 1);
}

void HardwareLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool separator, bool active, bool highlighted, bool ticked, bool hasSub,
                                             const juce::String& text, const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (separator) { paintEngravedLine (g, { (float) area.getX() + 8.f, (float) area.getCentreY() }, { (float) area.getRight() - 8.f, (float) area.getCentreY() }); return; }
    auto r = area.toFloat().reduced (3.f, 1.f);
    if (highlighted && active) { g.setColour (colours::print.withAlpha (.1f)); g.fillRoundedRectangle (r, 3.f); }
    if (ticked) paintLed (g, { r.getX() + 9.f, r.getCentreY() }, 3.f, colours::ledBlue, 1.f);
    juce::ignoreUnused (icon, hasSub);
    g.setColour (textColour != nullptr ? *textColour : (active ? colours::print : colours::printDim));
    g.setFont (getPopupMenuFont());
    g.drawFittedText (text, r.withTrimmedLeft (20.f).toNearestInt(), juce::Justification::centredLeft, 1);
    if (shortcut.isNotEmpty()) { g.setColour (colours::printDim); g.drawText (shortcut, r.withTrimmedRight (6.f), juce::Justification::centredRight); }
}

juce::Font HardwareLookAndFeel::getPopupMenuFont() { return Fonts::label (15.f); }
juce::Font HardwareLookAndFeel::getLabelFont (juce::Label& l) { return Fonts::label (l.getFont().getHeight()); }

void HardwareLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    paintRecess (g, juce::Rectangle<float> (0, 0, (float) w, (float) h), 3.f);
}

void HardwareLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    if (e.hasKeyboardFocus (true) && ! e.isReadOnly()) { g.setColour (colours::print.withAlpha (.45f)); g.drawRoundedRectangle (juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (.5f), 3.f, 1.f); }
}

void HardwareLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbPos, int thumbSize, bool over, bool down)
{
    juce::Rectangle<float> t = vertical ? juce::Rectangle<float> ((float) x + (float) w * .3f, (float) thumbPos, (float) w * .4f, (float) thumbSize)
                                        : juce::Rectangle<float> ((float) thumbPos, (float) y + (float) h * .3f, (float) thumbSize, (float) h * .4f);
    g.setColour (colours::print.withAlpha (down ? .45f : over ? .35f : .2f)); g.fillRoundedRectangle (t, 2.f);
}

void HardwareLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    g.setColour (colours::chip); g.fillRoundedRectangle (juce::Rectangle<float> (0, 0, (float) w, (float) h), 3.f);
    juce::AttributedString s; s.setJustification (juce::Justification::centredLeft); s.append (text, Fonts::label (14.f), colours::chipText);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, (float) w - 16.f);
    tl.draw (g, { 8.f, 5.f, (float) w - 16.f, (float) h - 10.f });
}

juce::Rectangle<int> HardwareLookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, Fonts::label (14.f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 360.f);
    const int w = (int) tl.getWidth() + 18, h = (int) tl.getHeight() + 12;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12, pos.y > parent.getCentreY() ? pos.y - (h + 6) : pos.y + 6, w, h).constrainedWithin (parent);
}

void HardwareLookAndFeel::drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&)
{
    g.setColour (colours::chassisTop); g.fillPath (path);
    g.setColour (juce::Colours::black); g.strokePath (path, juce::PathStrokeType (1.f));
}

void HardwareLookAndFeel::drawDocumentWindowTitleBar (juce::DocumentWindow& win, juce::Graphics& g, int w, int h, int tx, int tw, const juce::Image*, bool left)
{
    paintChassis (g, { 0, 0, w, h });
    paintEngravedLine (g, { 0.f, (float) h - 1.f }, { (float) w, (float) h - 1.f });
    paintPrinted (g, juce::Rectangle<float> ((float) tx, 0, (float) tw, (float) h), win.getName(), juce::jmin (17.f, (float) h * .62f),
                  left ? juce::Justification::centredLeft : juce::Justification::centred);
}

//==============================================================================
void Led::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    paintLed (g, r.getCentre(), juce::jmin (r.getWidth(), r.getHeight()) * .22f, colour, level);
}

void LedMeter::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float gap = 2.f, sw = (r.getWidth() - gap * (float) (n - 1)) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) (i + 1) / (float) n;
        const juce::Colour c = i == n - 1 ? colours::ledRed : (t > .75f ? colours::ledAmber : colour);
        const juce::Rectangle<float> seg (r.getX() + (float) i * (sw + gap), r.getY(), sw, r.getHeight());
        const bool on = level >= t - .5f / (float) n;
        g.setColour (on ? c : colours::ledOff); g.fillRoundedRectangle (seg, 1.f);
        if (on) { g.setColour (juce::Colours::white.withAlpha (.25f)); g.fillRect (seg.withHeight (1.f).translated (0, 1.f).reduced (1.f, 0)); }
    }
}
}
