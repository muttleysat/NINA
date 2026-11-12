#pragma once

#include "Theme.h"

// Manopola con etichetta e valore: usata dai moduli e dal pannello degli effetti.
class KnobBox final : public juce::Component
{
public:
    explicit KnobBox (const juce::String& title);
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

    // Stato del MIDI learn per l'indicatore: CC assegnato (-1 se nessuno) e apprendimento in corso
    std::function<int()> ccQuery;
    std::function<bool()> learningQuery;
    void refreshIndicator();            // ridisegna solo se lo stato e' cambiato

    juce::Label label;
    juce::Slider slider;

private:
    int shownCc = -2;
    bool shownLearning = false;
};
