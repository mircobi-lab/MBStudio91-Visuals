#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "StageComponent.h"

/*  Finestra senza bordi a schermo intero sul display scelto (il proiettore / secondo schermo).
    Esc o doppio clic la chiude. Ospita il palco; quando si chiude, il palco torna all'editor. */
class ProjectorWindow : public juce::TopLevelWindow
{
public:
    ProjectorWindow (StageComponent& stage, int displayIndex, std::function<void()> onClose);
    ~ProjectorWindow() override;

    bool keyPressed (const juce::KeyPress& k) override;
    void resized() override;
    void requestClose()                      { if (onCloseFn) { auto f = onCloseFn; onCloseFn = nullptr; f(); } }
    int displayIndex() const                 { return display; }

private:
    StageComponent& stage;
    int display;
    std::function<void()> onCloseFn;
};
