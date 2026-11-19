#include "LightsWindow.h"

namespace
{
    const juce::Colour kBg (0xff0b0d12), kPanel (0xff141821), kLine (0xff252b38), kText (0xffe8ecf3), kDim (0xff8a93a6), kAmber (0xffffb547), kErr (0xffff5470), kOk (0xff4ade80);
    juce::Font f (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

    void styleEditor (juce::TextEditor& e, bool numeric)
    {
        e.setColour (juce::TextEditor::backgroundColourId, kPanel);
        e.setColour (juce::TextEditor::outlineColourId, kLine);
        e.setColour (juce::TextEditor::textColourId, kText);
        e.setFont (f (12.f));
        e.setIndents (5, 3);
        if (numeric) e.setInputRestrictions (8, "0123456789.");
    }
}

//==============================================================================
/* Anteprima con le zone */
class ZoneView : public juce::Component
{
public:
    ZoneView (LightsEngine& e) : eng (e) {}
    std::function<void (int)> onSelect;
    std::function<void()> onChanged;
    int selected = -1;
    LightsEngine::Config cfg;

    juce::Rectangle<float> imageArea() const
    {
        auto r = getLocalBounds().toFloat();
        const float w = juce::jmin (r.getWidth(), r.getHeight() * 16.f / 9.f);
        return r.withSizeKeepingCentre (w, w * 9.f / 16.f);
    }
    juce::Rectangle<float> zoneRect (const LightsEngine::Fixture& fx) const
    {
        const auto a = imageArea();
        return { a.getX() + fx.x * a.getWidth(), a.getY() + fx.y * a.getHeight(), fx.w * a.getWidth(), fx.h * a.getHeight() };
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (kBg);
        const auto a = imageArea();
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImage (eng.preview(), a);
        g.setColour (kLine); g.drawRect (a, 1.f);
        for (size_t i = 0; i < cfg.fixtures.size(); ++i)
        {
            const auto& fx = cfg.fixtures[i];
            const auto r = zoneRect (fx);
            const bool sel = (int) i == selected;
            g.setColour ((sel ? juce::Colours::white : kAmber).withAlpha (.12f)); g.fillRect (r);
            if (fx.ch.containsChar ('[') && fx.cols * fx.rows > 1 && fx.cols * fx.rows <= 4096)
            {
                g.setColour (juce::Colours::white.withAlpha (.18f));
                for (int c = 1; c < fx.cols; ++c) { const float x = r.getX() + r.getWidth() * (float) c / (float) fx.cols; g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
                for (int c = 1; c < fx.rows; ++c) { const float y = r.getY() + r.getHeight() * (float) c / (float) fx.rows; g.drawHorizontalLine ((int) y, r.getX(), r.getRight()); }
            }
            g.setColour (sel ? juce::Colours::white : kAmber); g.drawRect (r, sel ? 2.f : 1.5f);
            g.fillRect (juce::Rectangle<float> (r.getRight() - 9, r.getBottom() - 9, 9, 9));
            g.setFont (f (10.f)); g.setColour (juce::Colours::white);
            g.drawText (juce::String ((int) i + 1) + " " + fx.name + "  U" + juce::String (fx.universe) + "." + juce::String (fx.address),
                        r.reduced (3).toNearestInt(), juce::Justification::topLeft, true);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        drag = -1;
        for (int i = (int) cfg.fixtures.size() - 1; i >= 0; --i)
        {
            const auto r = zoneRect (cfg.fixtures[(size_t) i]);
            if (r.expanded (2).contains (e.position))
            {
                drag = i; resize = juce::Rectangle<float> (r.getRight() - 12, r.getBottom() - 12, 14, 14).contains (e.position);
                start = cfg.fixtures[(size_t) i];
                selected = i; if (onSelect) onSelect (i); repaint(); return;
            }
        }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto a = imageArea();
        const float dx = (float) e.getDistanceFromDragStartX() / a.getWidth(), dy = (float) e.getDistanceFromDragStartY() / a.getHeight();
        auto& fx = cfg.fixtures[(size_t) drag];
        if (resize) { fx.w = juce::jlimit (.005f, 1 - fx.x, start.w + dx); fx.h = juce::jlimit (.005f, 1 - fx.y, start.h + dy); }
        else { fx.x = juce::jlimit (0.f, 1 - fx.w, start.x + dx); fx.y = juce::jlimit (0.f, 1 - fx.h, start.y + dy); }
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override { if (drag >= 0 && onChanged) onChanged(); drag = -1; }

private:
    LightsEngine& eng;
    int drag = -1; bool resize = false; LightsEngine::Fixture start;
};

//==============================================================================
/* Una riga dell'elenco: tutti i campi di un apparecchio */
class FixtureRow : public juce::Component
{
public:
    std::function<void (const LightsEngine::Fixture&)> onEdit;
    std::function<void()> onDelete, onDuplicate, onIdentify, onSelect;

    FixtureRow()
    {
        for (auto* e : { &name, &x, &y, &w, &h, &cols, &rows, &uni, &addr, &ch, &dim })
        {
            styleEditor (*e, e != &name && e != &ch);
            e->onReturnKey = [this] { commit(); };
            e->onFocusLost = [this] { commit(); };
            addAndMakeVisible (*e);
        }
        const char* labels[] = { "X %", "Y %", "Largh. %", "Alt. %", "Colonne", "Righe", "Universo", "Indirizzo", "Canali", "Lumin. %", "Lampo (F)" };
        for (int i = 0; i < 11; ++i) { lab[i].setText (labels[i], juce::dontSendNotification); lab[i].setFont (f (10.f)); lab[i].setColour (juce::Label::textColourId, kDim); addAndMakeVisible (lab[i]); }
        flash.addItemList ({ "Strobo", "Ogni beat", "Bassi", juce::String::fromUTF8 ("Luminosità zona") }, 1);
        flash.onChange = [this] { commit(); };
        addAndMakeVisible (flash);
        for (auto* t : { &whole, &serp, &vert, &flipX, &flipY }) { t->onClick = [this] { commit(); }; t->setColour (juce::ToggleButton::textColourId, kText); addAndMakeVisible (*t); }
        whole.setButtonText ("Pixel interi"); serp.setButtonText ("Serpentina"); vert.setButtonText ("Per colonne"); flipX.setButtonText ("Inverti X"); flipY.setButtonText ("Inverti Y");
        idBtn.onClick = [this] { if (onIdentify) onIdentify(); };
        dupBtn.onClick = [this] { if (onDuplicate) onDuplicate(); };
        delBtn.onClick = [this] { if (onDelete) onDelete(); };
        idBtn.setTooltip ("Fa lampeggiare in bianco questo apparecchio per 3 secondi");
        for (auto* b : { &idBtn, &dupBtn, &delBtn }) addAndMakeVisible (*b);
        info.setFont (f (10.5f)); addAndMakeVisible (info);
        indexLabel.setFont (f (12.f, true)); indexLabel.setColour (juce::Label::textColourId, kAmber); addAndMakeVisible (indexLabel);
    }
    void set (int index, const LightsEngine::Fixture& fx, const LightsEngine::Info& inf, bool sel)
    {
        current = fx; selected = sel;
        indexLabel.setText (juce::String (index + 1), juce::dontSendNotification);
        auto pct = [] (float v) { return juce::String (v * 100.f, 1).trimCharactersAtEnd ("0").trimCharactersAtEnd ("."); };
        auto st = [] (juce::TextEditor& e, const juce::String& s) { if (! e.hasKeyboardFocus (true)) e.setText (s, juce::dontSendNotification); };
        st (name, fx.name); st (x, pct (fx.x)); st (y, pct (fx.y)); st (w, pct (fx.w)); st (h, pct (fx.h));
        st (cols, juce::String (fx.cols)); st (rows, juce::String (fx.rows)); st (uni, juce::String (fx.universe)); st (addr, juce::String (fx.address));
        st (ch, fx.ch); st (dim, juce::String (juce::roundToInt (fx.dim * 100)));
        const juce::StringArray fl { "strobe", "beat", "bass", "level" };
        flash.setSelectedId (fl.indexOf (fx.flash) + 1, juce::dontSendNotification);
        whole.setToggleState (fx.whole, juce::dontSendNotification); serp.setToggleState (fx.serp, juce::dontSendNotification);
        vert.setToggleState (fx.vert, juce::dontSendNotification); flipX.setToggleState (fx.flipX, juce::dontSendNotification); flipY.setToggleState (fx.flipY, juce::dontSendNotification);
        juce::String t = inf.count == 0 ? juce::String ("Nessun canale: scrivi i canali, es. [R G B]")
                       : juce::String (inf.count) + " canali" + (inf.pixels ? " - " + juce::String (inf.pixels) + " pixel" : juce::String())
                         + " - U" + juce::String (fx.universe) + "." + juce::String (fx.address) + " -> U" + juce::String (inf.endUniverse) + "." + juce::String (inf.endAddress);
        if (inf.overlap) t << juce::String::fromUTF8 ("  ·  ⚠ canali sovrapposti a un altro apparecchio");
        info.setText (t, juce::dontSendNotification);
        info.setColour (juce::Label::textColourId, inf.overlap || inf.count == 0 ? kErr : kDim);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        g.setColour (kPanel); g.fillRoundedRectangle (r, 8.f);
        g.setColour (selected ? kAmber : kLine); g.drawRoundedRectangle (r, 8.f, selected ? 1.5f : 1.f);
    }
    void mouseDown (const juce::MouseEvent&) override { if (onSelect) onSelect(); }
    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 8);
        auto top = r.removeFromTop (24);
        indexLabel.setBounds (top.removeFromLeft (24));
        delBtn.setBounds (top.removeFromRight (30)); top.removeFromRight (4);
        dupBtn.setBounds (top.removeFromRight (30)); top.removeFromRight (4);
        idBtn.setBounds (top.removeFromRight (80)); top.removeFromRight (6);
        name.setBounds (top);
        r.removeFromTop (4);
        auto grid = [&] (juce::Rectangle<int> row, std::initializer_list<std::pair<juce::Label*, juce::Component*>> items)
        {
            const int n = (int) items.size(), cw = (row.getWidth() - (n - 1) * 6) / n;
            for (auto& [l, c] : items) { auto cell = row.removeFromLeft (cw); row.removeFromLeft (6); l->setBounds (cell.removeFromTop (13)); c->setBounds (cell); }
        };
        grid (r.removeFromTop (37), { { &lab[0], &x }, { &lab[1], &y }, { &lab[2], &w }, { &lab[3], &h }, { &lab[4], &cols }, { &lab[5], &rows }, { &lab[6], &uni }, { &lab[7], &addr } });
        r.removeFromTop (4);
        auto row3 = r.removeFromTop (37);
        grid (row3.removeFromRight (210), { { &lab[9], &dim }, { &lab[10], &flash } }); row3.removeFromRight (6);
        lab[8].setBounds (row3.removeFromTop (13)); ch.setBounds (row3);
        r.removeFromTop (4);
        auto tg = r.removeFromTop (22); const int tw = tg.getWidth() / 5;
        for (auto* t : { &whole, &serp, &vert, &flipX, &flipY }) t->setBounds (tg.removeFromLeft (tw));
        info.setBounds (r.removeFromTop (18));
    }

private:
    void commit()
    {
        auto fx = current;
        auto pf = [] (juce::TextEditor& e) { return e.getText().getFloatValue() / 100.f; };
        fx.name = name.getText(); fx.x = pf (x); fx.y = pf (y); fx.w = pf (w); fx.h = pf (h);
        fx.cols = cols.getText().getIntValue(); fx.rows = rows.getText().getIntValue();
        fx.universe = uni.getText().getIntValue(); fx.address = addr.getText().getIntValue();
        fx.ch = ch.getText(); fx.dim = pf (dim);
        const juce::StringArray fl { "strobe", "beat", "bass", "level" };
        fx.flash = fl[juce::jlimit (0, 3, flash.getSelectedId() - 1)];
        fx.whole = whole.getToggleState(); fx.serp = serp.getToggleState(); fx.vert = vert.getToggleState();
        fx.flipX = flipX.getToggleState(); fx.flipY = flipY.getToggleState();
        if (onEdit) onEdit (LightsEngine::cleanFixture (fx));
    }
    LightsEngine::Fixture current; bool selected = false;
    juce::Label indexLabel, info, lab[11];
    juce::TextEditor name, x, y, w, h, cols, rows, uni, addr, ch, dim;
    juce::ComboBox flash;
    juce::ToggleButton whole, serp, vert, flipX, flipY;
    juce::TextButton idBtn { "Identifica" }, dupBtn { juce::String::fromUTF8 ("⧉") }, delBtn { juce::String::fromUTF8 ("✕") };
};

//==============================================================================
class LightsWindow::Panel : public juce::Component, private juce::Timer
{
public:
    explicit Panel (LightsEngine& e) : eng (e), zones (e)
    {
        auto lbl = [this] (juce::Label& l, const juce::String& t) { l.setText (t, juce::dontSendNotification); l.setFont (f (11.f)); l.setColour (juce::Label::textColourId, kDim); addAndMakeVisible (l); };
        on.setButtonText ("Invio attivo"); black.setButtonText ("Buio luci"); osc.setButtonText ("OSC");
        for (auto* t : { &on, &black, &osc }) { t->setColour (juce::ToggleButton::textColourId, kText); t->onClick = [this] { commitTop(); }; addAndMakeVisible (*t); }
        fps.addItemList ({ "20 fps", "30 fps", "40 fps", "44 fps" }, 1); fps.onChange = [this] { commitTop(); }; addAndMakeVisible (fps);
        proto.addItemList ({ "Art-Net", "sACN / E1.31", "Nessuna" }, 1); proto.onChange = [this] { commitTop(); }; addAndMakeVisible (proto);
        for (auto* t : { &ip, &oscHost, &oscPort }) { styleEditor (*t, t == &oscPort); t->onReturnKey = [this] { commitTop(); }; t->onFocusLost = [this] { commitTop(); }; addAndMakeVisible (*t); }
        ip.setTextToShowWhenEmpty ("multicast (sACN)", kDim);
        lbl (ipL, "IP"); lbl (oscL, "OSC host : porta");
        status.setFont (f (11.f)); status.setColour (juce::Label::textColourId, kDim); addAndMakeVisible (status);
        help.setText (juce::String::fromUTF8 ("Art-Net: IP del nodo/controller, 127.0.0.1 per QLC+ sullo stesso computer, broadcast es. 2.255.255.255. "
                                              "sACN: vuoto = multicast 239.255.x.y. Canali: R G B W C M Y, D dimmer, L luminosità zona, F lampo, S velocità strobo, numeri = valore fisso; "
                                              "tra [ ] si ripetono per ogni pixel. Le zone si trascinano nell'anteprima (angolo in basso a destra per ridimensionare)."), juce::dontSendNotification);
        help.setFont (f (10.5f)); help.setColour (juce::Label::textColourId, kDim); help.setJustificationType (juce::Justification::topLeft); help.setMinimumHorizontalScale (1.f);
        addAndMakeVisible (help);
        const char* adds[] = { "+ LED wall", "+ Barra pixel", "+ PAR / faro", "+ Strobo" };
        const char* types[] = { "wall", "bar", "par", "strobe" };
        for (int i = 0; i < 4; ++i) { addBtn[i].setButtonText (adds[i]); const juce::String t = types[i]; addBtn[i].onClick = [this, t] { addFixture (t); }; addAndMakeVisible (addBtn[i]); }
        importBtn.onClick = [this] { importPatch(); }; exportBtn.onClick = [this] { exportPatch(); };
        addAndMakeVisible (importBtn); addAndMakeVisible (exportBtn);
        zones.onSelect = [this] (int i) { select (i); };
        zones.onChanged = [this] { cfg.fixtures = zones.cfg.fixtures; apply (false); };
        addAndMakeVisible (zones);
        viewport.setViewedComponent (&list, false); viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
        load();
        startTimerHz (6);
    }
    ~Panel() override { stopTimer(); }

