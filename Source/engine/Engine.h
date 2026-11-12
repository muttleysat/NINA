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

    // Valori automatizzabili correnti (cella per cella + master). Li scrive chi chiama process(), nello stesso thread,
    // subito prima di ogni blocco (il processore li legge dai parametri della DAW). Nessuna copia dal thread UI.
    struct LiveParams
    {
        std::array<CellDsp, kMaxCells> dsp;
        std::array<FxParams, kMaxCells> fx;
        FxParams masterFx;
        float masterGain = 1.0f;
        double bpm = 120.0;             // tempo corrente (DAW o interno) per gli effetti sincronizzati
    };
    LiveParams live;

    // Alloca gli stati degli effetti: da chiamare fuori dal thread audio (prepareToPlay).
    void prepare (double sampleRate, int maxBlockSize = 1024);

    // Thread UI
    void publish (KitSnapshotPtr snapshot) { exchange.publish (std::move (snapshot)); }

    // Anteprima di un campione (browser). Thread UI/qualsiasi: il campione deve restare vivo finche' suona.
    void playPreview (const SampleData* data, float gain);
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
    void applyLocks() noexcept;                      // parameter lock degli step sopra i valori correnti degli effetti

    // Parameter lock attivi per cella: restano finche' la cella riceve un nuovo step (o il sequencer si ferma)
    std::array<FxParams, kMaxCells> effFx;           // effetti effettivi del blocco = parametri live + lock degli step
    std::array<std::uint32_t, kMaxCells> lockMask {};
    std::array<std::array<float, (size_t) kNumLockable>, kMaxCells> lockValues {};
    void renderVoices (float* l, float* r, int start, int count) noexcept;
    void renderChunk (float* l, float* r, int n) noexcept;

    double sampleRate = 44100.0;
    int maxBlock = 1024;
    int tailSamples = 0;                // durata di coda concessa a delay/riverbero dopo la fine delle voci

    std::array<std::unique_ptr<FxChain>, kMaxCells> cellFx;
    std::array<int, kMaxCells> cellTail {};
    FxChain masterFx;
    std::vector<float> scratchL, scratchR;
    SnapshotExchange exchange;
    KitSnapshotPtr current;
    int lastGeneration = -1;
    juce::uint64 ageCounter = 0;

    SnapshotExchange previewExchange;
    KitSnapshotPtr previewCurrent;
    Voice previewVoice;

    std::array<Voice, kMaxVoices> voices;
    std::array<std::atomic<int>, kMaxCells> activity {};
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };
    std::atomic<float> previewGain { 1.0f };
};
}
