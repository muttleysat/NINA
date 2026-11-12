#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"

// Scheda Seq: una riga (lane) per ogni cella aggiunta, con step cliccabili, e sotto l'editor dello step selezionato:
// comportamento (modalita', reverse, velocity, probabilita', ratchet, nudge, gate, pitch) ed effetti per colpo
// (saturazione, LoFi, filtro, mandata al delay e al riverbero). Muovere una manopola di un effetto "blocca" quel valore
// su quello step (arancione); le manopole grigie seguono la cella. Doppio clic su una manopola: toglie il blocco.
class SeqPanel final : public juce::Component, private juce::Timer
{
public:
    explicit SeqPanel (SampleLabProcessor&);
    ~SeqPanel() override;

    void refresh();                    // ricostruisce le righe dal pattern attivo (cambio pattern/kit, lane aggiunte/rimosse)

    // Selezionando uno step (o il nome di una riga) si seleziona anche la sua cella: la scheda FX e il pannello a destra la seguono
    std::function<void (int cell)> onCellSelected;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class StepButton;
    class LaneRow;
    class LaneList;
    struct FxKnob;
    struct FxCard;

    void timerCallback() override;
    void stepClicked (int lane, int step, bool selectOnly);
    void stepVelocity (int lane, int step, int velocity);
    sl::Pattern& pat() { return processor.activePattern(); }
    const sl::Pattern& pat() const { return processor.activePattern(); }
    void edited();                     // pubblica il pattern al thread audio
    void selectStep (int lane, int step);
    void refreshEditor();              // rilegge lo step selezionato nei controlli
    sl::Step* selectedStep();
    int selectedCell() const;
    bool effectOn (const sl::Step&, sl::FxB, int cell) const;
    float cellValue (int cell, sl::FxF f) const;
    void onFxKnob (FxCard&, size_t knob);
    void onFxPower (FxCard&);
    void clearKnobLock (FxCard&, size_t knob);
    void showAddMenu();
    void exportMidi();
    void bounceWav();
    juce::File defaultExportFile (const juce::String& extension) const;
    int bars() const { return 1 << juce::jmax (0, barsBox.getSelectedId() - 1); }   // 1, 2, 4, 8
    void showMessage (const juce::String& text);

    SampleLabProcessor& processor;

    // Trasporto e pattern
    juce::TextButton playButton { "Play" }, addButton { "+ Cell" }, midiButton { "Export MIDI" }, wavButton { "Bounce WAV" },
                     reseedButton { "Reset seed" }, prevPattern { "<" }, nextPattern { ">" };
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
    int tick = 0;
    KnobBox velocityKnob { "Velocity" }, probKnob { "Probability" }, nudgeKnob { "Nudge" }, gateKnob { "Gate" },
            pitchKnob { "Pitch" }, ratchetKnob { "Ratchet" };
    juce::ComboBox stepModeBox;
    juce::ToggleButton reverseToggle { "Reverse" };
    juce::TextButton clearLocksButton { "Clear locks" };
    std::vector<std::unique_ptr<FxCard>> fxCards;

    juce::Rectangle<int> editorRect, headerRect, rulerRect, fxLabelRect, stepsArea;
    std::unique_ptr<juce::FileChooser> chooser;
};
