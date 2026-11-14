#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <map>
#include <array>
#include <deque>
#include <optional>
#include <atomic>
#include "engine/Engine.h"
#include "engine/LoopStretch.h"
#include "kit/KitIO.h"
#include "sequencer/SequencerExport.h"
#include "params/ParamSet.h"
#include "params/MidiLearn.h"

class SampleLabProcessor final : public juce::AudioProcessor
{
public:
    SampleLabProcessor();
    ~SampleLabProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Parametri automatizzabili (fonte di verita' dei valori) ----
    const sl::ParamSet& getParams() const { return params; }

    // ---- Sequencer (thread messaggi, tranne dove indicato) ----
    sl::Pattern& activePattern() { return kit.patterns[(size_t) kit.activePattern]; }
    const sl::Pattern& activePattern() const { return kit.patterns[(size_t) kit.activePattern]; }
    int getActivePatternIndex() const { return kit.activePattern; }
    void setActivePattern (int index);

    // Operazioni sui pattern (thread messaggi)
    bool duplicateActivePattern();                         // copia il pattern attivo nel prossimo pattern VUOTO e lo attiva; false se non ce n'e'
    void copyActivePattern();                              // negli appunti dei pattern
    bool hasPatternClipboard() const { return patternClipboard.has_value(); }
    void pasteIntoActivePattern();                         // sostituisce il contenuto del pattern attivo (il nome resta)
    void clearActivePattern();
    void renamePattern (int index, const juce::String& name);
    void patternEdited();                                   // dopo ogni modifica del pattern: pubblica al thread audio
    bool addLaneForCell (int cell);                         // "Aggiungi al sequencer" (tasto destro sulla cella)
    void removeLaneForCell (int cell);

    // Pattern Sequencer (song): catena di pattern. Con la modalita' song il Play del sequencer suona la catena invece del pattern attivo.
    sl::Song& song() { return kit.song; }
    const sl::Song& song() const { return kit.song; }
    void songEdited();                                      // dopo ogni modifica della song: la pubblica al thread audio
    void setSongMode (bool on);
    void setSongStartBeats (double beats) { songStart.store (juce::jmax (0.0, beats)); }     // da dove parte il Play nella scheda Song (inizio di un blocco)
    bool isSongMode() const { return songMode.load(); }
    double songTotalBeats() const { return kit.song.totalBeats (kit.patterns); }
    bool saveSongFile (const juce::File& file, juce::String& error) const;
    bool loadSongFile (const juce::File& file, juce::String& error);
    bool exportSongMidi (const juce::File& file, juce::String& error);
    bool bounceSongWav (const juce::File& file, juce::String& error);
    int exportSongStems (const juce::File& folder, juce::String& error);      // un WAV per cella usata nella song; -1 se errore

    // Trasporto: nello standalone e' interno (Play/Stop del plugin), nel plugin segue la DAW
    void setSequencerRunning (bool run);
    bool isSequencerRunning() const { return seqRunning.load(); }
    bool isSequencerPlaying() const { return seqPlaying.load(); }     // in riproduzione ora (DAW o interno)
    // Trasporto della DAW (plugin): il sequencer lo segue, resta in silenzio mentre la DAW registra, oppure lo ignora
    enum class HostSeqMode : int { Follow = 0, MuteOnRecord = 1, Ignore = 2 };
    HostSeqMode getHostSeqMode() const { return (HostSeqMode) hostSeqMode.load(); }
    void setHostSeqMode (HostSeqMode m) { hostSeqMode.store ((int) m); dirty = true; }
    int getSequencerPlayingPattern() const { return seqPlayIndex.load(); }     // -1 se fermo
    bool isHostPlaying() const { return hostRolling.load(); }         // il trasporto della DAW sta suonando: comanda lui
    double getSequencerPpq() const { return seqPpq.load(); }         // posizione corrente in quarti (per l'indicatore degli step)

    // Esportazione del pattern attivo: clip MIDI e audio (WAV 24 bit con effetti, lock e tail)
    bool exportPatternMidi (const juce::File& file, int bars, juce::String& error);
    bool bouncePatternWav (const juce::File& file, int bars, juce::String& error);

    // Code di delay/riverbero nei file audio esportati (Bounce WAV, Export lanes e le maniglie Drag audio/lanes)
    using ExportTail = sl::SequencerExport::TailMode;
    ExportTail getExportTail() const { return exportTail.load(); }
    void setExportTail (ExportTail t) { exportTail.store (t); }
    int exportPatternStems (const juce::File& folder, int bars, juce::String& error);    // un WAV per riga; ritorna quanti, -1 se errore

    // ---- MIDI learn dei parametri ----
    sl::MidiLearn& getMidiLearn() { return midiLearn; }
    const sl::MidiLearn& getMidiLearn() const { return midiLearn; }
    void markDirty() { dirty = true; }
    void pollMidiLearn() { if (midiLearn.poll()) dirty = true; }     // chiamato dal timer dell'interfaccia

    // ---- Kit (da usare solo dal thread messaggi) ----
    // Attenzione: i valori automatizzabili (level, pan, inviluppo, effetti...) stanno nei parametri;
    // il kit li contiene solo dopo un salvataggio. Per mostrarli nell'interfaccia usare getParams().
    sl::Kit& getKit() { return kit; }
    const sl::Kit& getKit() const { return kit; }
    bool isDirty() const { return dirty.load(); }

    bool loadSampleIntoCell (int cell, const juce::File& file, juce::String& error);
    void clearCell (int cell);                     // toglie il campione e riporta la cella al suo tasto di partenza
    // Annulla / ripeti per le schede Main, FX, Seq e Slice: celle (campioni, nome, tasti, slice, tempo), tutti i parametri (manopole) e i
    // pattern del sequencer (step, righe, swing, nome). Le modifiche si riconoscono confrontando lo stato a intervalli (undoTick, ~10 Hz): una trascinata di manopola o una
    // distribuzione di slice e' una voce sola. Contano solo le modifiche fatte dall'utente (noteUserInput), non l'automazione della DAW.
    void undoTick (bool mouseDown);
    void noteUserInput() { lastUserInputMs = juce::Time::getMillisecondCounterHiRes(); }
    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }
    double undoTime() const { return undoStack.empty() ? 0.0 : undoStack.back().when; }     // per decidere quale annullamento e' il piu' recente
    double redoTime() const { return redoStack.empty() ? 0.0 : redoStack.back().when; }
    bool undo();
    bool redo();
    void resetUndo();                              // dopo aver aperto/creato un kit: la cronologia riparte

