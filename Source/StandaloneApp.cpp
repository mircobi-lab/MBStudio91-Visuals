/*  Applicazione Standalone personalizzata.
    Differenze rispetto a quella predefinita di JUCE:
    - l'ingresso audio NON è silenziato al primo avvio (il plugin ascolta BlackHole: niente loop di feedback,
      perché l'audio in uscita va alle casse e non torna in BlackHole);
    - all'avvio prova a scegliere BlackHole come ingresso predefinito, se presente;
    - MIDI: apre automaticamente tutti i dispositivi MIDI collegati (per i controller);
    - argomenti da riga di comando per i test (--preset-a N, --preset-b N, --palette N, --pixel N, --xf V, --shot FILE). */

#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

class MBVStandaloneApp final : public juce::JUCEApplication
{
public:
    MBVStandaloneApp()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = juce::CharPointer_UTF8 (JucePlugin_Name);
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        o.folderName = "~/.config";
       #else
        o.folderName = "";
       #endif
        appProperties.setStorageParameters (o);
    }

    const juce::String getApplicationName() override     { return juce::CharPointer_UTF8 (JucePlugin_Name); }
    const juce::String getApplicationVersion() override  { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override           { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void initialise (const juce::String& commandLine) override
    {
        auto* settings = appProperties.getUserSettings();
        const bool firstRun = ! settings->containsKey ("mbvFirstRunDone");
        if (firstRun)
        {
            settings->setValue ("shouldMuteInput", false);      // il plugin deve sentire l'ingresso
            settings->setValue ("mbvFirstRunDone", true);
        }
        // Ingresso preferito: BlackHole (o VB-CABLE su Windows) se c'è ed è il primo avvio
        juce::String preferredDevice;
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        const juce::AudioDeviceManager::AudioDeviceSetup* setupPtr = nullptr;
        if (firstRun)
        {
            juce::AudioDeviceManager probe;
            probe.createAudioDeviceTypes (probeTypes);
            for (auto* t : probeTypes)
            {
                t->scanForDevices();
                for (auto& n : t->getDeviceNames (true))
                    if (n.containsIgnoreCase ("BlackHole") || n.containsIgnoreCase ("CABLE Output") || n.containsIgnoreCase ("Loopback"))
                    { setup.inputDeviceName = n; setup.useDefaultInputChannels = true; setupPtr = &setup; break; }
                if (setupPtr != nullptr) break;
            }
        }

        mainWindow = std::make_unique<juce::StandaloneFilterWindow> (getApplicationName(),
                                                                     juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                                                                     settings, false, preferredDevice, setupPtr,
                                                                     juce::Array<juce::StandalonePluginHolder::PluginInOuts>(), true);
        mainWindow->setVisible (true);
        applyCommandLine (commandLine);
    }

    void applyCommandLine (const juce::String& cmd)
    {
        auto args = juce::StringArray::fromTokens (cmd, true);
        auto* proc = dynamic_cast<MBVisualsProcessor*> (mainWindow->getAudioProcessor());
        if (proc == nullptr) return;
        auto set = [proc] (const juce::String& id, float v) { if (auto* p = proc->apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        for (int i = 0; i + 1 < args.size(); ++i)
        {
            const auto a = args[i], v = args[i + 1].unquoted();
            if (a == "--preset-a") set (mbv::ids::deck (0, mbv::ids::preset), v.getFloatValue());
            else if (a == "--preset-b") set (mbv::ids::deck (1, mbv::ids::preset), v.getFloatValue());
            else if (a == "--palette") set (mbv::ids::palette, v.getFloatValue());
            else if (a == "--pixel") set (mbv::ids::pixel, v.getFloatValue());
            else if (a == "--xf") set (mbv::ids::xf, v.getFloatValue());
            else if (a == "--mode") set (mbv::ids::mode, v.getFloatValue());
            else if (a == "--feedback") set (mbv::ids::feedback, v.getFloatValue());
            else if (a == "--test-tone") proc->testTone = true;
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr) mainWindow->pluginHolder->savePluginState();
        if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            juce::Timer::callAfterDelay (100, [] { if (auto app = JUCEApplicationBase::getInstance()) app->systemRequestedQuit(); });
        else
            quit();
    }

private:
    juce::ApplicationProperties appProperties;
    juce::OwnedArray<juce::AudioIODeviceType> probeTypes;
    std::unique_ptr<juce::StandaloneFilterWindow> mainWindow;
};

JUCE_CREATE_APPLICATION_DEFINE (MBVStandaloneApp)
