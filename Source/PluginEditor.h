#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class JetLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JetLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class JetAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit JetAudioProcessorEditor (JetAudioProcessor&);
    ~JetAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob&, const juce::String& paramId, const juce::String& name);

    JetAudioProcessor& proc;
    JetLookAndFeel lnf;
    Knob rate, depth, manual, feedback, tone, spread, mix, output;
    std::vector<Knob*> knobs;
    juce::ToggleButton tzButton { "THROUGH-ZERO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tzAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JetAudioProcessorEditor)
};
