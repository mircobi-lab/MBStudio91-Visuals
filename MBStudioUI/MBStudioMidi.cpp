#include "MBStudioMidi.h"
#include "MBStudioLookAndFeel.h"

namespace mbstudio::midi
{
namespace C = mbstudio::ui::colours;

// ================================================================================================ profili
// Valori di fabbrica dai documenti dei produttori / profili hardware pubblicati (vedi regole MBStudio).
// I controller sono riprogrammabili: il MIDI Learn resta sempre disponibile per correggere.
const std::vector<Profile>& profiles()
{
    static const std::vector<Profile> p = []
    {
        std::vector<Profile> v;
        auto row = [] (std::vector<ProfileSlot>& s, char bank, int firstCc, int channel = 0, Mode m = Mode::absolute)
        {
            for (int i = 0; i < 8; ++i) s.push_back ({ bank, i, channel, false, firstCc + i, m });
        };
        {   // Launch Control XL 3 prima della mk1/mk2: il nome della porta contiene anche "Launch Control XL"
            Profile pr { "NOVATION LAUNCH CONTROL XL 3", { "launch control xl 3", "lcxl3" }, true,
                         "MODO 16 (DEFAULT): ENCODER CC 13-36, FADER CC 5-12", {} };
            row (pr.slots, 'A', 13); row (pr.slots, 'B', 21); row (pr.slots, 'D', 29); row (pr.slots, 'C', 5);
            v.push_back (pr);
        }
        {
            Profile pr { "NOVATION LAUNCH CONTROL XL", { "launch control xl" }, true,
                         "USER TEMPLATE 1 DI FABBRICA: MANOPOLE CC 13-20 / 29-36 / 49-56, FADER CC 77-84", {} };
            row (pr.slots, 'A', 13); row (pr.slots, 'B', 29); row (pr.slots, 'D', 49); row (pr.slots, 'C', 77);
            v.push_back (pr);
        }
        {
            Profile pr { "NOVATION LAUNCHKEY MINI MK3", { "launchkey mini mk3", "launchkey mini" }, true,
                         "MANOPOLE IN UN MODO NON CUSTOM: CC 21-28 SUL CANALE 16", {} };
            row (pr.slots, 'A', 21, 16);
            v.push_back (pr);
        }
        {
            Profile pr { "NOVATION LAUNCHKEY MK3 / MK4", { "launchkey" }, false,
                         "CUSTOM MODE: CONTROLLARE I CC IN NOVATION COMPONENTS", {} };
            row (pr.slots, 'A', 21);
            v.push_back (pr);
        }
        {
            Profile pr { "AKAI APC40 MKII", { "apc40 mkii", "apc40 mk2", "apc40" }, true,
                         "GENERIC MODE (ALL'ACCENSIONE): DEVICE KNOB CC 16-23, TRACK KNOB CC 48-55, FADER CC 7 CANALI 1-8", {} };
            row (pr.slots, 'A', 16); row (pr.slots, 'B', 48);
            for (int i = 0; i < 8; ++i) pr.slots.push_back ({ 'C', i, i + 1, false, 7, Mode::absolute });
            pr.slots.push_back ({ 'D', 0, 0, false, 14, Mode::absolute });   // master fader
            pr.slots.push_back ({ 'D', 1, 0, false, 15, Mode::absolute });   // crossfader
            pr.slots.push_back ({ 'D', 2, 0, false, 47, Mode::relTwos });    // cue level (relativo)
            v.push_back (pr);
        }
        {
            Profile pr { "AKAI MPK MINI MK3", { "mpk mini 3", "mpk mini mk3", "mpk mini" }, true,
                         "PROGRAM 1 DI FABBRICA: MANOPOLE CC 70-77", {} };
            row (pr.slots, 'A', 70);
            v.push_back (pr);
        }
        {
            Profile pr { "M-AUDIO OXYGEN PRO 49/61", { "oxygen pro" }, false,
                         "PRESET MODE: NOTE SOLO LE MANOPOLE 3-6 (CC 46, 47, 62, 63); LE ALTRE CON MIDI LEARN", {} };
            const int cc[4] = { 46, 47, 62, 63 };
            for (int i = 0; i < 4; ++i) pr.slots.push_back ({ 'A', i + 2, 0, false, cc[i], Mode::absolute });
            v.push_back (pr);
        }
        {
            Profile pr { "M-AUDIO OXYGEN 49", { "oxygen 49", "oxygen" }, false,
                         "PRESET 1: NOTE SOLO LE MANOPOLE 1-2 (CC 75, 76); LE ALTRE CON MIDI LEARN", {} };
            pr.slots.push_back ({ 'A', 0, 0, false, 75, Mode::absolute });
            pr.slots.push_back ({ 'A', 1, 0, false, 76, Mode::absolute });
            v.push_back (pr);
        }
        {
            Profile pr { "ABLETON PUSH 2 (PORTA USER)", { "push 2" }, true,
                         "USARE LA PORTA USER; IN LIVE SPEGNERE TRACK/SYNC/REMOTE DELLA PORTA 2", {} };
            row (pr.slots, 'A', 71, 0, Mode::relTwos);
            pr.slots.push_back ({ 'D', 0, 0, false, 79, Mode::relTwos });    // encoder master
            v.push_back (pr);
        }
        return v;
    }();
    return p;
}

juce::String modeName (Mode m)
{
    switch (m)
    {
        case Mode::absolute:  return "ASSOLUTO";
        case Mode::relTwos:   return "RELATIVO (1-63 / 127-64)";
        case Mode::relOffset: return "RELATIVO (OFFSET 64)";
        case Mode::toggle:    return "COMMUTA";
        case Mode::momentary: return "MOMENTANEO";
    }
    return {};
}

juce::String Binding::describe() const
{
    return juce::String (note ? "NOTA " : "CC ") + juce::String (number) + (channel > 0 ? " CH " + juce::String (channel) : juce::String (" OMNI"));
}

static const juce::Identifier propParam { "mbsParam" };

void tag (juce::Component& c, const juce::String& paramId) { c.getProperties().set (propParam, paramId); }

juce::String taggedParam (const juce::Component* c)
{
    for (; c != nullptr; c = c->getParentComponent())
        if (c->getProperties().contains (propParam))
            return c->getProperties()[propParam].toString();
    return {};
}

// ================================================================================================ mapper
MidiMapper::MidiMapper (juce::AudioProcessor& p, const juce::String& pluginFolder) : proc (p), folder (pluginFolder)
{
    params = proc.getParameters();
    for (int i = 0; i < params.size(); ++i)
        if (auto* wid = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[i]))
            indexOf.set (wid->getParameterID(), i);
    for (auto& t : table) t.store (0);
    lastHw.fill (-1.f);
    scratch.ensureSize (8192);
    loadDefault();
}

