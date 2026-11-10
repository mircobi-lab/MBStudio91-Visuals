#include "ProjectorWindow.h"

ProjectorWindow::ProjectorWindow (StageComponent& s, int displayIndex, std::function<void()> onClose)
    : juce::TopLevelWindow ("MBStudio91 Visuals - Proiettore", true), stage (s), display (displayIndex), onCloseFn (std::move (onClose))
{
    setOpaque (true);
    setUsingNativeTitleBar (false);
    setDropShadowEnabled (false);
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    auto* d = (display >= 0 && display < displays.displays.size()) ? &displays.displays.getReference (display) : displays.getPrimaryDisplay();
    const auto area = d != nullptr ? d->logicalBounds.toNearestInt() : juce::Rectangle<int> (0, 0, 1280, 720);
    setBounds (area);
    addAndMakeVisible (stage);
    stage.setBounds (getLocalBounds());
    setWantsKeyboardFocus (true);
    setAlwaysOnTop (true);
    setVisible (true);
    toFront (true);
    juce::Desktop::getInstance().setKioskModeComponent (this, false);   // nasconde barra dei menu e Dock
    grabKeyboardFocus();
}

ProjectorWindow::~ProjectorWindow()
{
    if (juce::Desktop::getInstance().getKioskModeComponent() == this) juce::Desktop::getInstance().setKioskModeComponent (nullptr);
    removeChildComponent (&stage);
}

void ProjectorWindow::resized()
{
    stage.setBounds (getLocalBounds());
}

bool ProjectorWindow::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey || k.getTextCharacter() == 'f' || k.getTextCharacter() == 'F') { requestClose(); return true; }
    return false;
}
