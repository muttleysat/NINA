#include "LoopStretch.h"
#include <rubberband/RubberBandStretcher.h>
#include <cmath>

namespace sl
{
double estimateLoopBpm (const SampleData& d, float start, float end)
{
    const double seconds = (double) d.numSamples() / juce::jmax (1.0, d.sampleRate) * (double) juce::jmax (0.0f, end - start);
    if (seconds < 0.5)
        return 0.0;

    double best = 0.0, bestScore = 1.0e9;
    for (int beats = 1; beats <= 128; beats *= 2)                 // 1 beat .. 32 battute
    {
        const double bpm = (double) beats * 60.0 / seconds;
        if (bpm < 70.0 || bpm > 170.0)
            continue;
        const double score = std::abs (std::log2 (bpm / 120.0));
        if (score < bestScore)
        {
            bestScore = score;
            best = bpm;
        }
    }
    return best;
}

SampleDataPtr stretchSample (const SampleData& d, double timeRatio, double pitchScale)
{
    const int n = d.numSamples(), channels = d.numChannels();
    if (n <= 0 || channels <= 0 || timeRatio <= 0.0 || pitchScale <= 0.0)
        return nullptr;

    using RB = RubberBand::RubberBandStretcher;
    try
    {
        RB stretcher ((size_t) d.sampleRate, (size_t) channels, RB::OptionProcessOffline | RB::OptionEngineFiner | RB::OptionPitchHighQuality);
        stretcher.setTimeRatio (timeRatio);
        stretcher.setPitchScale (pitchScale);
        stretcher.setExpectedInputDuration ((size_t) n);

        std::vector<const float*> in ((size_t) channels);
        for (int ch = 0; ch < channels; ++ch)
            in[(size_t) ch] = d.audio.getReadPointer (ch);

        // Offline: prima si studia tutto il segnale, poi lo si elabora a blocchi
        constexpr int block = 4096;
        std::vector<const float*> chunk ((size_t) channels);
        for (int pos = 0; pos < n; pos += block)
        {
            const int count = juce::jmin (block, n - pos);
            for (int ch = 0; ch < channels; ++ch)
                chunk[(size_t) ch] = in[(size_t) ch] + pos;
            stretcher.study (chunk.data(), (size_t) count, pos + count >= n);
        }

        auto out = std::make_shared<SampleData>();
        const int expected = (int) std::llround ((double) n * timeRatio);
        std::vector<std::vector<float>> collected ((size_t) channels);

        auto drain = [&]
        {
            for (;;)
            {
                const int avail = stretcher.available();
                if (avail <= 0)
                    break;
                const int count = juce::jmin (avail, block);
                std::vector<std::vector<float>> tmp ((size_t) channels, std::vector<float> ((size_t) count));
                std::vector<float*> ptrs ((size_t) channels);
                for (int ch = 0; ch < channels; ++ch)
                    ptrs[(size_t) ch] = tmp[(size_t) ch].data();
                const size_t got = stretcher.retrieve (ptrs.data(), (size_t) count);
                for (int ch = 0; ch < channels; ++ch)
                    collected[(size_t) ch].insert (collected[(size_t) ch].end(), tmp[(size_t) ch].begin(), tmp[(size_t) ch].begin() + (long) got);
                if (got == 0)
                    break;
            }
        };

        for (int pos = 0; pos < n; pos += block)
        {
            const int count = juce::jmin (block, n - pos);
            for (int ch = 0; ch < channels; ++ch)
                chunk[(size_t) ch] = in[(size_t) ch] + pos;
            stretcher.process (chunk.data(), (size_t) count, pos + count >= n);
            drain();
        }
        drain();

        const int produced = (int) collected[0].size();
        const int len = juce::jmax (1, juce::jmin (produced, expected));
        out->audio.setSize (channels, len);
        for (int ch = 0; ch < channels; ++ch)
            std::copy (collected[(size_t) ch].begin(), collected[(size_t) ch].begin() + len, out->audio.getWritePointer (ch));
        out->sampleRate = d.sampleRate;
        out->file = d.file;
        out->name = d.name;
        return out;
    }
    catch (...)
    {
        return nullptr;
    }
}
}