MidiMapper::~MidiMapper() { cancelPendingUpdate(); }

void MidiMapper::setBanks (std::array<juce::StringArray, 4> b) { banks = std::move (b); }

void MidiMapper::rebuildTable()
{
    for (auto& t : table) t.store (0, std::memory_order_relaxed);
    lastHw.fill (-1.f);
    picked.fill (0);
    const juce::ScopedLock sl (lock);
    for (auto& b : list)
    {
        if (! indexOf.contains (b.paramId)) continue;
        const uint32_t entry = (uint32_t) (indexOf[b.paramId] + 1) | ((uint32_t) b.mode << 16);
        for (int ch = 0; ch < 16; ++ch)
            if (b.channel == 0 || b.channel == ch + 1)
            {
                const int slot = (b.note ? 2048 : 0) + ch * 128 + juce::jlimit (0, 127, b.number);
                table[(size_t) slot].store (entry, std::memory_order_release);
            }
    }
}

void MidiMapper::apply (int slot, uint32_t entry, int value, bool noteOn) noexcept
{
    auto* prm = params[(int) (entry & 0xFFFF) - 1];
    if (prm == nullptr) return;
    const auto mode = (Mode) ((entry >> 16) & 0xFF);
    const float cur = prm->getValue();
    const int steps = prm->getNumSteps();
    const bool isNote = slot >= 2048;
    float v = cur;
    switch (mode)
    {
        case Mode::absolute:
        {
            v = isNote ? (noteOn ? (float) value / 127.f : 0.f) : (float) value / 127.f;
            if (steps > 16 && ! isNote)        // soft takeover: si aggancia quando la manopola raggiunge il valore
            {
                auto& pk = picked[(size_t) slot];
                const float last = lastHw[(size_t) slot];
                if (pk && std::abs (v - cur) > 0.2f && std::abs (v - last) < 0.1f) pk = 0;   // cambiato altrove
                if (! pk && (std::abs (v - cur) <= 0.02f || (last >= 0 && (last - cur) * (v - cur) <= 0.f))) pk = 1;
                lastHw[(size_t) slot] = v;
                if (! pk) return;
            }
            break;
        }
        case Mode::relTwos:
        case Mode::relOffset:
        {
            int delta = mode == Mode::relTwos ? (value < 64 ? value : value - 128) : value - 64;
            if (delta == 0) return;
            const float step = (steps > 1 && steps <= 256) ? 1.f / (float) (steps - 1) : 1.f / 200.f;
            v = juce::jlimit (0.f, 1.f, cur + (float) delta * step);
            break;
        }
        case Mode::toggle:
            if (isNote ? ! noteOn : value < 64) return;
            v = cur < 0.5f ? 1.f : 0.f;
            break;
        case Mode::momentary:
            v = (isNote ? noteOn : value >= 64) ? 1.f : 0.f;
            break;
    }
    if (std::abs (v - cur) > 1.0e-6f) prm->setValueNotifyingHost (v);
}

