#include "FxChain.h"
#include <complex>

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

    const int modSize = (int) (sampleRate * 0.04) + 8;               // 40 ms bastano per chorus e flanger
    for (int ch = 0; ch < 2; ++ch)
    {
        chLine[ch].assign ((size_t) modSize, 0.0f);
        flLine[ch].assign ((size_t) modSize, 0.0f);
    }

    stCap = (int) (sampleRate * 0.75) + 8;                      // lo stutter ripete al massimo 0.75 s
    stRingL.assign ((size_t) stCap, 0.0f);
    stRingR.assign ((size_t) stCap, 0.0f);
    stFrozenL.assign ((size_t) stCap, 0.0f);
    stFrozenR.assign ((size_t) stCap, 0.0f);

    reset();
}

void FxChain::reset()
{
    for (auto& b : eqBand)
        b.z1[0] = b.z1[1] = b.z2[0] = b.z2[1] = 0.0;
    eqValid = false;
    eqWasOn = false;
    chWasOn = flWasOn = phWasOn = false;
    chW = flW = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill (chLine[ch].begin(), chLine[ch].end(), 0.0f);
        std::fill (flLine[ch].begin(), flLine[ch].end(), 0.0f);
        flFb[ch] = phFb[ch] = 0.0f;
        for (auto& z : apZ[ch]) z = 0.0f;
    }
    stState = StState::Off;
    stPos = stLen = 0;
    std::fill (stRingL.begin(), stRingL.end(), 0.0f);
    std::fill (stRingR.begin(), stRingR.end(), 0.0f);
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

float FxChain::eqResponseDb (const FxParams& p, double freq, double sr) noexcept
{
    const double nyq = sr * 0.45;
    const double w = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (1.0, nyq, freq) / sr;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
    double db = 0.0;
    auto add = [&] (const Coef& c)
    {
        const auto num = c.b0 + c.b1 * z1 + c.b2 * z2;
        const auto den = 1.0 + c.a1 * z1 + c.a2 * z2;
        db += 20.0 * std::log10 (std::max (1.0e-9, std::abs (num / den)));
    };
    if (std::abs (p.eqLow) > 0.05f) add (lowShelf (sr, std::min (120.0, nyq), juce::jlimit (-15.0f, 15.0f, p.eqLow)));
    if (std::abs (p.eqMidGain) > 0.05f)
        add (peak (sr, juce::jlimit (100.0, nyq, (double) p.eqMidFreq), juce::jlimit (-15.0f, 15.0f, p.eqMidGain), juce::jlimit (0.3, 8.0, (double) p.eqMidQ)));
    if (std::abs (p.eqHigh) > 0.05f) add (highShelf (sr, std::min (6000.0, nyq), juce::jlimit (-15.0f, 15.0f, p.eqHigh)));
    return (float) db;
}