    void resetKeyRange (int cell);                 // root/low/high tornano al tasto predefinito della cella (A1 = C1, A2 = C#1...)

    // Normalizzazione: porta il picco del campione (tra Start ed End) al livello target regolando il Level della cella.
    // Non modifica il file audio. Ritorna il guadagno applicato in dB; 0 se la cella e' vuota o silenziosa.
    float normalizeCell (int cell, float targetDb = -1.0f);
    int normalizeAllCells (float targetDb = -1.0f);               // ritorna quante celle sono state normalizzate
    void cellEdited (int cell);                    // parametri modificati: marca modificato e pubblica
    sl::SampleDataPtr getCellData (int cell) const;

    // Loop: sincronizzazione al tempo (BPM della DAW o interno) con Key Lock. Il campione originale non cambia: quello usato
    // dal motore e' una copia elaborata in background con Rubber Band (o l'originale se la sincronizzazione e' spenta).
    // Slice: anteprima di una regione del campione di una cella; distribuzione delle slice sulle celle (stesso file, Start/End diversi)
    void auditionCellRegion (int cell, float start, float end, float gainDb);
    void auditionData (sl::SampleDataPtr data, float start, float end, float gainDb);   // anteprima di un campione qualsiasi (sorgente della scheda Slice)
    sl::SampleDataPtr loadAudioFile (const juce::File& file);                           // decodifica un file esterno (non entra nel kit)
    // Slice da una sorgente esterna: ogni pezzo viene ritagliato in un WAV in 'dir' (il kit non dipende dal file originale) e caricato in una cella
    int distributeSliceData (sl::SampleDataPtr data, const juce::String& baseName, const std::vector<float>& bounds, int firstTarget,
                             float bpm, bool sync, const juce::File& dir);
    int distributeSlices (int sourceCell, const std::vector<float>& bounds, int firstTarget);   // bounds = n+1 limiti; ritorna le celle riempite
    void detectLoopBpm (int cell);                 // stima il BPM del loop dalla durata (regione Start..End) e lo imposta sulla cella
    bool isStretching (int cell) const;            // true mentre la copia elaborata e' in preparazione

