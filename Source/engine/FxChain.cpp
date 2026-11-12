#include "FxChain.h"

namespace sl
{
void FxChain::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = juce::jmax (16, maxBlockSize);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    filter.prepare (spec);
    compressor.prepare (spec);
    reverb.setSampleRate (sampleRate);

    const int delaySize = (int) (sampleRate * (double) kMaxDelayMs / 1000.0) + 4;
    delayL.assign ((size_t) delaySize, 0.0f);
    delayR.assign ((size_t) delaySize, 0.0f);

    reset();
}

void FxChain::reset()
{
    filter.reset();
    compressor.reset();
    reverb.reset();
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    delayPos = 0;
    holdL = holdR = holdCount = 0.0f;
    paramsValid = false;
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

void FxChain::applyParams (const FxParams& p, double bpm) noexcept
{
    if (paramsValid && p == applied)
        return;

    // Quando un effetto con memoria viene acceso si parte da uno stato pulito
    if (! paramsValid || (p.delayOn && ! applied.delayOn))
    {
        std::fill (delayL.begin(), delayL.end(), 0.0f);
        std::fill (delayR.begin(), delayR.end(), 0.0f);
        delayCurrent = delayMs (p, bpm) * (float) sr / 1000.0f;
    }
    if (! paramsValid || (p.reverbOn && ! applied.reverbOn))
        reverb.reset();
    if (! paramsValid || (p.filterOn && ! applied.filterOn))
        filter.reset();

    filter.setType (p.filterType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                    : p.filterType == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                        : juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency (juce::jlimit (20.0f, (float) sr * 0.45f, p.cutoff));
    filter.setResonance (juce::jlimit (0.3f, 10.0f, p.resonance));

    compressor.setThreshold (p.compThreshold);
    compressor.setRatio (juce::jmax (1.0f, p.compRatio));
    compressor.setAttack (juce::jmax (0.01f, p.compAttack));
    compressor.setRelease (juce::jmax (1.0f, p.compRelease));

    juce::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, p.reverbSize);
    rp.damping = juce::jlimit (0.0f, 1.0f, p.reverbDamp);
    rp.wetLevel = juce::jlimit (0.0f, 1.0f, p.reverbMix);
    rp.dryLevel = 1.0f - rp.wetLevel * 0.5f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    applied = p;
    paramsValid = true;
}

void FxChain::process (float* l, float* r, int n, const FxParams& p, double bpm) noexcept
{
    if (! p.anyOn())
    {
        paramsValid = false;            // al prossimo uso si riparte da stato pulito
        return;
    }

    for (int done = 0; done < n; done += maxBlock)
        processChunk (l + done, r + done, juce::jmin (maxBlock, n - done), p, bpm);
}

void FxChain::processChunk (float* l, float* r, int n, const FxParams& p, double bpm) noexcept
{
    applyParams (p, bpm);

    // 1) Saturazione (tanh, normalizzata in modo che il fondo scala resti ~1)
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
    {
        for (int i = 0; i < n; ++i)
        {
            l[i] = filter.processSample (0, l[i]);
            r[i] = filter.processSample (1, r[i]);
        }
    }

    // 4) Compressore
    if (p.compOn)
    {
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

    // 5) Delay
    if (p.delayOn)
    {
        const int size = (int) delayL.size();
        // In Sync il tempo segue il BPM; il cambio di tempo e' smussato (effetto "tape", senza click)
        const float target = delayMs (p, bpm) * (float) sr / 1000.0f;
        const float fb = juce::jlimit (0.0f, 0.95f, p.delayFeedback);
        const float mix = juce::jlimit (0.0f, 1.0f, p.delayMix);
        for (int i = 0; i < n; ++i)
        {
            delayCurrent += (target - delayCurrent) * 0.0008f;
            float rp = (float) delayPos - delayCurrent;
            while (rp < 0.0f) rp += (float) size;
            const int i0 = (int) rp % size, i1 = (i0 + 1) % size;
            const float f = rp - std::floor (rp);
            const float dl = delayL[(size_t) i0] + f * (delayL[(size_t) i1] - delayL[(size_t) i0]);
            const float dr = delayR[(size_t) i0] + f * (delayR[(size_t) i1] - delayR[(size_t) i0]);

            delayL[(size_t) delayPos] = l[i] + dl * fb;
            delayR[(size_t) delayPos] = r[i] + dr * fb;
            delayPos = (delayPos + 1) % size;

            l[i] += mix * dl;
            r[i] += mix * dr;
        }
    }

    // 6) Riverbero
    if (p.reverbOn)
        reverb.processStereo (l, r, n);
}
}
