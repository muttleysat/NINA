#pragma once

namespace sl
{
// Valori musicali per gli effetti sincronizzati al tempo (durata in quarti: 1/4 = 1 beat)
inline constexpr int kNumDivisions = 17;
inline constexpr const char* kDivisionNames[kNumDivisions] = {
    "1/1", "1/2", "1/2.", "1/2T", "1/4", "1/4.", "1/4T", "1/8", "1/8.", "1/8T",
    "1/16", "1/16.", "1/16T", "1/32", "1/32.", "1/32T", "1/64" };
// In double: nei sequencer lunghi un float accumula deriva nelle terzine (es. 1/6 di beat)
inline constexpr double kDivisionBeats[kNumDivisions] = {
    4.0, 2.0, 3.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0, 0.5, 0.75, 1.0 / 3.0,
    0.25, 0.375, 1.0 / 6.0, 0.125, 0.1875, 1.0 / 12.0, 0.0625 };

// Parametri degli effetti (float), uguali per ogni cella e per il master
enum class FxF { Drive, SatMix, Bits, Downsample, LofiMix, Cutoff, Resonance,
                 CompThreshold, CompRatio, CompAttack, CompRelease, CompMakeup,
                 DelayTime, DelayFeedback, DelayMix, ReverbSize, ReverbDamp, ReverbMix, Count };
// Interruttori on/off degli effetti
enum class FxB { Sat, Lofi, Filter, Comp, Delay, Reverb, Count };

// Parametri degli effetti di una cella o del kit (master). Struttura semplice (nessuna stringa),
// copiabile nel thread audio. Ordine della catena: Saturation > LoFi > Filter > Compressor > Delay > Reverb.
struct FxParams
{
    // Saturazione
    bool satOn = false;
    float drive = 6.0f;            // dB, 0..36
    float satMix = 1.0f;           // 0..1

    // LoFi (bit crusher + riduzione frequenza di campionamento)
    bool lofiOn = false;
    float bits = 12.0f;            // 1..16
    float downsample = 4.0f;       // fattore 1..40
    float lofiMix = 1.0f;

    // Filtro
    bool filterOn = false;
    int filterType = 0;            // 0 passa-basso, 1 passa-alto, 2 passa-banda
    float cutoff = 8000.0f;        // Hz, 20..20000
    float resonance = 0.707f;      // Q, 0.3..10

    // Compressore
    bool compOn = false;
    float compThreshold = -18.0f;  // dB
    float compRatio = 4.0f;
    float compAttack = 10.0f;      // ms
    float compRelease = 120.0f;    // ms
    float compMakeup = 0.0f;       // dB

    // Delay
    bool delayOn = false;
    bool delaySync = false;        // true: il tempo segue il BPM (valore musicale), false: tempo libero in ms
    int delayDivision = 7;         // indice in kDivisionNames (default 1/8)
    float delayTime = 250.0f;      // ms, 1..2000 (usato quando il sync e' spento)
    float delayFeedback = 0.35f;   // 0..0.95
    float delayMix = 0.3f;

    // Riverbero
    bool reverbOn = false;
    float reverbSize = 0.5f;
    float reverbDamp = 0.5f;
    float reverbMix = 0.25f;

    bool anyOn() const noexcept { return satOn || lofiOn || filterOn || compOn || delayOn || reverbOn; }
    bool hasTail() const noexcept { return delayOn || reverbOn; }

