#include "FxChain.h"

namespace sl
{
// ---------------------------------------------------------------- InsertFx

void InsertFx::prepare (double sampleRate)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, 512, 2 };
    filter.prepare (spec);
    reset();
}

void InsertFx::reset() noexcept
{
    filter.reset();
    holdL = holdR = holdCount = 0.0f;
    paramsValid = false;
}

void InsertFx::process (float* l, float* r, int n, const InsertParams& p) noexcept
{
    if (! p.any())
        return;

    if (! paramsValid || p.filterType != applied.filterType || p.cutoff != applied.cutoff || p.resonance != applied.resonance)
    {
        filter.setType (p.filterType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                        : p.filterType == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                            : juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency (juce::jlimit (20.0f, (float) sr * 0.45f, p.cutoff));
        filter.setResonance (juce::jlimit (0.3f, 10.0f, p.resonance));
    }
    applied = p;
    paramsValid = true;

    // 1) Saturazione (tanh normalizzata in modo che il fondo scala resti ~1)
    if (p.satOn)
    {
        const float g = juce::Decibels::decibelsToGain (juce::jlimit (0.0f, 36.0f, p.drive));
        const float norm = 1.0f / std::tanh (g);
        const float mix = juce::jlimit (0.0f, 1.0f, p.satMix);
        for (int i = 0; i < n; ++i)
        {
            l[i] += mix * (std::tanh (g * l[i]) * norm - l[i]);
            r[i] += mix * (std::tanh (g * r[i]) * norm - r[i]);
        }
    }

    // 2) LoFi: riduzione di bit e di frequenza di campionamento
    if (p.lofiOn)
    {
        const float levels = std::pow (2.0f, juce::jlimit (1.0f, 16.0f, p.bits) - 1.0f);
        const float factor = juce::jlimit (1.0f, 40.0f, p.downsample);
        const float mix = juce::jlimit (0.0f, 1.0f, p.lofiMix);
        for (int i = 0; i < n; ++i)
        {
            holdCount += 1.0f;
            if (holdCount >= factor)
            {
                holdCount -= factor;
                holdL = std::round (l[i] * levels) / levels;
                holdR = std::round (r[i] * levels) / levels;
            }
            l[i] += mix * (holdL - l[i]);
            r[i] += mix * (holdR - r[i]);
        }
    }

    // 3) Filtro
    if (p.filterOn)
        for (int i = 0; i < n; ++i)
        {
            l[i] = filter.processSample (0, l[i]);
            r[i] = filter.processSample (1, r[i]);
        }
}

// ----------------------------------------------------------------- FxChain

void FxChain::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = juce::jmax (16, maxBlockSize);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    compressor.prepare (spec);
    reverb.setSampleRate (sampleRate);

    tmpL.assign ((size_t) maxBlock, 0.0f);
    tmpR.assign ((size_t) maxBlock, 0.0f);

    const int delaySize = (int) (sampleRate * (double) kMaxDelayMs / 1000.0) + 4;
    delayL.assign ((size_t) delaySize, 0.0f);
    delayR.assign ((size_t) delaySize, 0.0f);

    reset();
}

void FxChain::reset()
{
    compressor.reset();
    compValid = false;
    resetDelay();
    resetReverb();
}

void FxChain::resetDelay() noexcept
{
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    delayPos = 0;
    delayFresh = true;
}

void FxChain::resetReverb() noexcept
{
    reverb.reset();
    reverbValid = false;
}

float FxChain::delayMs (const FxParams& p, double bpm) noexcept
{
    float ms = p.delayTime;
    if (p.delaySync)
    {
        const int d = juce::jlimit (0, kNumDivisions - 1, p.delayDivision);
        ms = (float) (kDivisionBeats[d] * 60000.0 / juce::jmax (20.0, bpm));
    }
    return juce::jlimit (1.0f, kMaxDelayMs, ms);
}

void FxChain::compress (float* l, float* r, int n, const FxParams& p) noexcept
{
    if (! p.compOn || n <= 0)
        return;

    if (! compValid || p.compThreshold != appliedComp.compThreshold || p.compRatio != appliedComp.compRatio
        || p.compAttack != appliedComp.compAttack || p.compRelease != appliedComp.compRelease)
    {
        compressor.setThreshold (p.compThreshold);
        compressor.setRatio (juce::jmax (1.0f, p.compRatio));
        compressor.setAttack (juce::jmax (0.01f, p.compAttack));
        compressor.setRelease (juce::jmax (1.0f, p.compRelease));
        appliedComp = p;
        compValid = true;
    }

    float* ch[2] = { l, r };
    juce::dsp::AudioBlock<float> block (ch, 2, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    compressor.process (ctx);

    const float makeup = juce::Decibels::decibelsToGain (p.compMakeup);
    if (makeup != 1.0f)
        for (int i = 0; i < n; ++i)
        {
            l[i] *= makeup;
            r[i] *= makeup;
        }
}

void FxChain::delay (const float* sendL, const float* sendR, float* outL, float* outR, int n, const FxParams& p, double bpm) noexcept
{
    const int size = (int) delayL.size();
    const float target = delayMs (p, bpm) * (float) sr / 1000.0f;
    if (delayFresh)
    {
        delayCurrent = target;
        delayFresh = false;
    }

    const float fb = juce::jlimit (0.0f, 0.95f, p.delayFeedback);
    for (int i = 0; i < n; ++i)
    {
        // Il cambio di tempo e' smussato (effetto "tape", senza click)
        delayCurrent += (target - delayCurrent) * 0.0008f;
        float rp = (float) delayPos - delayCurrent;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp % size, i1 = (i0 + 1) % size;
        const float f = rp - std::floor (rp);
        const float dl = delayL[(size_t) i0] + f * (delayL[(size_t) i1] - delayL[(size_t) i0]);
        const float dr = delayR[(size_t) i0] + f * (delayR[(size_t) i1] - delayR[(size_t) i0]);

        delayL[(size_t) delayPos] = sendL[i] + dl * fb;
        delayR[(size_t) delayPos] = sendR[i] + dr * fb;
        delayPos = (delayPos + 1) % size;

        outL[i] += dl;
        outR[i] += dr;
    }
}

void FxChain::reverbFx (const float* sendL, const float* sendR, float* outL, float* outR, int n, const FxParams& p) noexcept
{
    if (! reverbValid || p.reverbSize != appliedReverb.reverbSize || p.reverbDamp != appliedReverb.reverbDamp)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.0f, 1.0f, p.reverbSize);
        rp.damping = juce::jlimit (0.0f, 1.0f, p.reverbDamp);
        rp.wetLevel = 1.0f;                       // solo il bagnato: il livello e' deciso dalla mandata
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        appliedReverb = p;
        reverbValid = true;
    }

    std::copy (sendL, sendL + n, tmpL.begin());
    std::copy (sendR, sendR + n, tmpR.begin());
    reverb.processStereo (tmpL.data(), tmpR.data(), n);
    for (int i = 0; i < n; ++i)
    {
        outL[i] += tmpL[(size_t) i];
        outR[i] += tmpR[(size_t) i];
    }
}
}
