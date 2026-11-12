#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "Widgets.h"

// Scheda FX: catena di effetti (Saturation > LoFi > Filter > Compressor > Delay > Reverb) della cella
// selezionata oppure globale del kit (Master). Tutti i controlli sono parametri automatizzabili.
class FxPanel final : public juce::Component, private juce::Timer
{
public:
    explicit FxPanel (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<KnobBox> box;
        sl::FxF which;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    struct Card
    {
        juce::String title;
        float units = 3.0f;
        sl::FxB effect = sl::FxB::Sat;
        std::unique_ptr<juce::ToggleButton> power;
        std::unique_ptr<juce::ButtonParameterAttachment> powerAttachment;
        std::unique_ptr<juce::ComboBox> combo;                 // solo per il filtro (tipo)
        std::unique_ptr<juce::ComboBoxParameterAttachment> comboAttachment;
        // Solo per il delay: interruttore Sync e valore musicale (1/8, 1/16T...)
        std::unique_ptr<juce::TextButton> syncButton;
        std::unique_ptr<juce::ButtonParameterAttachment> syncAttachment;
        std::unique_ptr<juce::ComboBox> divisionCombo;
        std::unique_ptr<juce::ComboBoxParameterAttachment> divisionAttachment;
        std::vector<Knob> knobs;
        juce::Rectangle<int> bounds;
    };

    Card& addCard (const juce::String& title, float units, sl::FxB effect);
    void addKnob (Card&, const juce::String& title, sl::FxF which);
    const sl::FxRefs& refs() const;
    void updateHeader();
    void updateSyncState();
    void timerCallback() override;      // ridisegna il bordo degli effetti accesi/spenti dall'automazione

    SampleLabProcessor& processor;
    int cell = 0;
    bool masterTarget = false;
    unsigned lastPowerMask = 0;

    juce::TextButton cellButton, masterButton { "Master" };
    std::vector<std::unique_ptr<Card>> cards;
    juce::Rectangle<int> header;
};
