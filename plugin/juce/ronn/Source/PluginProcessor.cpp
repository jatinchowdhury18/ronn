/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RonnAudioProcessor::RonnAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters (*this, nullptr, Identifier ("ronn"),
    {
        std::make_unique<AudioParameterInt>   ("layers", "Layers", 1, 24, 6),
        std::make_unique<AudioParameterInt>   ("kernel", "Kernel Width", 1, 64, 3),
        std::make_unique<AudioParameterInt>   ("channels", "Channels", 1, 64, 8),
        std::make_unique<AudioParameterFloat> ("inputGain", "Input Gain", -24.0f, 24.0f, 0.0f),   
        std::make_unique<AudioParameterFloat> ("outputGain", "Output Gain", -24.0f, 24.0f, 0.0f),
        std::make_unique<AudioParameterBool>  ("useBias", "Use Bias", false),
        std::make_unique<AudioParameterInt>   ("activation", "Activation", 1, 10, 1),
        std::make_unique<AudioParameterInt>   ("dilation", "Dilation Factor", 1, 4, 1),
        std::make_unique<AudioParameterInt>   ("initType", "Init Type", 1, 6, 1),
        std::make_unique<AudioParameterInt>   ("seed", "Seed", 0, 1024, 42),
        std::make_unique<AudioParameterBool>  ("linkGain", "Link", false),
        std::make_unique<AudioParameterBool>  ("depthwise", "Depthwise", false)
    })
{
 
    layersParameter     = parameters.getRawParameterValue ("layers");
    kernelParameter     = parameters.getRawParameterValue ("kernel");
    channelsParameter   = parameters.getRawParameterValue ("channels");
    inputGainParameter  = parameters.getRawParameterValue ("inputGain");
    outputGainParameter = parameters.getRawParameterValue ("outputGain");
    useBiasParameter    = parameters.getRawParameterValue ("useBias");
    activationParameter = parameters.getRawParameterValue ("activation");
    dilationParameter   = parameters.getRawParameterValue ("dilation");
    initTypeParameter   = parameters.getRawParameterValue ("initType");
    seedParameter       = parameters.getRawParameterValue ("seed");
    depthwiseParameter  = parameters.getRawParameterValue ("depthwise");

    // neural network model
    model = std::make_shared<Model>(nInputs, 
                                   nOutputs, 
                                   *layersParameter, 
                                   *channelsParameter, 
                                   *kernelParameter, 
                                   *dilationParameter,
                                   *useBiasParameter, 
                                   *activationParameter,
                                   *initTypeParameter,
                                   *seedParameter,
                                   *depthwiseParameter);
}

RonnAudioProcessor::~RonnAudioProcessor()
{
    // we may need to delete the model here
}

//==============================================================================
const String RonnAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RonnAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RonnAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RonnAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RonnAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RonnAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RonnAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RonnAudioProcessor::setCurrentProgram (int index)
{
}

const String RonnAudioProcessor::getProgramName (int index)
{
    return {};
}

void RonnAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void RonnAudioProcessor::prepareToPlay (double sampleRate_, int samplesPerBlock_)
{
    // store the sample rate for future calculations
    sampleRate = sampleRate_;
    blockSamples = samplesPerBlock_;

    // setup high pass filter model
    double freq = 10.0;
    double q = 10.0;
    highPassFilters.clear();
    modelInputData.clear();
    for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel) {
        IIRFilter filter;
        filter.setCoefficients(IIRCoefficients::makeHighPass (sampleRate_, freq, q));
        highPassFilters.push_back(filter);
        modelInputData.push_back (0.0f);

    }

    calculateReceptiveField();      // compute the receptive field, make sure it's up to date
    setupBuffers();                 // setup the buffer for handling context
}

void RonnAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RonnAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RonnAudioProcessor::calculateReceptiveField()
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