    bool operator== (const FxParams&) const = default;
};

// Accesso per enumerazione (usato dai parameter lock del sequencer)
inline void setFxFloat (FxParams& p, FxF f, float v) noexcept
{
    switch (f)
    {
        case FxF::Drive: p.drive = v; break;
        case FxF::SatMix: p.satMix = v; break;
        case FxF::Bits: p.bits = v; break;
        case FxF::Downsample: p.downsample = v; break;
        case FxF::LofiMix: p.lofiMix = v; break;
        case FxF::Cutoff: p.cutoff = v; break;
        case FxF::Resonance: p.resonance = v; break;
        case FxF::CompThreshold: p.compThreshold = v; break;
        case FxF::CompRatio: p.compRatio = v; break;
        case FxF::CompAttack: p.compAttack = v; break;
        case FxF::CompRelease: p.compRelease = v; break;
        case FxF::CompMakeup: p.compMakeup = v; break;
        case FxF::DelayTime: p.delayTime = v; break;
        case FxF::DelayFeedback: p.delayFeedback = v; break;
        case FxF::DelayMix: p.delayMix = v; break;
        case FxF::ReverbSize: p.reverbSize = v; break;
        case FxF::ReverbDamp: p.reverbDamp = v; break;
        case FxF::ReverbMix: p.reverbMix = v; break;
        case FxF::Count: break;
    }
}

inline void setFxBool (FxParams& p, FxB b, bool v) noexcept
{
    switch (b)
    {
        case FxB::Sat: p.satOn = v; break;
        case FxB::Lofi: p.lofiOn = v; break;
        case FxB::Filter: p.filterOn = v; break;
        case FxB::Comp: p.compOn = v; break;
        case FxB::Delay: p.delayOn = v; break;
        case FxB::Reverb: p.reverbOn = v; break;
        case FxB::Count: break;
    }
}

// Effetto a cui appartiene ogni parametro float (bloccare un parametro accende l'effetto per quello step)
inline constexpr FxB fxOwner (FxF f) noexcept
{
    switch (f)
    {
        case FxF::Drive: case FxF::SatMix: return FxB::Sat;
        case FxF::Bits: case FxF::Downsample: case FxF::LofiMix: return FxB::Lofi;
        case FxF::Cutoff: case FxF::Resonance: return FxB::Filter;
        case FxF::CompThreshold: case FxF::CompRatio: case FxF::CompAttack: case FxF::CompRelease: case FxF::CompMakeup: return FxB::Comp;
        case FxF::DelayTime: case FxF::DelayFeedback: case FxF::DelayMix: return FxB::Delay;
        case FxF::ReverbSize: case FxF::ReverbDamp: case FxF::ReverbMix: return FxB::Reverb;
        case FxF::Count: break;
    }
    return FxB::Sat;
}

// Parametri che un step puo' bloccare "per colpo" (solo quel colpo cambia): sono quelli degli effetti per nota (saturazione, LoFi,
// filtro) e le mandate (send) al delay e al riverbero. Compressore, tempo/feedback del delay e dimensione del riverbero sono
// della cella: condivisi da tutte le note, non hanno senso per un singolo colpo.
inline constexpr bool isPerHitLockable (FxF f) noexcept
{
    switch (f)
    {
        case FxF::Drive: case FxF::SatMix: case FxF::Bits: case FxF::Downsample: case FxF::LofiMix:
        case FxF::Cutoff: case FxF::Resonance: case FxF::DelayMix: case FxF::ReverbMix:
            return true;
        default:
            return false;
    }
}

// Parametri "bloccabili" dagli step del sequencer: prima i 18 valori degli effetti (FxF), poi i 6 interruttori on/off (FxB)
inline constexpr int kNumLockable = (int) FxF::Count + (int) FxB::Count;
inline constexpr int lockIndex (FxB b) noexcept { return (int) FxF::Count + (int) b; }

inline float getFxFloat (const FxParams& p, FxF f) noexcept
{
    switch (f)
    {
        case FxF::Drive: return p.drive;
        case FxF::SatMix: return p.satMix;
        case FxF::Bits: return p.bits;
        case FxF::Downsample: return p.downsample;
        case FxF::LofiMix: return p.lofiMix;
        case FxF::Cutoff: return p.cutoff;
        case FxF::Resonance: return p.resonance;
        case FxF::CompThreshold: return p.compThreshold;
        case FxF::CompRatio: return p.compRatio;
        case FxF::CompAttack: return p.compAttack;
        case FxF::CompRelease: return p.compRelease;
        case FxF::CompMakeup: return p.compMakeup;
        case FxF::DelayTime: return p.delayTime;
        case FxF::DelayFeedback: return p.delayFeedback;
        case FxF::DelayMix: return p.delayMix;
        case FxF::ReverbSize: return p.reverbSize;
        case FxF::ReverbDamp: return p.reverbDamp;
        case FxF::ReverbMix: return p.reverbMix;
        case FxF::Count: break;
    }
    return 0.0f;
}
}
