#pragma once

#include "Theme.h"

// Finestra "Info": logo dell'app, versione, copyright e logo ASIO (driver audio dello standalone).
class AboutPanel final : public juce::Component
{
public:
    explicit AboutPanel (float scale);

    void paint (juce::Graphics&) override;
    void resized() override;

    // Apre la finestra (non bloccante) al centro dello schermo
    static void show (float scale);

private:
    juce::TextButton closeButton { "Close" };
    juce::Image logo, asio;
    juce::Rectangle<int> logoRect, versionRect, copyrightRect, asioRect, asioTextRect;
    float s = 1.0f;
};