void RonnAudioProcessor::setupBuffers()
{
    // compute the size of the buffer which will be passed to model
    mBufferLength = (int)(receptiveFieldSamples + blockSamples - 1);
    iBufferLength = (int)(receptiveFieldSamples + blockSamples - 1);

    // Initialize the to n channels
    nInputs = getTotalNumInputChannels();

    // and mBufferLength samples per channel
    mBuffer.setSize(nInputs, mBufferLength);
    mBuffer.clear();
    mBufferReadIdx = 0;
    mBufferWriteIdx = 0;

    // this buffer stories copy of data in mBuffer (always read from index 0)
    iBuffer.setSize(nInputs, iBufferLength);
    iBuffer.clear();
}

void RonnAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto inChannels  = getTotalNumInputChannels();
    auto outChannels = getTotalNumOutputChannels();

    if (modelChange == true) {
        buildModel(*seedParameter);
        calculateReceptiveField();
        setupBuffers();
        modelChange = false;
    }

    //if (true) {
    //    model->initModel(std::rand() %  1024);
    //}

    int twp, trp; // temporary write/read pointer counters

    for (int channel = 0; channel < nInputs; ++channel) {

        // to process each channel reset the pointers to global state
        twp = mBufferWriteIdx; 
        trp = mBufferReadIdx;  

        // first we copy the input to circular buffer
        for (int n = 0; n < blockSamples; n++) {
            mBuffer.setSample(channel, twp, buffer.getSample(channel, n));
            twp = twp + 1;
            if (twp > (mBufferLength-1)){
                twp = 0;
            }
        }

        trp = twp;  // start reading from where we finished writing

        // now we read these data into new time-aligned buffer
        for (int n = 0; n < mBufferLength; n++) {
            iBuffer.setSample(channel, n, mBuffer.getSample(channel, trp));
            trp = trp + 1; 
            if (trp > (mBufferLength-1)){
                trp = 0;
            }
        }
    }

    // now update the read and writer pointer/indices for global state
    mBufferWriteIdx = twp;
    mBufferReadIdx  = trp;

    iBuffer.applyGain (inputGainLn);

    // @TODO: use JUCE interleaver to interleave these channels so we have
    // instant access to the data in the correct shape!
    auto** iPtrs = iBuffer.getArrayOfWritePointers();
    auto** bPtrs = buffer.getArrayOfWritePointers();

    if (nInputs == 1)
    {
        for (int n = 0; n < iBuffer.getNumSamples(); ++n)
        {
            modelInputData[0] = iPtrs[0][n];
            bPtrs[0][n] = model->model->forward (modelInputData.data());
        }
    }
    else if (nInputs == 2)
    {
        for (int n = 0; n < iBuffer.getNumSamples(); ++n)
        {
            modelInputData[0] = iPtrs[0][n];
            modelInputData[1] = iPtrs[1][n];
            model->model->forward (modelInputData.data());
            
            auto* outs = model->model->getOutputs();
            bPtrs[0][n] = outs[0];
            bPtrs[1][n] = outs[1];
        }
    }
    else
    {
        // @TODO
        jassertfalse;
    }

    // now load the output channels back into the buffer
    for (int channel = 0; channel < outChannels; ++channel) {
        highPassFilters[channel].processSamples (buffer.getWritePointer (channel), buffer.getNumSamples());
    }
    buffer.applyGain(outputGainLn);                                  // apply the output gain

}

//==============================================================================
bool RonnAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* RonnAudioProcessor::createEditor()
{
    return new RonnAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void RonnAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RonnAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (ValueTree::fromXml (*xmlState));
}

//==============================================================================

void RonnAudioProcessor::buildModel(int seed) 
{
    model.reset(new Model(nInputs, 
                        nOutputs, 
                        *layersParameter, 
                        *channelsParameter, 
                        *kernelParameter, 
                        *dilationParameter,
                        *useBiasParameter,
                        *activationParameter,
                        *initTypeParameter,
                        *seedParameter,
                        *depthwiseParameter));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RonnAudioProcessor();
}
