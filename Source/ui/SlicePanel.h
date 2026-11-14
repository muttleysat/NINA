#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "IconButton.h"

// Scheda Slice: tagliare un campione in slice (rilevamento dei colpi, divisioni uguali, Chop a tempo, marcatori modificabili a mano)
// e distribuirle sulle celle (stesso file con Start/End per una cella; pezzi ritagliati in file separati per un file esterno).
// La sorgente e' la cella selezionata oppure un file audio esterno (trascinato o scelto con Load) di cui si sceglie la zona.
class SlicePanel final : public juce::Component,
                         public juce::FileDragAndDropTarget,
                         public juce::DragAndDropTarget,
                         private juce::Timer
{
public:
    explicit SlicePanel (SampleLabProcessor&);
    ~SlicePanel() override;

    void setCell (int cell);               // cella (e quindi campione) su cui si lavora in modalita' Cell
    void refresh();                        // il kit o la regione Start/End sono cambiati
    void runDetect() { doDetect(); }       // come premere Detect (anche per le schermate automatiche)
    void runDetectBpm() { doDetectBpm(); }  // come premere Detect nella riga TEMPO
    void loadFile (const juce::File& file); // carica un file esterno come sorgente (passa alla modalita' File)
    void reloadFromKit();                  // dopo un annullamento: i marcatori della cella tornano quelli del kit

    // Annulla / ripeti locale per la sorgente File (marcatori, zona, BPM): per la sorgente Cell se ne occupa la cronologia del kit
    double localUndoTime() const { return source == Source::File && ! localUndoStack.empty() ? localUndoStack.back().when : 0.0; }
    double localRedoTime() const { return source == Source::File && ! localRedoStack.empty() ? localRedoStack.back().when : 0.0; }
    bool localUndo();
    bool localRedo();

    // Chop: il loop suona e ogni pressione di M (o di una nota MIDI) mette un marcatore nel punto in cui si trova
    bool isChopping() const { return chopping; }
    bool isPlaying() const { return cursorActive(); }         // Chop, prova del tempo, zona o slice in ascolto
    void togglePlay();                     // barra spaziatrice: suona la zona / ferma tutto
    void stopAll();                        // ferma qualsiasi riproduzione (usato anche internamente)
    void userStop();                       // Stop / Esc / barra spaziatrice: ferma e lascia il cursore nel punto raggiunto
    void setZoneEdgeAtMarker (bool start); // tasti [ e ]: porta l'inizio / la fine della zona (sorgente File) sul cursore di posizione
    bool hasMarker() const { return playMarker >= 0.0f; }
    void setMarker (float p);              // posiziona il cursore di posizione (0..1 sul campione, -1 = nessuno)
    void chopMark();                       // tasto M

    std::function<void()> onKitChanged;    // dopo la distribuzione: l'editor ridisegna griglia e pannelli
    std::function<juce::File()> browserFile;   // file selezionato nel browser (per il trascinamento dal browser)

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (! isShowing()) stopAll(); }

    // Trascinamento di un file audio (da Esplora risorse o dal browser)
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { setDragOver (true); }
    void fileDragExit (const juce::StringArray&) override { setDragOver (false); }
    bool isInterestedInDragSource (const SourceDetails& d) override { return d.description == "SLFILE"; }
    void itemDropped (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override { setDragOver (true); }
    void itemDragExit (const SourceDetails&) override { setDragOver (false); }

private:
    class WaveView;
    class CellsStrip;
    friend class WaveView;
    friend class CellsStrip;

    enum class Source { Cell, File };

    // Dati di lavoro
    sl::SampleDataPtr data() const { return source == Source::File ? extData : processor.getCellData (cell); }
    float regionStart() const;
    float regionEnd() const;
    float viewLo() const { return source == Source::File ? 0.0f : regionStart(); }    // la forma d'onda mostra tutto il file (File) o la sola regione (Cell)
    float viewHi() const { return source == Source::File ? 1.0f : regionEnd(); }
    std::vector<float> bounds() const;                         // marcatori + fine regione (n+1 limiti)
    std::vector<int> targetCells() const;                      // celle che verrebbero riempite
    int firstTarget() const;
    bool hasSource() const;
    juce::String sourceName() const;
    void cellsStripRepaint();
    void markersChanged();                                     // salva nel kit (Cell) o nello stato della scheda (File) e ridisegna
    void regionChanged();                                      // la zona del file e' stata cambiata: marcatori dentro la zona
    struct LocalSnap
    {
        std::vector<float> markers;
        float start = 0.0f, end = 1.0f, bpm = 0.0f;
        double when = 0.0;
    };
    LocalSnap localCapture() const;
    void localCommit();                                        // se qualcosa e' cambiato dall'ultimo passo, registra lo stato precedente
    void localReset();
    void localApply (const LocalSnap&);
    LocalSnap localBase;
    std::vector<LocalSnap> localUndoStack, localRedoStack;

    void clampMarkers();                                       // marcatori sempre dentro la regione e almeno uno: l'inizio
    int distributeFromFile (const std::vector<float>& bounds, int firstTarget);
    void setSource (Source s);
    void updateSourceControls();
    void chooseFile();
    void setDragOver (bool on) { if (dragOver != on) { dragOver = on; repaint(); } }
    void auditionSlice (int index);
    void doDetect();
    void doEven (int count);
    void doClear();
    void doDistribute();
    void startChop();
    void stopChop (bool finished = false);
    void addChopMarker (double timeMs);
    double chopPosition (double nowMs) const;                  // posizione (0..1 sul campione) raggiunta dall'anteprima

    // Tempo della sorgente
    float srcBpm() const;
    void setSrcBpm (float bpm);
    void doDetectBpm();
    void doTap();
    void scaleBpm (double factor);
    void doClickTest();
    void stopTest (bool finished = false);
    void scrubTo (float pos);                                  // trascinando sulla forma d'onda: breve ascolto nel punto indicato
    void startPreview (float lo, float hi);                    // ascolto di una parte (zona o slice) con il cursore
    void stopPreview (bool finished = false);
    bool cursorActive() const { return chopping || testPlaying || previewing; }

    void setTargetStart (int cellIndex);
    void showStatus (const juce::String& text);
    void timerCallback() override;

    SampleLabProcessor& processor;
    int cell = 0;
    Source source = Source::Cell;
    std::vector<float> markers;                                // inizio di ogni slice, ordinati (0..1 sul campione)
    int targetStart = -1;                                      // -1 = automatico
    bool keepOriginal = true;
    juce::String status;
    double statusUntil = 0.0;
    bool dragOver = false;

    // Sorgente esterna
    sl::SampleDataPtr extData;
    juce::File extFile;
    float extStart = 0.0f, extEnd = 1.0f;
    float extBpm = 0.0f;
    std::vector<float> extMarkers;

    // Chop e prova del tempo (cursore sulla forma d'onda)
    bool chopping = false, testPlaying = false, previewing = false;
    double chopStartMs = 0.0, chopDurMs = 0.0;
    double lastScrubMs = 0.0;
    float playMarker = -1.0f;                                   // cursore di posizione (0..1 sul campione): resta dopo lo scrub e lo Stop
    void setZoneEdge (bool start, float p);
    double cursorLo = 0.0, cursorHi = 1.0;                      // parte del campione che sta suonando (per il cursore)
    uint32_t chopSeen = 0;
    std::vector<double> taps;

    // Riga SOURCE
    juce::TextButton cellSrcButton { "Cell" }, fileSrcButton { "File" };
    IconButton playButton { "Play zone", IconButton::Icon::ArrowRight };
    IconButton loadButton { "Load file...", IconButton::Icon::Layers };
    juce::ToggleButton auditionToggle { "Audition" };
    juce::Slider auditionVolume;

    // Riga CUT
    IconButton detectButton { "Detect", IconButton::Icon::Wave };
    juce::Slider sensitivity;
    juce::ComboBox evenBox;
    IconButton clearButton { "Clear", IconButton::Icon::Trash };
    IconButton chopButton { "Chop", IconButton::Icon::Scissors };

    // Riga TEMPO
    juce::Slider bpmSlider;
    juce::TextButton bpmDetectButton { "Detect" }, halfButton { "/2" }, doubleButton { "x2" }, tapButton { "Tap" };
    IconButton clickButton { "Click test", IconButton::Icon::Midi };

    std::unique_ptr<WaveView> wave;
    std::unique_ptr<CellsStrip> cellsStrip;
    juce::ToggleButton keepToggle, syncToggle { "Sync slices to tempo" };
    IconButton distributeButton { "Distribute to cells", IconButton::Icon::ArrowRight };
    std::unique_ptr<juce::FileChooser> chooser;

    struct Sep { int x; juce::Rectangle<int> row; };
    juce::Rectangle<int> sourceRect, toolbarRect, tempoRect, waveRect, cellsRect, actionRect, infoRect, sensLabelRect, bpmLabelRect, nameRect;
    juce::Rectangle<int> sourceTitle, cutTitle, tempoTitle;
    std::vector<Sep> seps;
};
