#pragma once

#include "Theme.h"

// Manopola con etichetta e valore: usata dai moduli e dal pannello degli effetti.
class KnobBox final : public juce::Component
{
public:
    explicit KnobBox (const juce::String& title);
    void resized() override;

    juce::Label label;
    juce::Slider slider;
};