    void load()
    {
        cfg = eng.config();
        on.setToggleState (cfg.on, juce::dontSendNotification); black.setToggleState (cfg.blackout, juce::dontSendNotification); osc.setToggleState (cfg.osc, juce::dontSendNotification);
        const int fpsIds[] = { 20, 30, 40, 44 }; int fi = 3; for (int i = 0; i < 4; ++i) if (fpsIds[i] == cfg.fps) fi = i + 1;
        fps.setSelectedId (fi == 3 && cfg.fps != 40 ? 3 : fi, juce::dontSendNotification);
        proto.setSelectedId (cfg.proto == "sacn" ? 2 : cfg.proto == "none" ? 3 : 1, juce::dontSendNotification);
        ip.setText (cfg.ip, juce::dontSendNotification); oscHost.setText (cfg.oscHost, juce::dontSendNotification); oscPort.setText (juce::String (cfg.oscPort), juce::dontSendNotification);
        rebuildRows();
    }
    void paint (juce::Graphics& g) override { g.fillAll (kBg); }
    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        auto top = r.removeFromTop (28);
        on.setBounds (top.removeFromLeft (120)); fps.setBounds (top.removeFromLeft (90).reduced (0, 2)); top.removeFromLeft (8);
        proto.setBounds (top.removeFromLeft (130).reduced (0, 2)); top.removeFromLeft (6);
        ipL.setBounds (top.removeFromLeft (20)); ip.setBounds (top.removeFromLeft (150).reduced (0, 2)); top.removeFromLeft (12);
        black.setBounds (top.removeFromRight (100));
        osc.setBounds (top.removeFromLeft (60)); oscHost.setBounds (top.removeFromLeft (110).reduced (0, 2)); top.removeFromLeft (4); oscPort.setBounds (top.removeFromLeft (60).reduced (0, 2));
        r.removeFromTop (4);
        status.setBounds (r.removeFromTop (18));
        help.setBounds (r.removeFromTop (44));
        r.removeFromTop (4);
        zones.setBounds (r.removeFromTop (juce::jmin (300, r.getHeight() / 2)));
        r.removeFromTop (6);
        auto br = r.removeFromTop (28); const int bw = juce::jmin (130, br.getWidth() / 6);
        for (auto& b : addBtn) { b.setBounds (br.removeFromLeft (bw).reduced (2, 1)); }
        exportBtn.setBounds (br.removeFromRight (bw).reduced (2, 1)); importBtn.setBounds (br.removeFromRight (bw).reduced (2, 1));
        r.removeFromTop (6);
        viewport.setBounds (r);
        layoutRows();
    }

private:
    void timerCallback() override
    {
        const auto s = eng.stats();
        juce::String t = (s.on ? juce::String::fromUTF8 ("● Invio attivo") : juce::String::fromUTF8 ("○ Invio spento")) + " - "
                         + juce::String (s.fixtures) + " apparecchi - " + juce::String (s.channels) + " canali - " + juce::String (s.universes) + " universi - "
                         + juce::String (s.packetsPerSecond) + " pacchetti/s";
        if (s.on && s.packetsPerSecond == 0 && s.fixtures > 0) t << juce::String::fromUTF8 ("  ·  nessun invio: l'uscita deve essere visibile (plugin aperto o proiettore acceso)");
        status.setText (t, juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, s.on ? kOk : kDim);
        zones.repaint();
    }
    void commitTop()
    {
        cfg.on = on.getToggleState(); cfg.blackout = black.getToggleState(); cfg.osc = osc.getToggleState();
        const int fpsIds[] = { 20, 30, 40, 44 }; cfg.fps = fpsIds[juce::jlimit (0, 3, fps.getSelectedId() - 1)];
        cfg.proto = proto.getSelectedId() == 2 ? "sacn" : proto.getSelectedId() == 3 ? "none" : "artnet";
        cfg.ip = ip.getText().trim(); cfg.oscHost = oscHost.getText().trim().isEmpty() ? juce::String ("127.0.0.1") : oscHost.getText().trim();
        cfg.oscPort = juce::jlimit (1, 65535, oscPort.getText().getIntValue());
        apply (false);
    }
    void apply (bool rebuild)
    {
        eng.setConfig (cfg);
        cfg = eng.config();
        if (rebuild) rebuildRows(); else updateRows();
    }
    void select (int i)
    {
        selected = i; zones.selected = i; zones.repaint(); updateRows();
        if (i >= 0 && i < (int) rows.size()) viewport.setViewPosition (0, rows[(size_t) i]->getY());
    }
    void addFixture (const juce::String& type)
    {
        auto fx = LightsEngine::makeFixture (type);
        int u = 1, a = 1;
        const auto inf = eng.info();
        for (auto& i : inf) if (i.count && (i.endUniverse > u || (i.endUniverse == u && i.endAddress + 1 > a))) { u = i.endUniverse; a = i.endAddress + 1; }
        if (a > 512) { ++u; a = 1; }
        fx.universe = u; fx.address = a;
        int n = 1; for (auto& o : cfg.fixtures) if (o.type == type) ++n;
        fx.name << " " << n;
        cfg.fixtures.push_back (fx); selected = (int) cfg.fixtures.size() - 1;
        apply (true);
    }
    void rebuildRows()
    {
        rows.clear();
        for (size_t i = 0; i < cfg.fixtures.size(); ++i)
        {
            auto row = std::make_unique<FixtureRow>();
            const int idx = (int) i;
            row->onEdit = [this, idx] (const LightsEngine::Fixture& fx) { if (idx < (int) cfg.fixtures.size()) { cfg.fixtures[(size_t) idx] = fx; apply (false); } };
            row->onDelete = [this, idx] { juce::MessageManager::callAsync ([this, idx] { if (idx < (int) cfg.fixtures.size()) { cfg.fixtures.erase (cfg.fixtures.begin() + idx); selected = -1; apply (true); } }); };
            row->onDuplicate = [this, idx] { juce::MessageManager::callAsync ([this, idx] {
                if (idx >= (int) cfg.fixtures.size()) return;
                auto c = cfg.fixtures[(size_t) idx]; c.id = {}; c.name << " copia";
                const auto inf = eng.info(); if (idx < (int) inf.size() && inf[(size_t) idx].count) { c.universe = inf[(size_t) idx].endAddress >= 512 ? inf[(size_t) idx].endUniverse + 1 : inf[(size_t) idx].endUniverse; c.address = inf[(size_t) idx].endAddress >= 512 ? 1 : inf[(size_t) idx].endAddress + 1; }
                cfg.fixtures.insert (cfg.fixtures.begin() + idx + 1, LightsEngine::cleanFixture (c)); selected = idx + 1; apply (true); }); };
            row->onIdentify = [this, idx] { if (idx < (int) cfg.fixtures.size()) eng.identify (cfg.fixtures[(size_t) idx].id); };
            row->onSelect = [this, idx] { select (idx); };
            list.addAndMakeVisible (*row);
            rows.push_back (std::move (row));
        }
        layoutRows(); updateRows();
    }
    void updateRows()
    {
        const auto inf = eng.info();
        zones.cfg = cfg; zones.selected = selected; zones.repaint();
        for (size_t i = 0; i < rows.size() && i < cfg.fixtures.size(); ++i)
            rows[i]->set ((int) i, cfg.fixtures[i], i < inf.size() ? inf[i] : LightsEngine::Info(), (int) i == selected);
    }
    void layoutRows()
    {
        const int w = viewport.getWidth() - viewport.getScrollBarThickness(), rh = 190;
        list.setSize (juce::jmax (100, w), juce::jmax (1, (int) rows.size() * rh));
        for (size_t i = 0; i < rows.size(); ++i) rows[i]->setBounds (0, (int) i * rh, w, rh - 6);
    }
    void importPatch()
    {
        chooser = std::make_unique<juce::FileChooser> ("Importa patch luci (anche da MBVX)", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult(); if (! file.existsAsFile()) return;
            juce::String err;
            if (eng.loadJson (file.loadFileAsString(), err)) { selected = -1; load(); }
            else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Patch non valida", err);
        });
    }
    void exportPatch()
    {
        chooser = std::make_unique<juce::FileChooser> ("Salva patch luci", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("mbvx-luci.json"), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult(); if (file == juce::File()) return;
            file.withFileExtension ("json").replaceWithText (eng.toJson());
        });
    }

    LightsEngine& eng;
    LightsEngine::Config cfg;
    int selected = -1;
    juce::ToggleButton on, black, osc;
    juce::ComboBox fps, proto;
    juce::TextEditor ip, oscHost, oscPort;
    juce::Label ipL, oscL, status, help;
    juce::TextButton addBtn[4], importBtn { "Importa..." }, exportBtn { "Esporta..." };
    ZoneView zones;
    juce::Viewport viewport;
    juce::Component list;
    std::vector<std::unique_ptr<FixtureRow>> rows;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
LightsWindow::LightsWindow (LightsEngine& e)
    : juce::DocumentWindow ("MBStudio91 Visuals - Luci", kBg, juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
{
    panel = new Panel (e);
    setContentOwned (panel, false);
    setResizable (true, true);
    setResizeLimits (760, 560, 2400, 2000);
    setUsingNativeTitleBar (true);
    centreWithSize (980, 820);
}

LightsWindow::~LightsWindow() = default;

void LightsWindow::refreshFromEngine() { if (panel != nullptr) panel->load(); }
