#include "Voice.h"

namespace sl
{
namespace
{
inline float hermite (const float* d, int n, int i0, float f) noexcept
{
    auto at = [d, n] (int i) { return d[juce::jlimit (0, n - 1, i)]; };
    const float y0 = at (i0 - 1), y1 = at (i0), y2 = at (i0 + 1), y3 = at (i0 + 2);
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
}
}

void Voice::start (const SampleData* sample, const CellDsp& params, int cell, int noteNumber, float velocity01,
                   double outSR, juce::uint64 ageStamp, float extra, bool fromSequencer) noexcept
{
    extraSemis = extra;
    fromSeq = fromSequencer;
    active = false;
    if (sample == nullptr)
        return;

    data = sample;
    dsp = params;
    cellIndex = cell;
    note = noteNumber;
    age = ageStamp;
    sampleRate = outSR;
    velocity = juce::jlimit (0.0f, 1.0f, velocity01);

    computeDerived();
    if (regionEnd - regionStart < 2)
        return;

    forward = ! dsp.reverse;
    pos = forward ? (double) regionStart : (double) (regionEnd - 1);
    holdSamplesLeft = (int) ((double) dsp.hold * outSR);
    env = 0.0f;
    stage = Stage::Attack;
    active = true;
}

void Voice::computeDerived() noexcept
{
    const int n = data->numSamples();
    regionStart = juce::jlimit (0, n - 2, (int) (dsp.start * (float) n));
    regionEnd = juce::jlimit (regionStart + 2, n, (int) (dsp.end * (float) n));

    const double semis = (double) dsp.pitchSemis + (double) dsp.tuneCents / 100.0 + (double) (note - dsp.rootNote)
                         + (double) extraSemis;
    step = (data->sampleRate / sampleRate) * std::pow (2.0, semis / 12.0);

    const float g = dsp.gain * velocity;
    const float angle = (dsp.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    gainL = g * std::cos (angle) * juce::MathConstants<float>::sqrt2;
    gainR = g * std::sin (angle) * juce::MathConstants<float>::sqrt2;

    sustainLevel = dsp.sustain;
    attackInc = 1.0f / (float) juce::jmax (1.0, (double) dsp.attack * sampleRate);
    decayDec = (1.0f - sustainLevel) / (float) juce::jmax (1.0, (double) dsp.decay * sampleRate);
}

void Voice::updateDsp (const CellDsp& params) noexcept
{
    if (! active || params == dsp)
        return;

    dsp = params;
    computeDerived();
    pos = juce::jlimit ((double) regionStart, (double) (regionEnd - 1), pos);

    if (stage == Stage::Release)
        releaseDec = juce::jmin (releaseDec, env / (float) juce::jmax (1.0, (double) dsp.release * sampleRate));
}

void Voice::release() noexcept
{
    if (! active || stage == Stage::Release)
        return;

    stage = Stage::Release;
    releaseDec = env / (float) juce::jmax (1.0, (double) dsp.release * sampleRate);
}

void Voice::render (float* l, float* r, int numSamples) noexcept
{
    if (! active)
        return;

    const int n = data->numSamples();
    const float* ch0 = data->audio.getReadPointer (0);
    const float* ch1 = data->numChannels() > 1 ? data->audio.getReadPointer (1) : ch0;
    const double len = (double) (regionEnd - regionStart);
    const bool loop = dsp.mode == TriggerMode::Loop;
    constexpr float fadeLen = 64.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const int i0 = (int) pos;
        const float f = (float) (pos - (double) i0);
        const float sL = hermite (ch0, n, i0, f);
        const float sR = (ch1 == ch0) ? sL : hermite (ch1, n, i0, f);

        // Envelope
        switch (stage)
        {
            case Stage::Attack:
                env += attackInc;
                if (env >= 1.0f) { env = 1.0f; stage = holdSamplesLeft > 0 ? Stage::Hold : Stage::Decay; }
                break;
            case Stage::Hold:
                if (--holdSamplesLeft <= 0) stage = Stage::Decay;
                break;
            case Stage::Decay:
                env -= decayDec;
                if (env <= sustainLevel) { env = sustainLevel; stage = Stage::Sustain; }
                break;
            case Stage::Sustain:
                break;
            case Stage::Release:
                env -= releaseDec;
                if (env <= 0.0f) { active = false; return; }
                break;
        }

        // Dissolvenza di pochi campioni alla fine della regione (evita click) se non in loop
        float endFade = 1.0f;
        if (! loop)
        {
            const float remaining = forward ? (float) ((double) regionEnd - pos) : (float) (pos - (double) regionStart + 1.0);
            endFade = juce::jmin (1.0f, remaining / fadeLen);
        }

        const float amp = env * endFade;
        l[i] += sL * amp * gainL;
        r[i] += sR * amp * gainR;

        if (forward)
        {
            pos += step;
            if (pos >= (double) regionEnd)
            {
                if (loop) { while (pos >= (double) regionEnd) pos -= len; }
                else { active = false; return; }
            }
        }
        else
        {
            pos -= step;
            if (pos < (double) regionStart)
            {
                if (loop) { while (pos < (double) regionStart) pos += len; }
                else { active = false; return; }
            }
        }
    }
}
}
