#pragma once

#include <optional>

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "IconButton.h"
#include "DragHandle.h"

// Scheda Seq: una riga (lane) per ogni cella aggiunta, con step cliccabili, e sotto l'editor dello step selezionato:
// comportamento (modalita', reverse, velocity, probabilita', ratchet, nudge, gate, pitch) ed effetti per colpo
// (saturazione, LoFi, filtro, mandata al delay e al riverbero). Muovere una manopola di un effetto "blocca" quel valore
// su quello step (arancione); le manopole grigie seguono la cella. Doppio clic su una manopola: toglie il blocco.
class SeqPanel final : public juce::Component, private juce::Timer
{
public:
    explicit SeqPanel (NinaProcessor&);
    ~SeqPanel() override;

    void copySelection();              // Ctrl+C: copia lo step selezionato (con velocity, nudge, lock...)
    void pasteSelection();             // Ctrl+V: lo incolla sullo step selezionato

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
    struct CellKnob;
    struct FxCard;

    void timerCallback() override;
    void stepClicked (int lane, int step, bool selectOnly);
    void stepVelocity (int lane, int step, int velocity);
    void paintStepAt (const juce::MouseEvent&, bool on);   // pennello: accende/spegne lo step sotto il mouse (anche su altre righe)
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
    void addAllLoadedCells();
    void showLaneMenu (int lane);      // tasto destro sull'intestazione di una riga: copia / incolla / svuota
    void showQuantizeMenu();           // quantizza il pattern o la riga, intensita' della registrazione
    void quantizePattern (bool onlySelectedLane, float strength);
    void captureRecorded();            // scrive negli step le note registrate dal thread audio
    void saveSequences();               // tutti i pattern in un file .slseq
    void showPatternMenu();            // duplica / copia / incolla / svuota / rinomina il pattern
    void relayoutLanes();              // ricalcola colonne e larghezze (cambio lunghezza righe, ridimensionamento)
    void exportMidi();
    void bounceWav();
    void exportStems();
    juce::File dragFolder() const;                       // cartella temporanea dei file trascinati verso la DAW
    juce::String dragBaseName() const;
    juce::File defaultExportFile (const juce::String& extension) const;
    int bars() const { return 1 << juce::jmax (0, barsBox.getSelectedId() - 1); }   // 1, 2, 4, 8
    void showMessage (const juce::String& text);

    NinaProcessor& processor;

    // Trasporto e pattern
    juce::TextButton playButton { "Play" }, prevPattern { "<" }, nextPattern { ">" };
    IconButton addButton { "+ Cell" }, addAllButton { {}, IconButton::Icon::Grid }, patternMenuButton { {}, IconButton::Icon::More },
               midiButton { "Export MIDI", IconButton::Icon::Midi }, wavButton { "Bounce WAV", IconButton::Icon::Wave },
               stemsButton { "Export Lanes", IconButton::Icon::Layers }, reseedButton { "Reset Seed", IconButton::Icon::Reset };
    juce::ComboBox patternBox, barsBox, tailBox, rateBox, hostModeBox;
    juce::TextButton recButton { "Rec" }, clickButton { "Click" };
    juce::Slider clickVolume;
    IconButton quantButton { "Rec options", IconButton::Icon::Grid };
    IconButton saveSeqButton { "Save seq", IconButton::Icon::Layers };
    std::optional<sl::Step> stepClip;
    std::optional<sl::Lane> laneClip;
    int recSnap = 100;                 // % di quantizzazione delle note registrate (100 = sulla griglia, 0 = micro-timing libero)
    juce::Rectangle<int> recRect;
    juce::Slider swingSlider;
    int lastViewX = 0;                 // scroll orizzontale corrente (l'intestazione delle righe resta ferma)

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

    // Maniglie "Drag to DAW": trascinandole fuori dalla finestra si lascia il file su una traccia della DAW
    std::unique_ptr<DragHandle> dragMidi, dragAudio, dragLanes;
    juce::Rectangle<int> dragRow;

    juce::Rectangle<int> swingRect;                 // manopola dello swing (etichetta in paint)
    std::vector<std::pair<int, int>> separatorsX;   // (x, riga) dei separatori verticali: riga 0 = prima, 1 = seconda, 2 = entrambe
    juce::Rectangle<int> row1Rect, row2Rect;
    juce::Rectangle<int> editorRect, headerRect, rulerRect, fxLabelRect, stepsArea;
    std::unique_ptr<juce::FileChooser> chooser;
};
