#include "PluginProcessor.h"
#include "PluginEditor.h"

SampleLabProcessor::SampleLabProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true))
{
}

void SampleLabProcessor::prepareToPlay (double, int) {}

bool SampleLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SampleLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);

    // Per ora silenzio: il motore di campionamento verra' aggiunto nelle fasi successive.
    buffer.clear();
}

juce::AudioProcessorEditor* SampleLabProcessor::createEditor()
{
    return new SampleLabEditor (*this);
}

void SampleLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("SampleLabState");
    state.setProperty ("uiWidth", uiWidth, nullptr);
    state.setProperty ("uiHeight", uiHeight, nullptr);
    state.setProperty ("uiZoom", uiZoom, nullptr);

    juce::MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void SampleLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (! state.isValid())
        return;

    uiWidth  = state.getProperty ("uiWidth", uiWidth);
    uiHeight = state.getProperty ("uiHeight", uiHeight);
    uiZoom   = state.getProperty ("uiZoom", uiZoom);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SampleLabProcessor();
}
