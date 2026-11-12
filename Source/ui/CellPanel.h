#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "LearnMenu.h"

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
    juce::TextButton normalizeButton { "Normalize" };
    std::unique_ptr<juce::ParameterAttachment> modeAttachment;            // modalita' e reverse sono parametri automatizzabili
    std::unique_ptr<juce::ButtonParameterAttachment> reverseAttachment;
    std::vector<std::unique_ptr<LearnMenu>> learnMenus;                   // tasto destro: MIDI learn

    // Mapping MIDI della cella: nota originale del campione e intervallo di tasti che la fanno suonare
    juce::Slider rootSlider, lowSlider, highSlider;
    juce::Rectangle<int> mapLabel, rootRow, lowRow, highRow;

    // Loop: sincronizzazione al BPM del progetto (Rubber Band), Key Lock, tempo originale del loop
    juce::ToggleButton syncButton { "Sync to tempo" }, keyLockButton { "Key lock" };
    juce::Slider bpmSlider;
    juce::TextButton detectButton { "Detect" };
    juce::Rectangle<int> loopLabel, loopToggleRow, bpmRow;

    juce::Rectangle<int> titleRect, sampleCard, modeLabel, reverseLabel, infoRect;
};
