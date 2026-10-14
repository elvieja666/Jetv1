#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg     (0xff18191c);
    const juce::Colour kPanel  (0xff24262a);
    const juce::Colour kAccent (0xffe8a33d);   // ámbar "turbina"
    const juce::Colour kText   (0xffdedede);
    const juce::Colour kDim    (0xff6b6d73);
}

JetLookAndFeel::JetLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, kText);
}

void JetLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float track = 3.5f;

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, endAngle, true);
    g.setColour (kDim.withAlpha (0.5f));
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, angle, true);
    g.setColour (kAccent);
    g.strokePath (val, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = radius - track * 3.0f;
    g.setColour (kPanel.brighter (0.15f));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR + 4.0f, 3.0f, knobR * 0.45f, 1.5f);
    g.setColour (kText);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void JetLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? kAccent : kPanel.brighter (highlighted ? 0.2f : 0.1f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? kBg : kDim.brighter (0.5f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

JetAudioProcessorEditor::JetAudioProcessorEditor (JetAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addKnob (rate,     "rate",     "RATE");
    addKnob (depth,    "depth",    "DEPTH");
    addKnob (manual,   "manual",   "MANUAL");
    addKnob (feedback, "feedback", "FEEDBACK");
    addKnob (tone,     "tone",     "TONE");
    addKnob (spread,   "spread",   "SPREAD");
    addKnob (mix,      "mix",      "MIX");
    addKnob (output,   "output",   "OUTPUT");

    addAndMakeVisible (tzButton);
    tzAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "tz", tzButton);

    setSize (560, 420);
}

JetAudioProcessorEditor::~JetAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void JetAudioProcessorEditor::addKnob (Knob& k, const juce::String& paramId, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, kDim.brighter (0.4f));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
    knobs.push_back (&k);
}

void JetAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    auto area = getLocalBounds().reduced (14);
    auto header = area.removeFromTop (44);

    g.setColour (kText);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("JET", header.removeFromLeft (90), juce::Justification::centredLeft);
    g.setColour (kDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("tape flanger", header.removeFromLeft (120), juce::Justification::centredLeft);

    g.setColour (kPanel);
    g.fillRoundedRectangle (area.toFloat(), 8.0f);
}

void JetAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14);
    auto header = area.removeFromTop (44);
    tzButton.setBounds (header.removeFromRight (130).reduced (0, 8));

    area.reduce (10, 10);
    const int cols = 4, rows = 2;
    const int cellW = area.getWidth() / cols;
    const int cellH = area.getHeight() / rows;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols;
        auto cell = juce::Rectangle<int> (area.getX() + c * cellW, area.getY() + r * cellH, cellW, cellH).reduced (6);
        knobs[i]->label.setBounds (cell.removeFromTop (18));
        knobs[i]->slider.setBounds (cell);
    }
}
