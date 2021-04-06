#include "ronn.h"

void ronn::addParameters (Parameters& params)
{
}

void ronn::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void ronn::releaseResources()
{
}

void ronn::processAudioBlock (AudioBuffer<float>& buffer)
{
}

AudioProcessorEditor* ronn::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ronn();
}
