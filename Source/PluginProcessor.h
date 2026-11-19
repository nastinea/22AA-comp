#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "OptoCompressor.h"

class AA22CompProcessor : public juce::AudioProcessor
{
public:
    AA22CompProcessor();
    ~AA22CompProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Metering (read by the editor on the message thread)
    std::atomic<float> meterGainReductionDb { 0.0f };
    std::atomic<float> meterOutputRms { 0.0f };

private:
    aa22::OptoCompressor comp;

    std::atomic<float>* pPeakReduction = nullptr;
    std::atomic<float>* pGain = nullptr;
    std::atomic<float>* pLimit = nullptr;
    std::atomic<float>* pEmphasis = nullptr;
    std::atomic<float>* pPower = nullptr;

    float rmsState = 0.0f;
    float rmsCoef = 0.0f;
    bool wasOn = true;
    juce::AudioBuffer<float> dryBuffer;

    void updateOutputMeter (const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AA22CompProcessor)
};