    void newKit();
    bool openKit (const juce::File& file, juce::String& error);
    bool saveKit (const juce::File& file, juce::String& error);

    // Kit recenti (ultimi 5, il piu' recente in alto), condivisi tra tutte le istanze
    juce::StringArray getRecentKits() const;
    void removeRecentKit (const juce::String& path);
    void clearRecentKits();

    std::pair<float, float> getMasterLevels() const { return engine.getMasterLevels(); }
    std::pair<float, float> takeCellPeaks (int cell) { return engine.takeCellPeaks (cell); }     // mixer: picchi dall'ultima lettura
    std::pair<float, float> takeMasterPeaks() { return engine.takeMasterPeaks(); }

    // Tempo per gli effetti sincronizzati: BPM della DAW se la DAW lo fornisce, altrimenti il BPM interno (standalone)
    double getCurrentBpm() const { return currentBpm.load(); }
    bool isHostTempo() const { return hostTempo.load(); }
    void setInternalBpm (double bpm) { internalBpm.store (juce::jlimit (20.0, 400.0, bpm)); }
    void setInternalBpmByUser (double bpm) { setInternalBpm (bpm); bpmEdited.store (true); }   // scritto a mano nel riquadro BPM
    // Le esportazioni audio hanno bisogno del tempo del progetto. Nel plugin lo si conosce solo dopo che la DAW lo ha mandato (di solito
    // quando parte il trasporto) o se e' stato scritto a mano: altrimenti si esporterebbe al tempo interno predefinito (120), sbagliato.
    bool exportTempoKnown() const
    {
        return wrapperType == juce::AudioProcessor::wrapperType_Standalone || lastHostBpm.load() > 0.0 || bpmEdited.load() || exportBpmOverride.load() > 0.0;
    }
    // Tempo usato dalle esportazioni (audio, MIDI): quello scelto a mano ("tempo di esportazione", 0 = automatico) oppure il tempo corrente
    // (DAW o interno). Nel plugin serve se la DAW manda un tempo diverso da quello del brano.
    double getExportBpm() const { const double o = exportBpmOverride.load(); return o > 0.0 ? o : currentBpm.load(); }
    double getExportBpmOverride() const { return exportBpmOverride.load(); }
    void setExportBpmOverride (double bpm) { exportBpmOverride.store (bpm >= 20.0 ? juce::jmin (400.0, bpm) : 0.0); }
    void logExport (const juce::String& what) const;           // riga nel diario dell'interfaccia: tempo e frequenza usati
    static juce::String tempoUnknownMessage()
    {
        return "NINA hasn't received the project tempo from your DAW yet, so the audio would be rendered at the wrong speed.\n\n"
               "Press Play in the DAW once (NINA then reads the tempo and remembers it), or type the tempo in the BPM box at the top "
               "(double-click it), and then export again.";
    }

    // Anteprima dei file del browser (caricamento in background, riproduzione su una voce dedicata)
    void auditionFile (const juce::File& file, float gainDb);
    void stopAudition();

    // Anteprima da UI (pad cliccato) e attivita' per l'indicatore delle celle
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Chop (scheda Slice): con la cattura accesa le note MIDI in ingresso non suonano le celle ma vengono solo registrate
    // (istante di ogni nota, in ms del contatore ad alta risoluzione) per mettere i marcatori mentre il loop suona.
    void setChopCapture (bool on) { chopCapture.store (on, std::memory_order_relaxed); }
    // Registrazione da tastiera nel sequencer: con il sequencer in riproduzione, le note MIDI in ingresso vengono annotate con la
    // loro posizione musicale (quarti) e l'interfaccia le scrive negli step
    struct RecNote { double ppq = 0.0; int note = 0; int velocity = 100; };
    void setSeqRecord (bool on) { if (on) recTail = recHead.load (std::memory_order_acquire); seqRecord.store (on); }
    bool isSeqRecording() const { return seqRecord.load(); }
    // Metronomo e conteggio iniziale (precount): il click suona a ogni quarto mentre il sequencer gira (accento sul primo di ogni battuta,
    // 4/4); con la registrazione accesa e il trasporto interno, prima di partire il sequencer conta 'countInBars' battute (clic anche
    // a metronomo spento).
    void setMetronome (bool on) { clickOn.store (on); }
    bool isMetronomeOn() const { return clickOn.load(); }
    void setClickVolume (float v) { clickVol.store (juce::jlimit (0.0f, 1.0f, v)); }
    float getClickVolume() const { return clickVol.load(); }
    void setCountInBars (int bars) { countInBars.store (juce::jlimit (0, 2, bars)); }
    int getCountInBars() const { return countInBars.load(); }
    int drainRecorded (RecNote* out, int maxCount);

