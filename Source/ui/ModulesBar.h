#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "Widgets.h"

// Fila di moduli della scheda Main: AMP (inviluppo, level, pan), PITCH (tune, pitch) e MASTER (volume + meter).
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
    struct Param
    {
        std::unique_ptr<KnobBox> box;
        int module = 0;
        std::function<float (const sl::CellParams&)> get;
        std::function<void (sl::CellParams&, float)> set;
    };

    enum Module { Amp = 0, Pitch, Master, numModules };

    void addKnob (int module, const juce::String& title, double min, double max, double step, double def, double skewMid,
                  std::function<float (const sl::CellParams&)> get,
                  std::function<void (sl::CellParams&, float)> set,
                  const juce::String& suffix = {});
    void edited();
    void updateEnvelope();

    SampleLabProcessor& processor;
    int cell = 0;
    bool updating = false;

    std::vector<Param> params;
    EnvelopeView envelope;
    KnobBox masterKnob { "Volume" };
    LevelMeter meter;

    juce::Rectangle<int> moduleBounds[numModules];
};
