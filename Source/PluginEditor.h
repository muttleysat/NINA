#pragma once

#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/PadGrid.h"
#include "ui/WaveStrip.h"
#include "ui/ModulesBar.h"
#include "ui/CellPanel.h"
#include "ui/FxPanel.h"
#include "ui/SampleBrowser.h"

class SampleLabEditor final : public juce::AudioProcessorEditor,
                              public juce::DragAndDropContainer,
                              private juce::Timer
{
public:
    explicit SampleLabEditor (SampleLabProcessor&);
    ~SampleLabEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void showTab (int tab);
    juce::Rectangle<int> tabRect (int index) const;
    void showSizeMenu();
    void applyScale (float zoom);

    // Kit
    void showKitMenu();
    void refreshTopBar();
    void refreshAll();
    void refreshCellViews();
    void confirmDiscard (std::function<void()> proceed);
    void doNew();
    void doOpen();
    void doOpenFile (const juce::File& f);
    void doOpenAdjacent (int direction);
    void doSave();
    void doSaveAs();
    void showError (const juce::String& message);

    // Celle
    void loadIntoCell (int cell, const juce::File& file);
    void requestLoadDialog (int cell);

    SampleLabLookAndFeel lnf;
    SampleLabProcessor& processor;

    juce::TextButton kitButton, prevKitButton { "<" }, nextKitButton { ">" }, saveButton { "Save" }, sizeButton { "Size 100%" };
    SampleBrowser browser;
    PadGrid grid;
    WaveStrip waveStrip;
    CellPanel cellPanel;
    ModulesBar modules;
    FxPanel fxPanel;
    int currentTab = 0;                 // 0 = Main, 3 = FX (le altre schede arriveranno con le fasi successive)
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> topBar, tabBar;

    static constexpr int baseWidth = 1280;
    static constexpr int baseHeight = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabEditor)
};
