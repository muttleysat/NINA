#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "WaveformView.h"

// Pannello di editing della cella selezionata: forma d'onda con Start/End, inviluppo, volume, pan,
// tune/pitch, modalita' di trigger, reverse e mapping delle note.
class CellEditor final : public juce::Component
{
public:
    explicit CellEditor (SampleLabProcessor&);

    void setCell (int cell);
    void refresh();                    // rilegge i parametri dal kit (es. dopo il caricamento di un sample)

    std::function<void()> onEdited;    // chiamato dopo una modifica (per ridisegnare la griglia)

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobBox final : public juce::Component
    {
        KnobBox (const juce::String& title);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
        std::function<float (const sl::CellParams&)> get;
        std::function<void (sl::CellParams&, float)> set;
    };

    KnobBox& addKnob (const juce::String& title, double min, double max, double step, double def, double skewMid,
                      std::function<float (const sl::CellParams&)> get,
                      std::function<void (sl::CellParams&, float)> set,
                      const juce::String& suffix = {});
    void edited();

    SampleLabProcessor& processor;
    int cell = 0;
    bool updating = false;

    juce::Label titleLabel;
    juce::ComboBox modeBox;
    juce::ToggleButton reverseButton { "Reverse" };
    WaveformView waveform;
    std::vector<std::unique_ptr<KnobBox>> knobs;

    juce::Rectangle<int> strip;
};
