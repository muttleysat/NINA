#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

// Scheda Seq: una riga (lane) per ogni cella aggiunta dall'utente, con step cliccabili, editor dello step
// (velocity, probabilita', ratchet, nudge, gate, pitch, parameter lock sugli effetti), trasporto ed esportazione.
class SeqPanel final : public juce::Component, private juce::Timer
{
public:
    explicit SeqPanel (SampleLabProcessor&);
    ~SeqPanel() override;

    void refresh();                    // ricostruisce le righe dal pattern attivo (cambio pattern/kit, lane aggiunte/rimosse)

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class StepButton;
    class LaneRow;
    class LaneList;

    void timerCallback() override;
    void stepClicked (int lane, int step, bool selectOnly);
    void stepVelocity (int lane, int step, int velocity);
    sl::Pattern& pat() { return processor.activePattern(); }
    const sl::Pattern& pat() const { return processor.activePattern(); }
    void edited();                     // pubblica il pattern al thread audio
    void selectStep (int lane, int step);
    void refreshEditor();
    void showAddMenu();
    void exportMidi();
    void bounceWav();
    juce::File defaultExportFile (const juce::String& extension) const;
    int bars() const { return 1 << juce::jmax (0, barsBox.getSelectedId() - 1); }   // 1, 2, 4, 8
    void applyLockSlider (bool fromUser);
    void updateLockControls();
    void showMessage (const juce::String& text);

    SampleLabProcessor& processor;

    // Trasporto e pattern
    juce::TextButton playButton { "Play" }, addButton { "+ Cell" }, midiButton { "Export MIDI" }, wavButton { "Bounce WAV" },
                     reseedButton { "Reseed" };
    juce::ComboBox patternBox, barsBox;
    juce::Slider swingSlider;
    juce::TextButton pageButtons[4];
    int page = 0;                      // pagina di 16 step visualizzata (0..3)

    // Righe
    std::unique_ptr<LaneList> lanes;
    juce::Viewport viewport;

    // Editor dello step selezionato
    int selLane = -1, selStep = -1;
    bool updatingEditor = false;
    juce::Slider velocitySlider, probSlider, nudgeSlider, gateSlider, pitchSlider, lockSlider;
    juce::ComboBox ratchetBox, lockParamBox, stepModeBox;
    juce::ToggleButton lockToggle { "Lock" }, reverseToggle { "Reverse this step" };
    juce::Rectangle<int> editorRect, titleRect;
    std::array<juce::Rectangle<int>, 5> labelRects;
    std::unique_ptr<juce::FileChooser> chooser;
};
