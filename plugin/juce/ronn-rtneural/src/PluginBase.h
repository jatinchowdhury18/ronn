#pragma once

#include <JuceHeader.h>

namespace chowdsp
{
/**
 * Base class for plugin processors.
 * 
 * Derived classes must override `prepareToPlay` and `releaseResources`
 * (from `juce::AudioProcessor`), as well as `processAudioBlock`, and
 * `addParameters`.
*/
template <class Processor>
class PluginBase : public juce::AudioProcessor
{
public:
    PluginBase();
    PluginBase (const juce::AudioProcessor::BusesProperties& layout);
    ~PluginBase();

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    virtual void processAudioBlock (juce::AudioBuffer<float>&) = 0;

protected:
    using Parameters = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;
    juce::AudioProcessorValueTreeState vts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBase)
};

/** Class that uses SFINAE to ensure that the
 *  processor class has an `addParameters` function
*/
template <typename T>
class HasAddParameters
{
    typedef char one;
    typedef long two;

    template <typename C>
    static one test (decltype (&C::addParameters));
    template <typename C>
    static two test (...);

public:
    enum
    {
        value = sizeof (test<T> (0)) == sizeof (char)
    };
};

template <class Processor>
PluginBase<Processor>::PluginBase() : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
                                      vts (*this, nullptr, juce::Identifier ("Parameters"), createParameterLayout())
{
}

template <class Processor>
PluginBase<Processor>::PluginBase (const juce::AudioProcessor::BusesProperties& layout) : AudioProcessor (layout),
                                                                                          vts (*this, nullptr, juce::Identifier ("Parameters"), createParameterLayout())
{
}

template <class Processor>
PluginBase<Processor>::~PluginBase()
{
}

template <class Processor>
juce::AudioProcessorValueTreeState::ParameterLayout PluginBase<Processor>::createParameterLayout()
{
    Parameters params;

    static_assert (HasAddParameters<Processor>::value,
                   "Processor class MUST contain a static addParameters function!");
    Processor::addParameters (params);

    return { params.begin(), params.end() };
}

template <class Processor>
bool PluginBase<Processor>::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    // only supports mono and stereo (for now)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // input and output layout must be the same
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

template <class Processor>
void PluginBase<Processor>::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
}

template <class Processor>
void PluginBase<Processor>::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    processAudioBlock (buffer);
}

} // namespace chowdsp