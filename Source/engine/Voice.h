#pragma once

#include "KitSnapshot.h"
#include "FxChain.h"

namespace sl
{
// Una voce di riproduzione: legge un campione con interpolazione cubica, pitch, reverse/loop e AHDSR.
class Voice
{
public:
    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }
    int getNote() const noexcept { return note; }
    int getCellIndex() const noexcept { return cellIndex; }
    juce::uint64 getAge() const noexcept { return age; }
    TriggerMode getMode() const noexcept { return dsp.mode; }

    // extraSemis: trasposizione aggiuntiva (step del sequencer); fromSequencer: voce avviata dal sequencer
    // reverseFlip: inverte il verso di lettura rispetto alla cella; modeOverride: -1 = come la cella, altrimenti TriggerMode (0..2)
    void start (const SampleData* sample, const CellDsp& params, int cell, int noteNumber, float velocity01,
                double outputSampleRate, juce::uint64 ageStamp, float extraSemis = 0.0f, bool fromSequencer = false,
                bool reverseFlip = false, int modeOverride = -1) noexcept;
    bool isFromSequencer() const noexcept { return fromSeq; }

    // Effetti per colpo (filtro, saturazione, LoFi): stato proprio della voce + parameter lock dello step che l'ha avviata
    InsertFx insert;
    void prepareFx (double sr) { insert.prepare (sr); }
    void setLocks (std::uint32_t mask, const std::array<float, (size_t) kNumLockable>& values) noexcept { lockMask = mask; lockValues = values; }
    bool hasLock (int i) const noexcept { return (lockMask >> i) & 1u; }
    float lockValue (int i) const noexcept { return lockValues[(size_t) i]; }
    void release() noexcept;
    void kill() noexcept { active = false; }

    // Applica i parametri aggiornati (automazione, manopole) a una voce gia' in riproduzione:
    // pitch, tune, level, pan, inviluppo, regione. Non fa nulla se i valori sono invariati.
    void updateDsp (const CellDsp& params) noexcept;

    // Somma l'uscita nei buffer l / r
    void render (float* l, float* r, int numSamples) noexcept;

private:
    enum class Stage { Attack, Hold, Decay, Sustain, Release };

    void computeDerived() noexcept;     // step, guadagni, tempi dell'inviluppo dai parametri correnti
    float velocity = 1.0f;
    std::uint32_t lockMask = 0;
    std::array<float, (size_t) kNumLockable> lockValues {};
    float extraSemis = 0.0f;
    bool fromSeq = false;
    int modeOverride = -1;
    CellDsp baseDsp;                    // parametri della cella senza gli override dello step (per rilevare le variazioni)

    bool active = false;
    const SampleData* data = nullptr;
    CellDsp dsp;
    int cellIndex = -1, note = -1;
    juce::uint64 age = 0;

    double pos = 0.0, step = 1.0;
    int regionStart = 0, regionEnd = 0;
    bool forward = true;
    float gainL = 1.0f, gainR = 1.0f;

    Stage stage = Stage::Attack;
    float env = 0.0f, attackInc = 1.0f, decayDec = 0.0f, releaseDec = 0.0f, sustainLevel = 1.0f;
    int holdSamplesLeft = 0;
    double sampleRate = 44100.0;
};
}
