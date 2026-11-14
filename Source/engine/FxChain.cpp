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
    for (auto& b : eqBand)
        b.z1[0] = b.z1[1] = b.z2[0] = b.z2[1] = 0.0;
    eqValid = false;
    eqWasOn = false;
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

namespace
{
// Coefficienti RBJ (Audio EQ Cookbook), normalizzati su a0
struct Coef { double b0, b1, b2, a1, a2; };

Coef normalise (double b0, double b1, double b2, double a0, double a1, double a2)
{
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

Coef lowShelf (double sr, double f, double dB)
{
    const double A = std::pow (10.0, dB / 40.0), w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
    const double c = std::cos (w0), alpha = std::sin (w0) / 2.0 * std::sqrt (2.0), sq = 2.0 * std::sqrt (A) * alpha;
    return normalise (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                      (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
}

Coef highShelf (double sr, double f, double dB)
{
    const double A = std::pow (10.0, dB / 40.0), w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
    const double c = std::cos (w0), alpha = std::sin (w0) / 2.0 * std::sqrt (2.0), sq = 2.0 * std::sqrt (A) * alpha;
    return normalise (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                      (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
}

Coef peak (double sr, double f, double dB, double q)
{
    const double A = std::pow (10.0, dB / 40.0), w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
    const double c = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
    return normalise (1 + alpha * A, -2 * c, 1 - alpha * A, 1 + alpha / A, -2 * c, 1 - alpha / A);
}
}

void FxChain::eq (float* l, float* r, int n, const FxParams& p) noexcept
{
    if (! p.eqOn || n <= 0)
    {
        eqWasOn = false;
        return;
    }

    if (! eqWasOn)                                              // appena acceso: si riparte da stati puliti
    {
        for (auto& b : eqBand)
            b.z1[0] = b.z1[1] = b.z2[0] = b.z2[1] = 0.0;
        eqValid = false;
    }
    eqWasOn = true;

    if (! eqValid || p.eqLow != appliedEq.eqLow || p.eqMidFreq != appliedEq.eqMidFreq || p.eqMidGain != appliedEq.eqMidGain
        || p.eqMidQ != appliedEq.eqMidQ || p.eqHigh != appliedEq.eqHigh)
    {
        const double nyq = sr * 0.45;
        auto set = [this] (int i, const Coef& c) { eqBand[i].b0 = c.b0; eqBand[i].b1 = c.b1; eqBand[i].b2 = c.b2; eqBand[i].a1 = c.a1; eqBand[i].a2 = c.a2; };
        eqBandOn[0] = std::abs (p.eqLow) > 0.05f;
        eqBandOn[1] = std::abs (p.eqMidGain) > 0.05f;
        eqBandOn[2] = std::abs (p.eqHigh) > 0.05f;
        if (eqBandOn[0]) set (0, lowShelf (sr, std::min (120.0, nyq), juce::jlimit (-15.0f, 15.0f, p.eqLow)));
        if (eqBandOn[1]) set (1, peak (sr, juce::jlimit (100.0, nyq, (double) p.eqMidFreq), juce::jlimit (-15.0f, 15.0f, p.eqMidGain),
                                       juce::jlimit (0.3, 8.0, (double) p.eqMidQ)));
        if (eqBandOn[2]) set (2, highShelf (sr, std::min (6000.0, nyq), juce::jlimit (-15.0f, 15.0f, p.eqHigh)));
        appliedEq = p;
        eqValid = true;
    }

    float* ch[2] = { l, r };
    for (int b = 0; b < 3; ++b)
    {
        if (! eqBandOn[b])
            continue;
        auto& q = eqBand[b];
        for (int c = 0; c < 2; ++c)
        {
            float* x = ch[c];
            double z1 = q.z1[c], z2 = q.z2[c];
            for (int i = 0; i < n; ++i)                         // forma trasposta II
            {
                const double in = (double) x[i];
                const double out = q.b0 * in + z1;
                z1 = q.b1 * in - q.a1 * out + z2;
                z2 = q.b2 * in - q.a2 * out;
                x[i] = (float) out;
            }
            q.z1[c] = z1;
            q.z2[c] = z2;
        }
    }
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
