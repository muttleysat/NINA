#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>

namespace sl
{
// Campione caricato in RAM. Immutabile dopo la creazione: condiviso tra UI e thread audio.
struct SampleData
{
    juce::AudioBuffer<float> audio;   // 1 o 2 canali
    double sampleRate = 44100.0;
    juce::File file;
    juce::String name;

    int numSamples() const noexcept { return audio.getNumSamples(); }
    int numChannels() const noexcept { return audio.getNumChannels(); }
};

using SampleDataPtr = std::shared_ptr<const SampleData>;

// Picco assoluto (0..1) del campione nella regione [start, end] (frazioni 0..1 della lunghezza)
inline float samplePeak (const SampleData& d, float start = 0.0f, float end = 1.0f)
{
    const int n = d.numSamples();
    const int a = juce::jlimit (0, n, (int) (start * (float) n));
    const int b = juce::jlimit (a, n, (int) (end * (float) n));
    float peak = 0.0f;
    for (int ch = 0; ch < d.numChannels(); ++ch)
        peak = juce::jmax (peak, d.audio.getMagnitude (ch, a, b - a));
    return peak;
}

// Guadagno in dB per portare il picco al livello target (es. -1 dBFS). Nessuna correzione per campioni silenziosi.
inline float normalizeGainDb (float peak, float targetDb = -1.0f)
{
    if (peak < 1.0e-5f)
        return 0.0f;
    return targetDb - juce::Decibels::gainToDecibels (peak);
}

// Carica un file audio in RAM (da chiamare fuori dal thread audio). Ritorna nullptr in caso di errore.
SampleDataPtr loadSampleFile (juce::AudioFormatManager& formats, const juce::File& file);
}
