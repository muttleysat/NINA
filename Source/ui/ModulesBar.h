#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "LearnMenu.h"
#include "Widgets.h"

// Fila di moduli della scheda Main: AMP (inviluppo, level, pan), PITCH (tune, pitch) e MASTER (volume + meter).
// Le manopole sono collegate ai parametri della DAW: seguono l'automazione e ne registrano i gesti.
class ModulesBar final : public juce::Component, private juce::Timer
{
public:
    explicit ModulesBar (NinaProcessor&);

    void setCell (int cell);
    void refresh();                    // ricollega le manopole ai parametri della cella selezionata

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Param
    {
        std::unique_ptr<KnobBox> box;
        int module = 0;
        sl::CellF which = sl::CellF::Level;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    enum Module { Amp = 0, Pitch, Master, numModules };

    void addKnob (int module, const juce::String& title, sl::CellF which);
    void timerCallback() override;
    void updateEnvelope();

    NinaProcessor& processor;
    int cell = 0;

    std::vector<Param> params;
    std::vector<std::unique_ptr<LearnMenu>> learnMenus;      // tasto destro: MIDI learn
    EnvelopeView envelope;
    KnobBox masterKnob { "Volume" };
    std::unique_ptr<juce::SliderParameterAttachment> masterAttachment;
    LevelMeter meter;

    float lastEnv[5] = { -1, -1, -1, -1, -1 };
    juce::Rectangle<int> moduleBounds[numModules];
};
