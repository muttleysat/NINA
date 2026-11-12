#pragma once

#include "PluginProcessor.h"

// Interfaccia di scheletro: mostra le aree principali (browser, griglia 4x12, pannello inferiore)
// con layout proporzionale, per verificare ridimensionamento e adattamento (UI-12, UI-13).
class SampleLabEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SampleLabEditor (SampleLabProcessor&);
    ~SampleLabEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showSizeMenu();
    void applyScale (float zoom);

    SampleLabProcessor& processor;
    juce::TextButton sizeButton { "Size 100%" };

    juce::Rectangle<int> topBar, browser, grid, bottomPanel;

    static constexpr int baseWidth = 1280;
    static constexpr int baseHeight = 760;
    static constexpr int rows = 4, cols = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabEditor)
};
