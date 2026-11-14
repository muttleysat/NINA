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

    // EQ a tre bande in place sul bus (se eqOn): shelf basso 120 Hz, campana centrale, shelf alto 6 kHz. Nessuna allocazione.
    void eq (float* l, float* r, int n, const FxParams& p) noexcept;

    // Risposta in dB dell'EQ alla frequenza 'freq' (per disegnare la curva)
    static float eqResponseDb (const FxParams& p, double freq, double sampleRate = 44100.0) noexcept;

    // Stutter (beat repeat) in place, da chiamare alla fine della catena. Registra sempre il segnale in un anello. Da acceso:
    //  - appena acceso, se nella storia recente c'e' suono (un loop in corso) ripete a ciclo l'ultima porzione;
    //  - a ogni nuovo colpo della cella ('onset') registra la porzione che parte da li' (intanto il colpo si sente normalmente) e poi
    //    la ripete a ciclo, ogni volta piu' piano secondo Decay, finche' non arriva un altro colpo o si spegne.
    // La porzione dura quanto la divisione musicale (con il BPM dato). Nessuna allocazione.
    void stutter (float* l, float* r, int n, const FxParams& p, double bpm, bool onset = false) noexcept;

    // Effetti di modulazione in place sul bus (se acceso): chorus (linea di ritardo modulata), flanger (ritardo brevissimo con feedback),
    // phaser (4 filtri passa-tutto modulati con feedback). Ognuno ha il suo LFO, con i due canali sfasati di 90 gradi per l'ampiezza stereo.
    void chorus (float* l, float* r, int n, const FxParams& p) noexcept;
    void flanger (float* l, float* r, int n, const FxParams& p) noexcept;
    void phaser (float* l, float* r, int n, const FxParams& p) noexcept;

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

    // EQ: tre biquad (RBJ) per canale; i coefficienti si ricalcolano solo quando cambiano i parametri
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1[2] { 0.0, 0.0 }, z2[2] { 0.0, 0.0 };
    };
    Biquad eqBand[3];
    bool eqBandOn[3] { false, false, false };
    FxParams appliedEq;
    bool eqValid = false, eqWasOn = false;

    // Stutter: anello della storia recente e porzione "congelata" che si ripete
    std::vector<float> stRingL, stRingR, stFrozenL, stFrozenR;
    int stCap = 0, stW = 0, stLen = 0, stPos = 0;
    enum class StState { Off, Armed, Capturing, Repeating };
    StState stState = StState::Off;
    int stCapturePos = 0;
    float stGain = 1.0f;

    // Linee di ritardo del chorus e del flanger (2 canali), stato degli LFO e del phaser
    std::vector<float> chLine[2], flLine[2];
    int chW = 0, flW = 0;
    double chPhase = 0.0, flPhase = 0.0, phPhase = 0.0;
    float flFb[2] { 0.0f, 0.0f }, phFb[2] { 0.0f, 0.0f }, apZ[2][4] {};
    bool chWasOn = false, flWasOn = false, phWasOn = false;

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
