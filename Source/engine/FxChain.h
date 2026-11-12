#pragma once

#include "FxParams.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace sl
{
// Catena di effetti stereo con stato interno. Usata per ogni cella e per il master.
// prepare() alloca (thread non audio); process() non alloca.
class FxChain
{
public:
    static constexpr float kMaxDelayMs = 2000.0f;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Elabora in place. n puo' superare maxBlockSize: viene suddiviso internamente.
    // bpm: tempo corrente (DAW o interno), usato dagli effetti in modalita' Sync.
    void process (float* l, float* r, int n, const FxParams& p, double bpm) noexcept;

    // Tempo del delay in ms secondo i parametri e il BPM (limitato alla lunghezza massima)
    static float delayMs (const FxParams& p, double bpm) noexcept;

private:
    void processChunk (float* l, float* r, int n, const FxParams& p, double bpm) noexcept;
    void applyParams (const FxParams& p, double bpm) noexcept;

    double sr = 44100.0;
    int maxBlock = 512;
    FxParams applied;
    bool paramsValid = false;

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::Compressor<float> compressor;
    juce::Reverb reverb;

    // LoFi
    float holdL = 0.0f, holdR = 0.0f, holdCount = 0.0f;

    // Delay (linea con interpolazione lineare e tempo smussato)
    std::vector<float> delayL, delayR;
    int delayPos = 0;
    float delayCurrent = 0.0f;
};
}