void MidiMapper::process (juce::MidiBuffer& midi) noexcept
{
    const int lp = learnParam.load (std::memory_order_acquire);
    bool consumedAny = false;
    scratch.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const bool cc = m.isController() && m.getControllerNumber() < 120;
        const bool note = m.isNoteOnOrOff();
        bool keep = true;
        if (cc || note)
        {
            const int ch = m.getChannel() - 1;
            const int num = cc ? m.getControllerNumber() : m.getNoteNumber();
            if (lp >= 0 && (cc || m.isNoteOn()))
            {
                learned.store (0x80000000u | (note ? 0x8000u : 0u) | ((uint32_t) ch << 8) | (uint32_t) num, std::memory_order_release);
                learnParam.store (-1);
                triggerAsyncUpdate();
                keep = false;
            }
            else
            {
                const int slot = (note ? 2048 : 0) + ch * 128 + num;
                if (const auto entry = table[(size_t) slot].load (std::memory_order_acquire))
                {
                    apply (slot, entry, cc ? m.getControllerValue() : m.getVelocity(), m.isNoteOn());
                    // mod wheel, sustain restano anche al motore
                    keep = cc && (num == 1 || num == 64);
                }
            }
        }
        if (keep) scratch.addEvent (meta.data, meta.numBytes, meta.samplePosition);
        else consumedAny = true;
    }
    if (consumedAny) midi.swapWith (scratch);
}

void MidiMapper::handleAsyncUpdate()
{
    const auto l = learned.exchange (0);
    if ((l & 0x80000000u) == 0 || learning.isEmpty()) return;
    Binding b;
    b.note = (l & 0x8000u) != 0;
    b.channel = (int) ((l >> 8) & 0xF) + 1;
    b.number = (int) (l & 0x7F);
    b.paramId = learning;
    b.mode = b.note ? Mode::toggle : Mode::absolute;
    {
        const juce::ScopedLock sl (lock);
        list.erase (std::remove_if (list.begin(), list.end(), [&] (const Binding& o)
        {
            return o.paramId == b.paramId || (o.note == b.note && o.number == b.number && (o.channel == 0 || o.channel == b.channel));
        }), list.end());
        list.push_back (b);
    }
    learning.clear();
    rebuildTable();
    showStatus (b.describe() + "  >  " + paramName (b.paramId));
    changed();
}

