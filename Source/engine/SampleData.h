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

// Carica un file audio in RAM (da chiamare fuori dal thread audio). Ritorna nullptr in caso di errore.
SampleDataPtr loadSampleFile (juce::AudioFormatManager& formats, const juce::File& file);
}
