#pragma once

#include <JuceHeader.h>

class ronn : public chowdsp::PluginBase<ronn>,
             private AudioProcessorValueTreeState::Listener
{
public:
    ronn();

    static void addParameters (Parameters& params);
    void parameterChanged (const String& parameterID, float newValue) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (AudioBuffer<float>& buffer) override;

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    void getStateInformation (MemoryBlock& data) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    std::atomic<float>* inputGainParameter  = nullptr;
    std::atomic<float>* outputGainParameter = nullptr;
    std::atomic<float>* layersParameter     = nullptr;
    std::atomic<float>* channelsParameter   = nullptr;
    std::atomic<float>* kernelParameter     = nullptr;
    std::atomic<float>* useBiasParameter    = nullptr;
    std::atomic<float>* dilationParameter   = nullptr;
    std::atomic<float>* activationParameter = nullptr;
    std::atomic<float>* initTypeParameter   = nullptr;
    std::atomic<float>* linkGainParameter   = nullptr;
    std::atomic<float>* seedParameter       = nullptr;
    std::atomic<float>* depthwiseParameter  = nullptr;

    dsp::StateVariableTPTFilter<float> dcBlocker;
    dsp::Gain<float> inputGain;
    dsp::Gain<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ronn)
};
