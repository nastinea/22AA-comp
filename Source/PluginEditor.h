#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace aa22ui
{
    // Base design size; the whole UI is drawn in these coordinates and scaled.
    constexpr int baseW = 1000;
    constexpr int baseH = 276;

    class RackLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    private:
        void drawBigKnob (juce::Graphics&, juce::Rectangle<float> area, float angle, juce::Slider&);
        void drawPointerKnob (juce::Graphics&, juce::Rectangle<float> area, float angle);
        void drawScrewTrim (juce::Graphics&, juce::Rectangle<float> area, float angle);
    };

    class VUMeter : public juce::Component, private juce::Timer
    {
    public:
        explicit VUMeter (AA22CompProcessor&);
        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;
        float dbToNeedle (float db) const;

        AA22CompProcessor& proc;
        float needle = 0.0f;    // 0..1 position on the scale
        float velocity = 0.0f;
        float lamp = 1.0f;
    };

    class Content : public juce::Component
    {
    public:
        explicit Content (AA22CompProcessor&);
        ~Content() override;
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        AA22CompProcessor& proc;
        RackLookAndFeel lnf;

        juce::Slider gainKnob, peakKnob, meterKnob, emphasisScrew;
        juce::ToggleButton limitSwitch, powerSwitch;
        VUMeter vu;

        using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
        using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
        std::unique_ptr<SA> gainAtt, peakAtt, meterAtt, emphasisAtt;
        std::unique_ptr<BA> limitAtt, powerAtt;

        void drawScrew (juce::Graphics&, float cx, float cy, float r) const;
        void drawKnobScale (juce::Graphics&, juce::Point<float> c, float radius) const;
    };
}

class AA22CompEditor : public juce::AudioProcessorEditor
{
public:
    explicit AA22CompEditor (AA22CompProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    aa22ui::Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AA22CompEditor)
};
