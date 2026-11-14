#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "WaveformView.h"

// Striscia sotto la griglia: ID/nome della cella, forma d'onda con Start/End e letture in ms.
// Start ed End sono parametri automatizzabili: il trascinamento dei marker apre e chiude un gesto verso la DAW.
class WaveStrip final : public juce::Component, private juce::Timer
{
public:
    explicit WaveStrip (NinaProcessor&);

    void setCell (int cell);
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    double lengthMs() const;
    juce::AudioParameterFloat* startParam() const;
    juce::AudioParameterFloat* endParam() const;

    NinaProcessor& processor;
    int cell = 0;
    float shownStart = -1.0f, shownEnd = -1.0f;
    WaveformView waveform;
    juce::Rectangle<int> idBox, readout;
};
