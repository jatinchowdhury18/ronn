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

    ronnParams = Model::Params {
        getMainBusNumInputChannels(),
        getMainBusNumOutputChannels(),
        (int)  *layersParameter,
        (int)  *channelsParameter,
        (int)  *kernelParameter,
        (int)  *dilationParameter,
        (bool) *useBiasParameter,
        (int)  *activationParameter,
        (int)  *initTypeParameter,
        (int)  *seedParameter,
        (bool) *depthwiseParameter
    };

    vts.addParameterListener ("inputGain", this);
    vts.addParameterListener ("outputGain", this);
    vts.addParameterListener ("linkGain", this);

    vts.addParameterListener ("layers", this);
    vts.addParameterListener ("kernel", this);
    vts.addParameterListener ("channels", this);
    vts.addParameterListener ("dilation", this);
    vts.addParameterListener ("useBias", this);
    vts.addParameterListener ("initType", this);
    vts.addParameterListener ("seed", this);
    vts.addParameterListener ("depthwise", this);

    buildModel();
}

void ronn::addParameters (Parameters& params)
{
    // define these for now so we don't blow up the CPU
    constexpr int maxLayers = 8; // 24;
    constexpr int maxKWidth = 8; // 64;
    constexpr int maxChannels = 8; // 64;

    params.push_back (std::make_unique<AudioParameterInt>   ("layers", "Layers", 1, maxLayers, 6));
    params.push_back (std::make_unique<AudioParameterInt>   ("kernel", "Kernel Width", 1, maxKWidth, 3));
    params.push_back (std::make_unique<AudioParameterInt>   ("channels", "Channels", 1, maxChannels, 8));
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

    // rebuild the whole model
    ronnParams = Model::Params {
        getMainBusNumInputChannels(),
        getMainBusNumOutputChannels(),
        (int)  *layersParameter,
        (int)  *channelsParameter,
        (int)  *kernelParameter,
        (int)  *dilationParameter,
        (bool) *useBiasParameter,
        (int)  *activationParameter,
        (int)  *initTypeParameter,
        (int)  *seedParameter,
        (bool) *depthwiseParameter
    };

    if (paramID == "layers")
        ronnParams.nLayers = (int) newValue;
    else if (paramID == "kernel")
        ronnParams.kWidth = (int) newValue;
    else if (paramID == "channels")
        ronnParams.nChannels = (int) newValue;
    else if (paramID == "dilation")
        ronnParams.dilationFactor = (int) newValue;
    else if (paramID == "useBias")
        ronnParams.useBias = (bool) newValue;
    else if (paramID == "initType")
        ronnParams.init = (int) newValue;
    else if (paramID == "seed")
        ronnParams.seed = (int) newValue;
    else if (paramID == "depthwise")
        ronnParams.dwise = (bool) newValue;

    buildModel();
}

void ronn::buildModel()
{
    SpinLock::ScopedLockType modelLoadScopedLock (modelLoadLock);
    ronnModel = std::make_unique<Model> (ronnParams);
    calculateReceptiveField();
    sendChangeMessage(); // tell GUI to update!
}

void ronn::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);

    dsp::ProcessSpec inputSpec { sampleRate,
                                 (uint32) samplesPerBlock,
                                 (uint32) getMainBusNumInputChannels() };
    
    dsp::ProcessSpec outputSpec { sampleRate,
                                  (uint32) samplesPerBlock,
                                  (uint32) getMainBusNumOutputChannels() };

    inputGain.prepare (inputSpec);
    outputGain.prepare (outputSpec);

    dcBlocker.prepare (outputSpec);
    dcBlocker.setType (dsp::StateVariableTPTFilterType::highpass);
    dcBlocker.setCutoffFrequency (30.0f);

    ronnModel->model->reset();
}

void ronn::releaseResources()
{
}

void ronn::calculateReceptiveField()
{
    int k = *kernelParameter;
    int d = *dilationParameter;
    int l = *layersParameter;
    double rf =  k * d;

    for (int layer = 1; layer < l; ++layer) {
        rf = rf + ((k-1) * pow(d,layer));
    }

    receptiveFieldSamples = rf; // store in attribute
}

void ronn::processAudioBlock (AudioBuffer<float>& buffer)
{
    SpinLock::ScopedTryLockType modelLoadTryLock (modelLoadLock);

    // don't try to process anything while the model is loading!
    if (! modelLoadTryLock.isLocked())
        return;

    // process input gain
    dsp::AudioBlock<float> audioBlock (buffer);
    inputGain.setGainDecibels (*inputGainParameter);
    inputGain.process (dsp::ProcessContextReplacing<float> { audioBlock });

    if (getMainBusNumInputChannels() == 1)
    {
        auto* x = audioBlock.getChannelPointer (0);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float modelInput[] = { x[n] };
            x[n] = ronnModel->model->forward (modelInput);
        }
    }
    else if (getMainBusNumInputChannels() == 2)
    {
        auto* xLeft = audioBlock.getChannelPointer (0);
        auto* xRight = audioBlock.getChannelPointer (1);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float modelInput[] = { xLeft[n], xRight[n] };
            ronnModel->model->forward (modelInput);
            
            auto* outs = ronnModel->model->getOutputs();
            xLeft[n] = outs[0];
            xRight[n] = outs[1];
        }
    }
    else
    {
        // @TODO: handle any number of channels
        jassertfalse;
    }

    // DC blocker
    dcBlocker.process (dsp::ProcessContextReplacing<float> { audioBlock });

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
