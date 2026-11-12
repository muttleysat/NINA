#pragma once

#include "Theme.h"

namespace widgets
{
// Cornice di un modulo (pannello con titolo), come AMP / PITCH / MASTER del riferimento grafico.
void drawModule (juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title, float scale);
}

// Meter stereo verticale con ritorno graduale.
class LevelMeter final : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (std::function<std::pair<float, float>()> getLevels);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::function<std::pair<float, float>()> levels;
    float l = 0.0f, r = 0.0f;
};

// Disegno della forma dell'inviluppo AHDSR.
class EnvelopeView final : public juce::Component
{
public:
    void setParams (float attack, float hold, float decay, float sustain, float release);
    void paint (juce::Graphics&) override;

private:
    float a = 0.0005f, h = 0.0f, d = 0.1f, s = 1.0f, r = 0.02f;
};
