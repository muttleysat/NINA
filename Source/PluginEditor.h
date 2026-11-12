#pragma once

#include "PluginProcessor.h"
#include "ui/PadGrid.h"
#include "ui/CellEditor.h"
#include "ui/SampleBrowser.h"

class SampleLabEditor final : public juce::AudioProcessorEditor,
                              public juce::DragAndDropContainer
{
public:
    explicit SampleLabEditor (SampleLabProcessor&);
    ~SampleLabEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showSizeMenu();
    void applyScale (float zoom);

    // Kit
    void showKitMenu();
    void refreshTopBar();
    void refreshAll();
    void confirmDiscard (std::function<void()> proceed);
    void doNew();
    void doOpen();
    void doOpenFile (const juce::File& f);
    void doSave();
    void doSaveAs();
    void showError (const juce::String& message);

    // Celle
    void loadIntoCell (int cell, const juce::File& file);
    void requestLoadDialog (int cell);

    SampleLabProcessor& processor;

    juce::TextButton kitButton, sizeButton { "Size 100%" };
    SampleBrowser browser;
    PadGrid grid;
    CellEditor editor;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> topBar;

    static constexpr int baseWidth = 1280;
    static constexpr int baseHeight = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabEditor)
};