    uint32_t chopNoteCount() const { return chopHead.load (std::memory_order_acquire); }
    double chopNoteTime (uint32_t index) const { return chopTimes[index % chopTimes.size()].load (std::memory_order_relaxed); }
    int getCellActivity (int cell) const { return engine.getCellActivity (cell); }

    // Dimensione/zoom dell'interfaccia (persistiti nello stato del progetto, UI-12)
    int uiWidth = 1280;
    int uiHeight = 760;
    float uiZoom = 1.0f;

    static constexpr int kMaxRecentKits = 5;

private:
    static BusesProperties makeBuses();
    void publishKit (bool newGeneration);
    void publishPattern();
    static bool writeWav24 (const juce::File& file, const juce::AudioBuffer<float>& audio, double sampleRate, int numSamples, juce::String& error);
    std::shared_ptr<sl::KitSnapshot> buildSnapshot() const;
    void loadKitSamples();
    void ensurePlayData();                                   // allinea i campioni letti dal motore con quelli della cella
    void refreshStretch (int cell);                          // avvia (se serve) l'elaborazione per il tempo corrente
    void tempoTick();                                        // timer: controlla il BPM e aggiorna i loop sincronizzati
    void addRecentKit (const juce::File& file);
    juce::PropertiesFile& getSettings() const;

    // Segna il kit come modificato quando cambia un parametro (da qualsiasi thread: UI, automazione, MIDI)
    struct DirtyListener final : juce::AudioProcessorParameter::Listener
    {
        explicit DirtyListener (std::atomic<bool>& f) : flag (f) {}
        void parameterValueChanged (int, float) override { flag.store (true); }
        void parameterGestureChanged (int, bool) override {}
        std::atomic<bool>& flag;
    };

    sl::ParamSet params;
    sl::MidiLearn midiLearn { params };
    sl::Engine engine;

    // Sequencer: pattern attivo pubblicato al thread audio, generatore di eventi e trasporto interno
    // Il thread audio riceve il pattern mostrato (in modifica) e, se e' in corso un altro pattern, anche quello: cosi' puo' finire
    // il ciclo del vecchio pattern e poi passare al nuovo (cambio "a fine pattern").
    struct PatternMsg
    {
        sl::Pattern shown;                 // pattern selezionato nell'interfaccia
        int shownIndex = 0;
        bool hasPlay = false;              // true se sta ancora suonando un altro pattern
        sl::Pattern play;
        int playIndex = -1;
    };
    sl::SharedExchange<PatternMsg> patternExchange;
    std::shared_ptr<const PatternMsg> audioMsg;             // solo thread audio
    const sl::Pattern* curPat = nullptr;                    // pattern in riproduzione (punta dentro audioMsg)
    const sl::Pattern* nextPat = nullptr;                   // pattern in attesa del prossimo punto di cambio
    int curIndex = -1, nextIndex = -1;
    std::atomic<int> seqPlayIndex { -1 };                   // indice del pattern in riproduzione (-1 = fermo)
    struct SongMsg
    {
        std::vector<sl::Pattern> pats;      // tutti i pattern del kit al momento della pubblicazione
        std::vector<sl::Segment> segs;      // puntano dentro 'pats'
        double total = 0.0;
        bool loop = false;
    };
    sl::SharedExchange<SongMsg> songExchange;
    std::shared_ptr<const SongMsg> songMsg;                 // solo thread audio
    std::atomic<bool> songMode { false };
    std::atomic<double> songStart { 0.0 };
    void publishSong();
    void songPlan (std::vector<sl::Pattern>& pats, std::vector<sl::Segment>& segs, double& total, int isolateCell) const;
    sl::SequencerRunner seqRunner;
    std::atomic<bool> seqRunning { false }, seqPlaying { false }, hostRolling { false };
    std::atomic<int> hostSeqMode { (int) HostSeqMode::MuteOnRecord };
    std::atomic<double> seqPpq { 0.0 };
    double internalPpq = 0.0;                               // solo thread audio
    bool wasRunning = false;                                // solo thread audio
    double currentSampleRate = 44100.0;
    juce::MidiKeyboardState keyboardState;
    juce::AudioFormatManager formats;

