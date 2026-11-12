#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "Widgets.h"

// Fila di moduli in basso: AMP (inviluppo, level, pan), PITCH (tune, pitch), MAPPING (note) e MASTER (volume + meter).
class ModulesBar final : public juce::Component
{
public:
    explicit ModulesBar (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();                    // rilegge i parametri dal kit

    std::function<void()> onEdited;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobBox final : public juce::Component
    {
        explicit KnobBox (const juce::String& title);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
        std::function<float (const sl::CellParams&)> get;
        std::function<void (sl::CellParams&, float)> set;
        int module = 0;
    };

    enum Module { Amp = 0, Pitch, Mapping, Master, numModules };

    KnobBox& addKnob (int module, const juce::String& title, double min, double max, double step, double def, double skewMid,
                      std::function<float (const sl::CellParams&)> get,
                      std::function<void (sl::CellParams&, float)> set,
                      const juce::String& suffix = {});
    void edited();
    void updateEnvelope();

    SampleLabProcessor& processor;
    int cell = 0;
    bool updating = false;

    std::vector<std::unique_ptr<KnobBox>> knobs;
    EnvelopeView envelope;
    KnobBox masterKnob { "Volume" };
    LevelMeter meter;

    juce::Rectangle<int> moduleBounds[numModules];
};