void MidiMapper::startLearn (const juce::String& id)
{
    if (! indexOf.contains (id)) return;
    learning = id;
    learned.store (0);
    learnParam.store (indexOf[id], std::memory_order_release);
    showStatus ("MUOVI UN CONTROLLO PER  " + paramName (id));
    changed();
}

void MidiMapper::cancelLearn()
{
    learnParam.store (-1);
    learning.clear();
    changed();
}

juce::String MidiMapper::learningParam() const { return learning; }

void MidiMapper::setLearnMode (bool on)
{
    learnMode = on;
    if (! on) cancelLearn();
    showStatus (on ? "MIDI LEARN: CLIC SU UN CONTROLLO, POI MUOVI LA MANOPOLA" : "MIDI LEARN CHIUSO");
    changed();
}

void MidiMapper::forget (const juce::String& id)
{
    {
        const juce::ScopedLock sl (lock);
        list.erase (std::remove_if (list.begin(), list.end(), [&] (const Binding& o) { return o.paramId == id; }), list.end());
    }
    rebuildTable();
    showStatus ("ASSEGNAZIONE TOLTA: " + paramName (id));
    changed();
}

void MidiMapper::clearAll()
{
    {
        const juce::ScopedLock sl (lock);
        list.clear();
        profileIndex = -1;
    }
    rebuildTable();
    showStatus ("TUTTE LE ASSEGNAZIONI CANCELLATE");
    changed();
}

void MidiMapper::setMode (const juce::String& id, Mode m)
{
    {
        const juce::ScopedLock sl (lock);
        for (auto& b : list) if (b.paramId == id) b.mode = m;
    }
    rebuildTable();
    changed();
}

std::vector<Binding> MidiMapper::bindings() const
{
    const juce::ScopedLock sl (lock);
    return list;
}

const Binding* MidiMapper::bindingFor (const juce::String& id) const
{
    const juce::ScopedLock sl (lock);
    for (auto& b : list) if (b.paramId == id) return &b;
    return nullptr;
}

juce::String MidiMapper::describe (const juce::String& id) const
{
    const juce::ScopedLock sl (lock);
    for (auto& b : list) if (b.paramId == id) return b.describe();
    return {};
}

juce::String MidiMapper::paramName (const juce::String& id) const
{
    if (indexOf.contains (id)) return params[indexOf[id]]->getName (40).toUpperCase();
    return id.toUpperCase();
}

bool MidiMapper::applyProfile (int index)
{
    const auto& all = profiles();
    std::vector<Binding> next;
    if (index >= 0 && index < (int) all.size())
        for (auto& s : all[(size_t) index].slots)
        {
            const int bank = s.bank - 'A';
            if (bank < 0 || bank > 3 || s.index >= banks[(size_t) bank].size()) continue;
            const auto id = banks[(size_t) bank][s.index];
            if (id.isEmpty() || ! indexOf.contains (id)) continue;
            next.push_back ({ s.channel, s.note, s.number, id, s.mode });
        }
    {
        const juce::ScopedLock sl (lock);
        list = std::move (next);
        profileIndex = (index >= 0 && index < (int) all.size()) ? index : -1;
    }
    rebuildTable();
    if (profileIndex >= 0)
        showStatus (all[(size_t) profileIndex].name + ": " + all[(size_t) profileIndex].setupNote);
    else
        showStatus ("NESSUN PROFILO: SOLO MIDI LEARN");
    changed();
    return profileIndex >= 0;
}

bool MidiMapper::autoSelectFromPorts (const juce::StringArray& portNames)
{
    {
        const juce::ScopedLock sl (lock);
        if (! list.empty()) return false;
    }
    const auto& all = profiles();
    for (int i = 0; i < (int) all.size(); ++i)
        for (auto& port : portNames)
            for (auto& key : all[(size_t) i].portMatch)
                if (port.toLowerCase().contains (key))
                    return applyProfile (i);
    return false;
}

