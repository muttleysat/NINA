#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "IconButton.h"

// Scheda Slice: tagliare il campione di una cella in slice (rilevamento dei colpi, divisioni uguali, marcatori modificabili a
// mano) e distribuirle sulle celle successive (stesso file, ognuna con il proprio Start/End: nessuna copia dei dati).
class SlicePanel final : public juce::Component, private juce::Timer
{
public:
    explicit SlicePanel (SampleLabProcessor&);
    ~SlicePanel() override;

    void setCell (int cell);               // cella (e quindi campione) su cui si lavora
    void refresh();                        // il kit o la regione Start/End sono cambiati
    void runDetect() { doDetect(); }       // come premere Detect (anche per le schermate automatiche)

    // Chop: il loop suona e ogni pressione di M (o di una nota MIDI) mette un marcatore nel punto in cui si trova
    bool isChopping() const { return chopping; }
    void chopMark();                       // tasto M

    std::function<void()> onKitChanged;    // dopo la distribuzione: l'editor ridisegna griglia e pannelli

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (! isShowing()) stopChop(); }

private:
    class WaveView;
    class CellsStrip;
    friend class WaveView;
    friend class CellsStrip;

    // Dati di lavoro
    float regionStart() const;
    float regionEnd() const;
    sl::SampleDataPtr data() const { return processor.getCellData (cell); }
    std::vector<float> bounds() const;                         // marcatori + fine regione (n+1 limiti)
    std::vector<int> targetCells() const;                      // celle che verrebbero riempite
    int firstTarget() const;
    void cellsStripRepaint();
    void markersChanged();                                     // salva nel kit e ridisegna
    void auditionSlice (int index);
    void doDetect();
    void doEven (int count);
    void doClear();
    void doDistribute();
    void startChop();
    void stopChop (bool finished = false);
    void addChopMarker (double timeMs);
    double chopPosition (double nowMs) const;                  // posizione (0..1 sul campione) raggiunta dall'anteprima
    void setTargetStart (int cellIndex);
    void showStatus (const juce::String& text);
    void timerCallback() override;

    SampleLabProcessor& processor;
    int cell = 0;
    std::vector<float> markers;                                // inizio di ogni slice, ordinati (0..1 sul campione)
    int targetStart = -1;                                      // -1 = automatico (cella dopo l'originale)
    bool keepOriginal = true;
    juce::String status;
    double statusUntil = 0.0;
    bool chopping = false;
    double chopStartMs = 0.0, chopDurMs = 0.0;
    uint32_t chopSeen = 0;

    // Barra degli strumenti
    IconButton detectButton { "Detect", IconButton::Icon::Wave };
    juce::Slider sensitivity;
    juce::ComboBox evenBox;
    IconButton clearButton { "Clear", IconButton::Icon::Trash };
    IconButton chopButton { "Chop", IconButton::Icon::Scissors };
    juce::ToggleButton auditionToggle { "Audition" };
    juce::Slider auditionVolume;

    std::unique_ptr<WaveView> wave;
    std::unique_ptr<CellsStrip> cellsStrip;
    juce::ToggleButton keepToggle;
    IconButton distributeButton { "Distribute to cells", IconButton::Icon::ArrowRight };

    juce::Rectangle<int> toolbarRect, waveRect, cellsRect, actionRect, infoRect, sensLabelRect, titleRect;
    std::vector<int> toolbarSeps;
};
