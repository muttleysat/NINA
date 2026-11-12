#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

// Pannello destro: titolo della cella con frecce, campione assegnato, modalita' di trigger, reverse e informazioni.
class CellPanel final : public juce::Component
{
public:
    explicit CellPanel (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();

    std::function<void (int delta)> onStep;          // cella precedente/successiva
    std::function<void (int cell)> onLoadRequested;
    std::function<void()> onEdited;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sectionLabel (juce::Graphics&, juce::Rectangle<int>, const juce::String&, float s);

    SampleLabProcessor& processor;
    int cell = 0;
    bool updating = false;

    juce::TextButton prevButton { "<" }, nextButton { ">" }, loadButton { "Load" };
    juce::TextButton modeButtons[3];
    juce::ToggleButton reverseButton { "Reverse" };
    std::unique_ptr<juce::ParameterAttachment> modeAttachment;            // modalita' e reverse sono parametri automatizzabili
    std::unique_ptr<juce::ButtonParameterAttachment> reverseAttachment;

    // Mapping MIDI della cella: nota originale del campione e intervallo di tasti che la fanno suonare
    juce::Slider rootSlider, lowSlider, highSlider;
    juce::Rectangle<int> mapLabel, rootRow, lowRow, highRow;

    juce::Rectangle<int> titleRect, sampleCard, modeLabel, reverseLabel, infoRect;
};
