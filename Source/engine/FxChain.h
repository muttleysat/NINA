#pragma once

#include "FxParams.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace sl
{
// Effetti "per colpo": filtro, saturazione e LoFi. Ogni voce ne ha una copia con il suo stato, quindi un lock di uno step
// cambia solo quel colpo e non tocca le note gia' in corso.
struct InsertParams
{
    bool satOn = false;
    float drive = 6.0f, satMix = 1.0f;
    bool lofiOn = false;
    float bits = 12.0f, downsample = 4.0f, lofiMix = 1.0f;
    bool filterOn = false;
    int filterType = 0;
    float cutoff = 8000.0f, resonance = 0.707f;

    bool any() const noexcept { return satOn || lofiOn || filterOn; }
    bool operator== (const InsertParams&) const = default;

    static InsertParams from (const FxParams& f) noexcept
    {
        InsertParams p;
        p.satOn = f.satOn; p.drive = f.drive; p.satMix = f.satMix;
        p.lofiOn = f.lofiOn; p.bits = f.bits; p.downsample = f.downsample; p.lofiMix = f.lofiMix;
        p.filterOn = f.filterOn; p.filterType = f.filterType; p.cutoff = f.cutoff; p.resonance = f.resonance;
        return p;
    }
};

class InsertFx
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;                                        // da chiamare all'inizio di ogni nota

    // Elabora in place. Nessuna allocazione.
    void process (float* l, float* r, int n, const InsertParams& p) noexcept;

private:
    double sr = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> filter;
    InsertParams applied;
    bool paramsValid = false;
    float holdL = 0.0f, holdR = 0.0f, holdCount = 0.0f;
};

// Effetti "di cella" (e del master): compressore sul bus, delay e riverbero come mandate.
// Il delay e il riverbero ricevono un segnale di mandata (send) e AGGIUNGONO l'uscita bagnata al bus: un colpo mandato
// all'effetto continua a risuonare anche dopo la fine della nota o dello step.
class FxChain
{
public:
    static constexpr float kMaxDelayMs = 2000.0f;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void resetDelay() noexcept;
    void resetReverb() noexcept;

    // Compressore in place sul bus (se compOn). n <= maxBlockSize.
    void compress (float* l, float* r, int n, const FxParams& p) noexcept;

    // Delay: sendL/R e' il segnale mandato; l'uscita bagnata viene SOMMATA a outL/outR. bpm per il sync.
    void delay (const float* sendL, const float* sendR, float* outL, float* outR, int n, const FxParams& p, double bpm) noexcept;

    // Riverbero: come sopra (solo uscita bagnata sommata a out).
    void reverbFx (const float* sendL, const float* sendR, float* outL, float* outR, int n, const FxParams& p) noexcept;

    // Tempo del delay in ms secondo i parametri e il BPM (limitato alla lunghezza massima)
    static float delayMs (const FxParams& p, double bpm) noexcept;

private:
    double sr = 44100.0;
    int maxBlock = 512;

    juce::dsp::Compressor<float> compressor;
    FxParams appliedComp;
    bool compValid = false;

    juce::Reverb reverb;
    FxParams appliedReverb;
    bool reverbValid = false;
    std::vector<float> tmpL, tmpR;

    std::vector<float> delayL, delayR;
    int delayPos = 0;
    float delayCurrent = 0.0f;
    bool delayFresh = true;
};
}
