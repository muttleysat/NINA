#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "Widgets.h"

// Scheda FX: catena di effetti (Saturation > LoFi > Filter > Compressor > Delay > Reverb) della cella
// selezionata oppure globale del kit (Master).
class FxPanel final : public juce::Component
{
public:
    explicit FxPanel (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();

    std::function<void()> onEdited;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Getter = std::function<float (const sl::FxParams&)>;
    using Setter = std::function<void (sl::FxParams&, float)>;

    struct Knob
    {
        std::unique_ptr<KnobBox> box;
        Getter get;
        Setter set;
    };

    struct Card
    {
        juce::String title;
        float units = 3.0f;
        std::unique_ptr<juce::ToggleButton> power;
        std::function<bool (const sl::FxParams&)> getOn;
        std::function<void (sl::FxParams&, bool)> setOn;
        std::unique_ptr<juce::ComboBox> combo;                // solo per il filtro (tipo)
        std::vector<Knob> knobs;
        juce::Rectangle<int> bounds;
    };

    Card& addCard (const juce::String& title, float units,
                   std::function<bool (const sl::FxParams&)> getOn, std::function<void (sl::FxParams&, bool)> setOn);
    void addKnob (Card&, const juce::String& title, double min, double max, double step, double def, double skewMid,
                  Getter, Setter, const juce::String& suffix = {});

    sl::FxParams& target();
    void edited();
    void updateHeader();

    SampleLabProcessor& processor;
    int cell = 0;
    bool masterTarget = false;
    bool updating = false;

    juce::TextButton cellButton, masterButton { "Master" };
    std::vector<std::unique_ptr<Card>> cards;
    juce::Rectangle<int> header;
};
