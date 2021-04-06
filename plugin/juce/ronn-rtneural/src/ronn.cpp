#include "ronn.h"
#include "PluginEditor.h"

void ronn::addParameters (Parameters& params)
{
    params.push_back (std::make_unique<AudioParameterInt>   ("layers", "Layers", 1, 24, 6));
    params.push_back (std::make_unique<AudioParameterInt>   ("kernel", "Kernel Width", 1, 64, 3));
    params.push_back (std::make_unique<AudioParameterInt>   ("channels", "Channels", 1, 64, 8));
    params.push_back (std::make_unique<AudioParameterFloat> ("inputGain", "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<AudioParameterFloat> ("outputGain", "Output Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<AudioParameterBool>  ("useBias", "Use Bias", false));
    params.push_back (std::make_unique<AudioParameterInt>   ("activation", "Activation", 1, 10, 1));
    params.push_back (std::make_unique<AudioParameterInt>   ("dilation", "Dilation Factor", 1, 4, 1));
    params.push_back (std::make_unique<AudioParameterInt>   ("initType", "Init Type", 1, 6, 1));
    params.push_back (std::make_unique<AudioParameterInt>   ("seed", "Seed", 0, 1024, 42));
    params.push_back (std::make_unique<AudioParameterBool>  ("linkGain", "Link", false));
    params.push_back (std::make_unique<AudioParameterBool>  ("depthwise", "Depthwise", false));
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
    return new RonnAudioProcessorEditor (*this, vts);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ronn();
}
