#pragma once

namespace sl
{
// Valori musicali per gli effetti sincronizzati al tempo (durata in quarti: 1/4 = 1 beat)
inline constexpr int kNumDivisions = 17;
inline constexpr const char* kDivisionNames[kNumDivisions] = {
    "1/1", "1/2", "1/2.", "1/2T", "1/4", "1/4.", "1/4T", "1/8", "1/8.", "1/8T",
    "1/16", "1/16.", "1/16T", "1/32", "1/32.", "1/32T", "1/64" };
inline constexpr float kDivisionBeats[kNumDivisions] = {
    4.0f, 2.0f, 3.0f, 4.0f / 3.0f, 1.0f, 1.5f, 2.0f / 3.0f, 0.5f, 0.75f, 1.0f / 3.0f,
    0.25f, 0.375f, 1.0f / 6.0f, 0.125f, 0.1875f, 1.0f / 12.0f, 0.0625f };

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
}
