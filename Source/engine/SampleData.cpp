#include "SampleData.h"

namespace sl
{
SampleDataPtr loadSampleFile (juce::AudioFormatManager& formats, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 2 || reader->numChannels < 1)
        return nullptr;

    auto data = std::make_shared<SampleData>();
    const int channels = (int) juce::jmin (2u, reader->numChannels);
    data->audio.setSize (channels, (int) reader->lengthInSamples);

    if (! reader->read (&data->audio, 0, (int) reader->lengthInSamples, 0, true, channels > 1))
        return nullptr;

    data->sampleRate = reader->sampleRate;
    data->file = file;
    data->name = file.getFileNameWithoutExtension();
    return data;
}
}