void FxChain::stutter (float* l, float* r, int n, const FxParams& p, double bpm, bool onset) noexcept
{
    if (stCap <= 0 || n <= 0)
        return;

    const int cap = stCap;
    const int desired = juce::jlimit (64, cap - 8, (int) std::lround (kDivisionBeats[juce::jlimit (0, kNumDivisions - 1, p.stutterDivision)]
                                                                       * 60.0 / juce::jmax (20.0, bpm) * sr));
    const float mix = juce::jlimit (0.0f, 1.0f, p.stutterMix);
    const float keep = 1.0f - juce::jlimit (0.0f, 0.95f, p.stutterDecay);
    const int fade = juce::jmax (8, (int) (0.002 * sr));                 // 2 ms di dissolvenza ai bordi: niente scatti

    // Congela le ultime 'len' campioni dell'anello (compreso l'ultimo scritto); ritorna il picco della porzione
    auto captureHistory = [&] (int len)
    {
        int src = stW - len + 1;
        if (src < 0)
            src += cap;
        float pk = 0.0f;
        for (int k = 0; k < len; ++k)
        {
            stFrozenL[(size_t) k] = stRingL[(size_t) src];
            stFrozenR[(size_t) k] = stRingR[(size_t) src];
            pk = juce::jmax (pk, std::abs (stFrozenL[(size_t) k]), std::abs (stFrozenR[(size_t) k]));
            if (++src == cap)
                src = 0;
        }
        stLen = len;
        stPos = 0;
        return pk;
    };

    if (! p.stutterOn)
    {
        stState = StState::Off;
        for (int i = 0; i < n; ++i)                                      // anche da spento si registra la storia
        {
            stRingL[(size_t) stW] = l[i];
            stRingR[(size_t) stW] = r[i];
            if (++stW == cap)
                stW = 0;
        }
        return;
    }

    if (onset)                                                          // nuovo colpo: da qui si registra la porzione da ripetere
    {
        stState = StState::Capturing;
        stLen = desired;
        stCapturePos = 0;
    }

    for (int i = 0; i < n; ++i)
    {
        const float dl = l[i], dr = r[i];
        stRingL[(size_t) stW] = dl;
        stRingR[(size_t) stW] = dr;

        if (stState == StState::Off)                                    // appena acceso
        {
            // Se c'e' suono nella storia recente (loop in corso) si ripete quello, altrimenti si aspetta il prossimo colpo
            if (captureHistory (desired) > 1.0e-3f)
            {
                stGain = 1.0f;
                stState = StState::Repeating;
            }
            else
                stState = StState::Armed;
        }

        if (stState == StState::Capturing)
        {
            stFrozenL[(size_t) stCapturePos] = dl;                      // il colpo suona normalmente mentre si registra
            stFrozenR[(size_t) stCapturePos] = dr;
            if (++stCapturePos >= stLen)
            {
                stState = StState::Repeating;
                stPos = 0;
                stGain = 1.0f;
            }
        }
        else if (stState == StState::Repeating)
        {
            const float e = juce::jmin (1.0f, (float) juce::jmin (stPos, stLen - 1 - stPos) / (float) fade);
            const float g = stGain * e;
            l[i] = dl * (1.0f - mix) + stFrozenL[(size_t) stPos] * g * mix;
            r[i] = dr * (1.0f - mix) + stFrozenR[(size_t) stPos] * g * mix;

            if (++stPos >= stLen)
            {
                stGain *= keep;
                if (desired != stLen)                                    // cambio di divisione o di tempo: si ricatta la storia
                    captureHistory (desired);
                else
                    stPos = 0;
            }
        }

        if (++stW == cap)
            stW = 0;
    }
}

namespace
{
// Lettura con interpolazione lineare 'delay' campioni indietro rispetto alla posizione di scrittura 'w'
inline float readLine (const std::vector<float>& b, int w, float delay) noexcept
{
    const int size = (int) b.size();
    float rp = (float) w - delay;
    while (rp < 0.0f)
        rp += (float) size;
    int i0 = (int) rp;
    const float f = rp - (float) i0;
    if (i0 >= size)
        i0 -= size;
    int i1 = i0 + 1;
    if (i1 >= size)
        i1 -= size;
    return b[(size_t) i0] * (1.0f - f) + b[(size_t) i1] * f;
}
constexpr double kTwoPi = juce::MathConstants<double>::twoPi;
}

void FxChain::chorus (float* l, float* r, int n, const FxParams& p) noexcept
{
    if (! p.chorusOn || n <= 0)
    {
        chWasOn = false;
        return;
    }
    if (! chWasOn)
    {
        for (int ch = 0; ch < 2; ++ch)
            std::fill (chLine[ch].begin(), chLine[ch].end(), 0.0f);
        chWasOn = true;
    }

    const double inc = (double) juce::jlimit (0.05f, 5.0f, p.chorusRate) / sr;
    const float depthMs = juce::jlimit (0.0f, 1.0f, p.chorusDepth) * 6.0f;
    const float mix = juce::jlimit (0.0f, 1.0f, p.chorusMix);
    const float dryG = 1.0f - 0.5f * mix, wetG = 0.5f * mix;
    const int size = (int) chLine[0].size();
    float* io[2] = { l, r };

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const float dry = io[ch][i];
            chLine[ch][(size_t) chW] = dry;
            const float lfo = (float) std::sin (kTwoPi * (chPhase + 0.25 * ch));
            const float d = (15.0f + depthMs * lfo) * (float) sr * 0.001f;
            const float wet = readLine (chLine[ch], chW, d);
            io[ch][i] = dry * dryG + wet * wetG;
        }
        if (++chW == size)
            chW = 0;
        chPhase += inc;
        if (chPhase >= 1.0)
            chPhase -= 1.0;
    }
}

