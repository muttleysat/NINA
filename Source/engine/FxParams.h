#pragma once

namespace sl
{
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
    float delayTime = 250.0f;      // ms, 1..750
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