juce::ValueTree MidiMapper::toValueTree() const
{
    juce::ValueTree t ("MIDIMAP");
    const juce::ScopedLock sl (lock);
    if (profileIndex >= 0) t.setProperty ("profile", profiles()[(size_t) profileIndex].name, nullptr);
    for (auto& b : list)
    {
        juce::ValueTree c ("BIND");
        c.setProperty ("ch", b.channel, nullptr);
        c.setProperty ("note", b.note, nullptr);
        c.setProperty ("num", b.number, nullptr);
        c.setProperty ("param", b.paramId, nullptr);
        c.setProperty ("mode", (int) b.mode, nullptr);
        t.appendChild (c, nullptr);
    }
    return t;
}

void MidiMapper::fromValueTree (const juce::ValueTree& t)
{
    if (! t.hasType ("MIDIMAP")) return;
    std::vector<Binding> next;
    for (const auto& c : t)
    {
        Binding b;
        b.channel = juce::jlimit (0, 16, (int) c.getProperty ("ch", 0));
        b.note = (bool) c.getProperty ("note", false);
        b.number = juce::jlimit (0, 127, (int) c.getProperty ("num", 0));
        b.paramId = c.getProperty ("param").toString();
        b.mode = (Mode) juce::jlimit (0, 4, (int) c.getProperty ("mode", 0));
        if (indexOf.contains (b.paramId)) next.push_back (b);
    }
    int pi = -1;
    const auto pname = t.getProperty ("profile").toString();
    for (int i = 0; i < (int) profiles().size(); ++i)
        if (profiles()[(size_t) i].name == pname) pi = i;
    {
        const juce::ScopedLock sl (lock);
        list = std::move (next);
        profileIndex = pi;
    }
    rebuildTable();
    changed();
}

juce::File MidiMapper::defaultFile() const
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");
   #endif
    return base.getChildFile ("MBStudio91").getChildFile (folder).getChildFile ("midimap.xml");
}

bool MidiMapper::saveAsDefault()
{
    auto f = defaultFile();
    f.getParentDirectory().createDirectory();
    const bool ok = toValueTree().createXml()->writeTo (f);
    showStatus (ok ? "MAPPA SALVATA COME PREDEFINITA" : "SALVATAGGIO NON RIUSCITO");
    return ok;
}

bool MidiMapper::loadDefault()
{
    const auto f = defaultFile();
    if (! f.existsAsFile()) return false;
    if (auto xml = juce::XmlDocument::parse (f))
    {
        fromValueTree (juce::ValueTree::fromXml (*xml));
        return true;
    }
    return false;
}

void MidiMapper::showStatus (const juce::String& s)
{
    statusText = s;
    statusMs = juce::Time::getMillisecondCounter();
}

juce::String MidiMapper::status() const
{
    return juce::Time::getMillisecondCounter() - statusMs < 3500 ? statusText : juce::String();
}

void MidiMapper::changed() { changes.sendChangeMessage(); }

// ================================================================================================ overlay
MidiLearnOverlay::MidiLearnOverlay (MidiMapper& m) : map (m)
{
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
    map.changes.addChangeListener (this);
    startTimerHz (15);
}

MidiLearnOverlay::~MidiLearnOverlay()
{
    map.changes.removeChangeListener (this);
    if (root != nullptr) root->removeMouseListener (this);
}

void MidiLearnOverlay::parentHierarchyChanged()
{
    auto* p = getParentComponent();
    if (p == root) return;
    if (root != nullptr) root->removeMouseListener (this);
    root = p;
    if (root != nullptr)
    {
        root->addMouseListener (this, true);
        setBounds (root->getLocalBounds());
        toFront (false);
    }
    collect();
}

