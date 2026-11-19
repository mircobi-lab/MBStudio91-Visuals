#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "LightsEngine.h"

/*  Finestra "Luci": impostazioni di rete (Art-Net / sACN / OSC), anteprima dell'uscita con le zone degli
    apparecchi (trascina per spostare, angolo in basso a destra per ridimensionare) ed elenco degli apparecchi. */
class LightsWindow : public juce::DocumentWindow
{
public:
    explicit LightsWindow (LightsEngine&);
    ~LightsWindow() override;
    void closeButtonPressed() override { setVisible (false); }
    void refreshFromEngine();          // dopo un caricamento di stato/patch

private:
    class Panel;
    Panel* panel = nullptr;
};
