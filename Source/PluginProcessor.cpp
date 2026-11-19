#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids
{
    static const juce::String peakReduction = "peakReduction";
    static const juce::String gain          = "gain";
    static const juce::String limit         = "limit";
    static const juce::String emphasis      = "emphasis";
    static const juce::String meter         = "meter";
    static const juce::String power         = "power";
}

AA22CompProcessor::AA22CompProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pPeakReduction = apvts.getRawParameterValue (ids::peakReduction);
    pGain          = apvts.getRawParameterValue (ids::gain);
    pLimit         = apvts.getRawParameterValue (ids::limit);
    pEmphasis      = apvts.getRawParameterValue (ids::emphasis);
    pPower         = apvts.getRawParameterValue (ids::power);
}

juce::AudioProcessorValueTreeState::ParameterLayout AA22CompProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (v, 1); });

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::peakReduction, 1 }, "Peak Reduction",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, pct));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::gain, 1 }, "Gain",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) {
                const float db = (v - 50.0f) * 0.4f;
                return String (v, 1) + " (" + (db >= 0 ? "+" : "") + String (db, 1) + " dB)";
            })));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ids::limit, 1 }, "Limit", false,
        AudioParameterBoolAttributes().withStringFromValueFunction (
            [] (bool v, int) { return v ? String ("Limit") : String ("Compress"); })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::emphasis, 1 }, "HF Emphasis",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, pct));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::meter, 1 }, "Meter",
        StringArray { "Output +10", "Gain Reduction", "Output +4" }, 1));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ids::power, 1 }, "Power", true));

    return { params.begin(), params.end() };
}

bool AA22CompProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void AA22CompProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    comp.prepare (sampleRate);
    aa22::OptoCompressor::Params p;
    p.peakReduction = pPeakReduction->load();
    p.gain = pGain->load();
    p.limit = pLimit->load() > 0.5f;
    p.emphasis = pEmphasis->load();
    comp.setParams (p);
    comp.reset();

    rmsCoef = 1.0f - std::exp (-1.0f / (0.05f * (float) sampleRate)); // 50 ms
    rmsState = 0.0f;
    wasOn = pPower->load() > 0.5f;
    dryBuffer.setSize (2, juce::jmax (1, samplesPerBlock), false, false, true);
}

void AA22CompProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), getTotalNumInputChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    if (numCh == 0 || n == 0) return;

    const bool isOn = pPower->load() > 0.5f;

    if (! isOn && ! wasOn)
    {
        meterGainReductionDb.store (0.0f);
        updateOutputMeter (buffer);
        return;
    }

    // Keep a dry copy when the power switch changes so we can crossfade (no clicks).
    const bool transition = isOn != wasOn;
    if (transition)
    {
        dryBuffer.setSize (2, n, false, false, true);
        for (int c = 0; c < numCh; ++c)
            dryBuffer.copyFrom (c, 0, buffer, c, 0, n);
    }

    aa22::OptoCompressor::Params p;
    p.peakReduction = pPeakReduction->load();
    p.gain = pGain->load();
    p.limit = pLimit->load() > 0.5f;
    p.emphasis = pEmphasis->load();
    comp.setParams (p);
    comp.process (buffer.getArrayOfWritePointers(), numCh, n);

    if (transition)
    {
        // isOn: fade processed in; !isOn: fade processed out.
        const float wetStart = isOn ? 0.0f : 1.0f;
        const float wetEnd   = isOn ? 1.0f : 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            buffer.applyGainRamp (c, 0, n, wetStart, wetEnd);
            buffer.addFromWithRamp (c, 0, dryBuffer.getReadPointer (c), n, 1.0f - wetStart, 1.0f - wetEnd);
        }
        if (! isOn) comp.reset();
    }

    wasOn = isOn;
    meterGainReductionDb.store (isOn ? comp.getGainReductionDb() : 0.0f);
    updateOutputMeter (buffer);
}

void AA22CompProcessor::updateOutputMeter (const juce::AudioBuffer<float>& buffer)
{
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            const float v = buffer.getReadPointer (c)[i];
            s = juce::jmax (s, v * v);
        }
        rmsState += rmsCoef * (s - rmsState);
    }
    meterOutputRms.store (std::sqrt (juce::jmax (0.0f, rmsState)));
}

void AA22CompProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AA22CompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AA22CompProcessor::createEditor()
{
    return new AA22CompEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AA22CompProcessor();
}
