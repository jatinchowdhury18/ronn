#pragma once

#include <JuceHeader.h>

class ronn : public chowdsp::PluginBase<ronn>
{
public:
    ronn() {}

    static void addParameters (Parameters& params);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (AudioBuffer<float>& buffer) override;

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    void getStateInformation (MemoryBlock& data) override {} //TODO
    void setStateInformation (const void* data, int sizeInBytes) override {} //TODO

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ronn)
};
