#include "ronn.h"
#include "PluginEditor.h"

ronn::ronn()
{
    layersParameter     = vts.getRawParameterValue ("layers");
    kernelParameter     = vts.getRawParameterValue ("kernel");
    channelsParameter   = vts.getRawParameterValue ("channels");
    inputGainParameter  = vts.getRawParameterValue ("inputGain");
    outputGainParameter = vts.getRawParameterValue ("outputGain");
    useBiasParameter    = vts.getRawParameterValue ("useBias");
    activationParameter = vts.getRawParameterValue ("activation");
    dilationParameter   = vts.getRawParameterValue ("dilation");
    initTypeParameter   = vts.getRawParameterValue ("initType");
    linkGainParameter   = vts.getRawParameterValue ("linkGain");
    seedParameter       = vts.getRawParameterValue ("seed");
    depthwiseParameter  = vts.getRawParameterValue ("depthwise");

    vts.addParameterListener ("inputGain", this);
    vts.addParameterListener ("outputGain", this);
    vts.addParameterListener ("linkGain", this);
}

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

void ronn::parameterChanged (const String& paramID, float newValue)
{
    auto linkOutputGain = [=] {
        auto outGainParam = vts.getParameter ("outputGain");
        auto linkedValue = outGainParam->convertTo0to1 (-1.0f * inputGainParameter->load());
        outGainParam->setValueNotifyingHost (linkedValue);
    };

    auto linkInputGain = [=] {
        auto inGainParam = vts.getParameter ("inputGain");
        auto linkedValue = inGainParam->convertTo0to1 (-1.0f * outputGainParameter->load());
        inGainParam->setValueNotifyingHost (linkedValue);
    };

    if (paramID == "linkGain") // link output gain to input gain
    {
        if (newValue == 1)
            linkOutputGain();
        return;
    }

    if (paramID == "inputGain" && static_cast<bool> (*linkGainParameter))
    {
        linkOutputGain();
        return;
    }

    if (paramID == "outputGain" && static_cast<bool> (*linkGainParameter))
    {
        linkInputGain();
        return;
    }
}

void ronn::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    inputGain.prepare ({ sampleRate, (uint32) samplesPerBlock, (uint32) getMainBusNumInputChannels() });
    outputGain.prepare ({ sampleRate, (uint32) samplesPerBlock, (uint32) getMainBusNumOutputChannels() });
}

void ronn::releaseResources()
{
}

void ronn::processAudioBlock (AudioBuffer<float>& buffer)
{
    // process input gain
    dsp::AudioBlock<float> audioBlock (buffer);
    inputGain.setGainDecibels (*inputGainParameter);
    inputGain.process (dsp::ProcessContextReplacing<float> { audioBlock });

    // process output gain
    outputGain.setGainDecibels (*outputGainParameter);
    outputGain.process (dsp::ProcessContextReplacing<float> { audioBlock });
}

AudioProcessorEditor* ronn::createEditor()
{
    return new RonnAudioProcessorEditor (*this, vts);
}

void ronn::getStateInformation (MemoryBlock& data)
{
    auto state = vts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, data);
}

void ronn::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (vts.state.getType()))
            vts.replaceState (ValueTree::fromXml (*xmlState));
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ronn();
}