void MidiLearnOverlay::collect()
{
    tagged.clear();
    if (root == nullptr) return;
    std::function<void (juce::Component&)> walk = [&] (juce::Component& c)
    {
        for (auto* ch : c.getChildren())
        {
            if (ch == this) continue;
            if (ch->getProperties().contains ("mbsParam")) tagged.add (ch);
            walk (*ch);
        }
    };
    walk (*root);
}

void MidiLearnOverlay::timerCallback()
{
    ++ticks;
    if (root != nullptr && getBounds() != root->getLocalBounds()) setBounds (root->getLocalBounds());
    if (ticks % 15 == 0) collect();
    if (map.isLearnMode() || map.learningParam().isNotEmpty() || map.status().isNotEmpty() || ticks % 15 == 0) repaint();
}

void MidiLearnOverlay::paint (juce::Graphics& g)
{
    const auto learningId = map.learningParam();
    const bool blink = (ticks / 4) % 2 == 0;
    if (map.isLearnMode() || learningId.isNotEmpty())
    {
        for (auto& sp : tagged)
        {
            auto* c = sp.getComponent();
            if (c == nullptr || ! c->isShowing()) continue;
            const auto id = c->getProperties()["mbsParam"].toString();
            const auto r = getLocalArea (c, c->getLocalBounds()).toFloat().expanded (1.f);
            const auto d = map.describe (id);
            if (id == learningId)
            {
                g.setColour (C::ledAmber.withAlpha (blink ? 1.f : .45f));
                g.drawRoundedRectangle (r, 3.f, 2.f);
            }
            else if (map.isLearnMode())
            {
                g.setColour (C::ledAmber.withAlpha (d.isNotEmpty() ? .85f : .25f));
                g.drawRoundedRectangle (r, 3.f, 1.f);
            }
            if (d.isNotEmpty() && map.isLearnMode())
            {
                g.setFont (ui::Fonts::label (11.f));
                const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), d) + 6.f;
                const juce::Rectangle<float> badge (r.getX(), r.getY() - 1.f, w, 13.f);
                g.setColour (C::recess.withAlpha (.92f));
                g.fillRect (badge);
                g.setColour (C::ledAmber);
                g.drawText (d, badge, juce::Justification::centred, false);
            }
        }
    }
    const auto s = map.status();
    if (s.isNotEmpty())
    {
        g.setFont (ui::Fonts::label (14.f));
        const float w = juce::jmin ((float) getWidth() - 40.f, juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 40.f);
        const juce::Rectangle<float> box ((float) getWidth() * .5f - w * .5f, (float) getHeight() - 52.f, w, 30.f);
        ui::paintRecess (g, box);
        ui::paintLed (g, { box.getX() + 14.f, box.getCentreY() }, 3.5f, C::ledAmber, 1.f);
        g.setColour (C::print);
        g.drawText (s, box.withTrimmedLeft (24.f).withTrimmedRight (8.f), juce::Justification::centred, true);
    }
}

void MidiLearnOverlay::mouseDown (const juce::MouseEvent& e)
{
    auto* c = e.eventComponent;
    if (c == nullptr || c == this) return;
    const auto id = taggedParam (c);
    if (id.isEmpty()) return;
    auto* target = c;
    while (target != nullptr && ! target->getProperties().contains ("mbsParam")) target = target->getParentComponent();
    if (target == nullptr) return;
    if (e.mods.isPopupMenu()) showMenu (id, *target);
    else if (map.isLearnMode()) map.startLearn (id);
}

