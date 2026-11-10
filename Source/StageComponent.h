#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/*  Il "palco": il componente su cui OpenGL disegna l'uscita. Vive nel processore e viene spostato
    tra l'editor e la finestra del proiettore. Doppio clic = proiettore a schermo intero / torna. */
class StageComponent : public juce::Component
{
public:
    StageComponent() { setOpaque (true); setInterceptsMouseClicks (true, false); }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }
    std::function<void()> onDoubleClick;
};
