#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "WaveformView.h"

// Striscia sotto la griglia: ID/nome della cella, forma d'onda con Start/End e letture in ms.
class WaveStrip final : public juce::Component
{
public:
    explicit WaveStrip (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();

    std::function<void()> onEdited;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void edited();
    double lengthMs() const;

    SampleLabProcessor& processor;
    int cell = 0;
    WaveformView waveform;
    juce::Rectangle<int> idBox, readout;
};