void FxChain::flanger (float* l, float* r, int n, const FxParams& p) noexcept
{
    if (! p.flangerOn || n <= 0)
    {
        flWasOn = false;
        return;
    }
    if (! flWasOn)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            std::fill (flLine[ch].begin(), flLine[ch].end(), 0.0f);
            flFb[ch] = 0.0f;
        }
        flWasOn = true;
    }

    const double inc = (double) juce::jlimit (0.05f, 5.0f, p.flangerRate) / sr;
    const float depth = juce::jlimit (0.0f, 1.0f, p.flangerDepth);
    const float fb = juce::jlimit (0.0f, 0.9f, p.flangerFeedback);
    const float mix = juce::jlimit (0.0f, 1.0f, p.flangerMix);
    const float dryG = 1.0f - 0.5f * mix, wetG = 0.5f * mix;
    const int size = (int) flLine[0].size();
    float* io[2] = { l, r };

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const float dry = io[ch][i];
            flLine[ch][(size_t) flW] = dry + fb * flFb[ch];                 // il feedback rientra nella linea
            const float lfo = (float) std::sin (kTwoPi * (flPhase + 0.25 * ch));
            const float d = (0.2f + depth * 2.6f * (0.5f + 0.5f * lfo)) * (float) sr * 0.001f;       // 0.2 .. 2.8 ms
            const float wet = readLine (flLine[ch], flW, d);
            flFb[ch] = wet;
            io[ch][i] = dry * dryG + wet * wetG;
        }
        if (++flW == size)
            flW = 0;
        flPhase += inc;
        if (flPhase >= 1.0)
            flPhase -= 1.0;
    }
}

void FxChain::phaser (float* l, float* r, int n, const FxParams& p) noexcept
{
    if (! p.phaserOn || n <= 0)
    {
        phWasOn = false;
        return;
    }
    if (! phWasOn)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            phFb[ch] = 0.0f;
            for (auto& z : apZ[ch]) z = 0.0f;
        }
        phWasOn = true;
    }

    const double inc = (double) juce::jlimit (0.05f, 5.0f, p.phaserRate) / sr;
    const float depth = juce::jlimit (0.0f, 1.0f, p.phaserDepth);
    const float fb = juce::jlimit (0.0f, 0.9f, p.phaserFeedback);
    const float mix = juce::jlimit (0.0f, 1.0f, p.phaserMix);
    const float dryG = 1.0f - 0.5f * mix, wetG = 0.5f * mix;
    const float fMax = (float) std::min (sr * 0.45, 4000.0);
    float* io[2] = { l, r };

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const float dry = io[ch][i];
            const float lfo = (float) std::sin (kTwoPi * (phPhase + 0.25 * ch));
            const float f = 200.0f * std::pow (fMax / 200.0f, 0.5f + 0.5f * depth * lfo);            // 200 Hz .. 4 kHz, esponenziale
            const float t = std::tan (juce::MathConstants<float>::pi * f / (float) sr);
            const float a = (t - 1.0f) / (t + 1.0f);

            float x = dry + fb * phFb[ch];
            for (int st = 0; st < 4; ++st)                                  // quattro filtri passa-tutto del primo ordine
            {
                const float y = a * x + apZ[ch][st];
                apZ[ch][st] = x - a * y;
                x = y;
            }
            phFb[ch] = x;
            io[ch][i] = dry * dryG + x * wetG;
        }
        phPhase += inc;
        if (phPhase >= 1.0)
            phPhase -= 1.0;
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
