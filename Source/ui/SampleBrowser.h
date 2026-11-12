#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Theme.h"

// Browser dei file: accesso a tutte le unita'/cartelle del PC, filtro sui formati audio,
// trascinamento verso le celle e doppio clic per caricare nella cella selezionata.
class SampleBrowser final : public juce::Component,
                            private juce::FileBrowserListener
{
public:
    SampleBrowser();
    ~SampleBrowser() override;

    juce::File getSelectedFile() const { return tree.getSelectedFile(); }
    std::function<void (const juce::File&)> onFileActivated;   // doppio clic / Invio
    std::function<void (const juce::File&)> onPreview;         // file audio selezionato (anteprima attiva)
    std::function<void()> onPreviewStop;                       // selezione non piu' valida / anteprima disattivata

    float getPreviewGainDb() const { return (float) previewVolume.getValue(); }
    void replayPreview() { selectionChanged(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectionChanged() override
    {
        const auto f = tree.getSelectedFile();
        if (autoPlay.getToggleState() && f.existsAsFile())
        {
            if (onPreview) onPreview (f);
        }
        else if (onPreviewStop)
            onPreviewStop();
    }
    void fileClicked (const juce::File&, const juce::MouseEvent&) override {}
    void fileDoubleClicked (const juce::File& f) override { if (onFileActivated && f.existsAsFile()) onFileActivated (f); }
    void browserRootChanged (const juce::File&) override {}

    void rebuildRoots();
    void setRoot (const juce::File&);

    juce::TimeSliceThread thread { "Sample browser" };
    juce::WildcardFileFilter filter { "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3", "*", "Audio files" };
    juce::DirectoryContentsList contents { &filter, thread };
    juce::FileTreeComponent tree { contents };

    juce::ToggleButton autoPlay { "Preview" };
    juce::Slider previewVolume;
    juce::ComboBox rootBox;
    std::vector<juce::File> roots;
};