void MidiLearnOverlay::showMenu (const juce::String& id, juce::Component& target)
{
    juce::PopupMenu m, modes;
    m.addSectionHeader (map.paramName (id));
    m.addItem (1, "MIDI LEARN");
    const auto d = map.describe (id);
    m.addItem (2, "DIMENTICA" + (d.isNotEmpty() ? "  (" + d + ")" : juce::String()), d.isNotEmpty());
    Mode cur = Mode::absolute;
    if (auto* b = map.bindingFor (id)) cur = b->mode;
    for (int i = 0; i <= 4; ++i) modes.addItem (10 + i, modeName ((Mode) i), d.isNotEmpty(), cur == (Mode) i);
    m.addSubMenu ("MODO DEL CONTROLLO", modes, d.isNotEmpty());
    juce::Component::SafePointer<MidiLearnOverlay> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target), [safe, id] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& mp = safe->map;
        if (r == 1) mp.startLearn (id);
        else if (r == 2) mp.forget (id);
        else if (r >= 10 && r <= 14) mp.setMode (id, (Mode) (r - 10));
    });
}

// ================================================================================================ tasti
MidiControls::MidiControls (MidiMapper& m) : map (m)
{
    learnBtn.getProperties().set ("led", true);
    learnBtn.getProperties().set ("ledColour", C::ledAmber.toString());
    learnBtn.setTooltip ("MIDI LEARN: clic su un controllo del pannello, poi muovi la manopola del controller. Clic destro su un controllo per altre opzioni");
    learnBtn.onClick = [this] { map.setLearnMode (! map.isLearnMode()); };
    ctrlBtn.getProperties().set ("led", true);
    ctrlBtn.getProperties().set ("ledColour", C::ledBlue.toString());
    ctrlBtn.setTooltip ("Profili pronti per controller MIDI, mappa predefinita, elenco delle assegnazioni");
    ctrlBtn.onClick = [this] { showControllerMenu(); };
    addAndMakeVisible (learnBtn);
    addAndMakeVisible (ctrlBtn);
    map.changes.addChangeListener (this);
    changeListenerCallback (nullptr);
}

MidiControls::~MidiControls() { map.changes.removeChangeListener (this); }

void MidiControls::resized()
{
    auto r = getLocalBounds();
    const int w = (r.getWidth() - 6) * 2 / 5;
    learnBtn.setBounds (r.removeFromLeft (w));
    r.removeFromLeft (6);
    ctrlBtn.setBounds (r);
}

void MidiControls::changeListenerCallback (juce::ChangeBroadcaster*)
{
    learnBtn.setToggleState (map.isLearnMode() || map.learningParam().isNotEmpty(), juce::dontSendNotification);
    ctrlBtn.setToggleState (! map.bindings().empty(), juce::dontSendNotification);
}

void MidiControls::showControllerMenu()
{
    juce::PopupMenu m, list;
    m.addSectionHeader ("PROFILO CONTROLLER");
    m.addItem (1, "NESSUNO (SOLO MIDI LEARN)", true, map.currentProfile() < 0);
    const auto& all = profiles();
    for (int i = 0; i < (int) all.size(); ++i)
        m.addItem (100 + i, all[(size_t) i].name + (all[(size_t) i].verified ? juce::String() : "  (DA VERIFICARE)"), true, map.currentProfile() == i);
    m.addSeparator();
    const auto b = map.bindings();
    for (auto& x : b) list.addItem (-1, x.describe() + "  >  " + map.paramName (x.paramId) + "   [" + modeName (x.mode) + "]", false);
    if (b.empty()) list.addItem (-1, "NESSUNA", false);
    m.addSubMenu ("ASSEGNAZIONI (" + juce::String ((int) b.size()) + ")", list);
    m.addItem (200, "SALVA COME PREDEFINITA");
    m.addItem (201, "RICARICA PREDEFINITA", map.defaultFile().existsAsFile());
    m.addItem (202, "CANCELLA TUTTE LE ASSEGNAZIONI", ! b.empty());
    juce::Component::SafePointer<MidiControls> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&ctrlBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& mp = safe->map;
        if (r == 1) mp.applyProfile (-1);
        else if (r >= 100 && r < 200) mp.applyProfile (r - 100);
        else if (r == 200) mp.saveAsDefault();
        else if (r == 201) { mp.loadDefault(); mp.showStatus ("MAPPA PREDEFINITA RICARICATA"); }
        else if (r == 202) mp.clearAll();
    });
}
}
