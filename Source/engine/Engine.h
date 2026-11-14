#pragma once

#include "Voice.h"
#include "FxChain.h"
#include "sequencer/Sequencer.h"
#include <array>
#include <atomic>
#include <memory>

namespace sl
{
class Engine
{
public:
    static constexpr int kMaxVoices = 64;
    static constexpr int kMaxCells = 48;    // griglia 4x12; tutti i parametri di queste celle sono automatizzabili
    static constexpr int kMaxOutputs = 16;  // Main + 15 uscite stereo aggiuntive

    // Valori automatizzabili correnti (cella per cella + master). Li scrive chi chiama process(), nello stesso thread,
    // subito prima di ogni blocco (il processore li legge dai parametri della DAW). Nessuna copia dal thread UI.
    struct LiveParams
    {
        std::array<CellDsp, kMaxCells> dsp;
        std::array<FxParams, kMaxCells> fx;
        std::array<bool, kMaxCells> mute {}, solo {};     // mixer: Mute e Solo per cella
        FxParams masterFx;
        float masterGain = 1.0f;
        double bpm = 120.0;             // tempo corrente (DAW o interno) per gli effetti sincronizzati
    };
    LiveParams live;

    // Uscite multiple: outputMap[k] = indice del canale sinistro dell'uscita k nel buffer (la destra e' il canale successivo);
    // -1 = uscita non disponibile (bus spento dalla DAW): le celle assegnate a quell'uscita suonano nel Main. L'uscita 0 (Main)
    // e' sempre sui canali 0/1. Si scrive dal thread audio, nello stesso blocco in cui si chiama process().
    std::array<int, kMaxOutputs> outputMap = [] { std::array<int, kMaxOutputs> m {}; m.fill (-1); m[0] = 0; return m; }();

    // Alloca gli stati degli effetti: da chiamare fuori dal thread audio (prepareToPlay).
    void prepare (double sampleRate, int maxBlockSize = 1024);

    // Thread UI
    void publish (KitSnapshotPtr snapshot) { exchange.publish (std::move (snapshot)); }

    // Anteprima di un campione (browser). Thread UI/qualsiasi: il campione deve restare vivo finche' suona.
    void playPreview (const SampleData* data, float gain, float start = 0.0f, float end = 1.0f);   // start/end: regione (0..1)
    void stopPreview();

    // Thread audio. Somma nel buffer (che deve essere gia' azzerato dal chiamante).
    // seq: eventi del sequencer per questo blocco (ordinati per posizione), puo' essere nullptr.
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                  const std::vector<SeqEvent>* seq = nullptr) noexcept;

    // Livello di picco dell'ultimo blocco (L/R), letto dalla UI per il meter master
    std::pair<float, float> getMasterLevels() const noexcept
    {
        return { peakL.load (std::memory_order_relaxed), peakR.load (std::memory_order_relaxed) };
    }

    // Picchi accumulati dall'ultima lettura (mixer): per cella (dopo effetti, Mute e Solo) e per il master. Leggere azzera.
    std::pair<float, float> takeCellPeaks (int cell) noexcept
    {
        if (cell < 0 || cell >= kMaxCells)
            return { 0.0f, 0.0f };
        return { cellPeakL[(size_t) cell].exchange (0.0f, std::memory_order_relaxed), cellPeakR[(size_t) cell].exchange (0.0f, std::memory_order_relaxed) };
    }
    std::pair<float, float> takeMasterPeaks() noexcept
    {
        return { masterAccL.exchange (0.0f, std::memory_order_relaxed), masterAccR.exchange (0.0f, std::memory_order_relaxed) };
    }

    // Numero di voci attive per cella (letto dalla UI per l'indicatore di attivita')
    int getCellActivity (int cell) const noexcept
    {
        return (cell >= 0 && cell < kMaxCells) ? activity[(size_t) cell].load (std::memory_order_relaxed) : 0;
    }

private:
    CellDsp liveDsp (int cell) const noexcept;       // parametri live + mapping note dello snapshot
    void noteOn (int note, float velocity) noexcept;
    void noteOff (int note) noexcept;
    Voice* allocateVoice() noexcept;
    void seqEvent (const SeqEvent& e) noexcept;      // NoteOn/NoteOff/Stop dal sequencer
    void renderVoices (int start, int count) noexcept;
    void renderChunk (int start, int n) noexcept;
    void renderMaster (float* l, float* r, int n) noexcept;

    double sampleRate = 44100.0;
    int maxBlock = 1024;
    int tailSamples = 0;                // durata di coda concessa a delay/riverbero dopo l'ultima mandata

    // Effetti di cella: filtro/saturazione/LoFi stanno nelle voci (per colpo); compressore, delay e riverbero nella cella
    std::array<std::unique_ptr<FxChain>, kMaxCells> cellFx;
    std::array<int, kMaxCells> delayTail {}, reverbTail {};     // campioni di coda rimasti dopo l'ultima mandata
    FxChain masterFx;
    InsertFx masterInsert;
    bool masterDelayWasOn = false, masterReverbWasOn = false;

    // Buffer di lavoro (preallocati): bus della cella, singola voce, mandate al delay e al riverbero
    std::vector<float> busL, busR, voiceL, voiceR, sendDL, sendDR, sendRL, sendRR;
    SnapshotExchange exchange;
    KitSnapshotPtr current;
    int lastGeneration = -1;
    juce::uint64 ageCounter = 0;
    std::array<float*, kMaxOutputs> outL {}, outR {};     // puntatori di uscita del blocco corrente (nullptr = non disponibile)

    SnapshotExchange previewExchange;
    KitSnapshotPtr previewCurrent;
    Voice previewVoice;

    std::array<FxParams, kMaxCells> effFx;           // copia dei parametri effetti del blocco corrente
    std::array<Voice, kMaxVoices> voices;
    std::array<std::atomic<int>, kMaxCells> activity {};
    std::array<bool, kMaxCells> cellOnset {};          // un nuovo colpo e' appena partito sulla cella (per lo stutter)
    bool masterOnset = false;
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };
    std::array<std::atomic<float>, kMaxCells> cellPeakL {}, cellPeakR {};
    std::atomic<float> masterAccL { 0.0f }, masterAccR { 0.0f };
    static void raise (std::atomic<float>& a, float v) noexcept { if (v > a.load (std::memory_order_relaxed)) a.store (v, std::memory_order_relaxed); }
    std::atomic<float> previewGain { 1.0f };
    std::atomic<float> previewStart { 0.0f }, previewEnd { 1.0f };
};
}