    sl::Kit kit = sl::Kit::makeDefault();
    std::vector<sl::SampleDataPtr> cellData;                 // campioni originali
    std::vector<sl::SampleDataPtr> playData;                 // campioni letti dal motore (originali o elaborati)

    struct StretchState
    {
        sl::SampleDataPtr source;                            // originale da cui deriva playData
        bool sync = false, keyLock = true;
        double loopBpm = 0.0, bpm = 0.0;                     // valori con cui playData e' stato elaborato
        double pendingLoopBpm = 0.0, pendingBpm = 0.0;       // elaborazione in corso (0 = nessuna)
        bool pendingKeyLock = true;
        int token = 0;
        std::deque<sl::SampleDataPtr> retired;               // le voci attive possono leggere ancora i buffer vecchi
    };
    std::vector<StretchState> stretch;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
    struct TempoTimer final : juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { if (fn) fn(); }
    } tempoTimer;
    std::map<juce::String, sl::SampleDataPtr> sampleCache;   // i campioni restano in memoria per la sessione
    std::optional<sl::Pattern> patternClipboard;
    std::atomic<bool> dirty { false };
    std::atomic<sl::SequencerExport::TailMode> exportTail { sl::SequencerExport::TailMode::Cut };
    DirtyListener dirtyListener { dirty };
    std::atomic<double> internalBpm { 120.0 }, currentBpm { 120.0 };
    std::atomic<bool> hostTempo { false };
    std::atomic<double> lastHostBpm { 0.0 };                 // ultimo tempo ricevuto dalla DAW (0 = mai)
    std::atomic<bool> bpmEdited { false };
    std::atomic<double> exportBpmOverride { 0.0 };
    int generation = 0;

    mutable std::unique_ptr<juce::ApplicationProperties> appProps;

    // Anteprima: gli ultimi campioni restano vivi finche' la voce audio puo' ancora leggerli
    juce::CriticalSection auditionLock;
    std::deque<sl::SampleDataPtr> auditionKeep;
    std::atomic<int> auditionToken { 0 };
    struct UndoSnap
    {
        std::vector<sl::CellParams> cells;
        std::vector<sl::SampleDataPtr> data;
        std::vector<float> params;
        sl::Song song;
        std::vector<std::shared_ptr<const sl::Pattern>> patterns;     // i pattern non modificati sono condivisi tra le voci
        std::vector<juce::uint64> patHashes;
        juce::uint64 hash = 0;
        double when = 0.0;
    };
    UndoSnap captureUndo() const;
    juce::uint64 hashUndoState() const;
    std::vector<juce::uint64> patternHashes() const;
    struct PatCache { juce::uint64 hash = 0; std::shared_ptr<const sl::Pattern> ptr; };
    mutable std::vector<PatCache> patCache;
    void applyUndo (const UndoSnap&);
    const std::vector<juce::RangedAudioParameter*>& undoParams() const;
    UndoSnap undoBaseline;
    bool undoBaselineValid = false;
    double undoBaselineAt = 0.0, lastUserInputMs = 0.0;
    std::vector<UndoSnap> undoStack, redoStack;
    mutable std::vector<juce::RangedAudioParameter*> undoParamList;

    std::atomic<bool> seqRecord { false };
    std::atomic<bool> clickOn { false };
    std::atomic<float> clickVol { 0.67f };                  // volume del click (0..1; 0.67 = livello di base)
    std::atomic<int> countInBars { 1 };
    // Sintesi del click (solo thread audio)
    int clickLeft = 0, clickTotal = 0;
    float clickPhase = 0.0f, clickFreq = 1200.0f, clickAmp = 0.0f;
    std::array<RecNote, 256> recRing {};
    std::atomic<uint32_t> recHead { 0 };
    uint32_t recTail = 0;                                   // solo thread UI
    std::atomic<bool> chopCapture { false };
    std::array<std::atomic<double>, 32> chopTimes {};
    std::atomic<uint32_t> chopHead { 0 };
    juce::ThreadPool auditionPool { 1 };
    juce::ThreadPool stretchPool { 1 };    // ultimo membro: viene distrutto (e atteso) per primo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabProcessor)
};
