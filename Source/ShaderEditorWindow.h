#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

/*  Editor dello shader di un deck: codice GLSL (solo mainImage e funzioni di aiuto; l'intestazione con
    uniform e helper è fissa), Compila (Ctrl/Cmd+Invio), errori, Salva come preset utente, Ripristina preset. */
class ShaderEditorWindow : public juce::DocumentWindow, private juce::Timer
{
public:
    explicit ShaderEditorWindow (MBVisualsProcessor&);
    ~ShaderEditorWindow() override;
    void showDeck (int deck);
    void closeButtonPressed() override { setVisible (false); }

private:
    class Content;
    void timerCallback() override;
    MBVisualsProcessor& proc;
    Content* content = nullptr;
};
