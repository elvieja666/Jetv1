#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kMaxManualMs = 10.0f;   // máximo barrido / delay central
    constexpr float kTzRefMs     = 10.5f;   // delay fijo de la señal seca en modo through-zero
    constexpr float kMinDelay    = 1.0f;    // en muestras (la interpolación Hermite necesita >= 1)

    // Saturación suave en el lazo de feedback: evita que se dispare con feedback alto
    inline float softClip (float x) { return x / (1.0f + std::abs (x) * 0.3f); }
}

juce::AudioProcessorValueTreeState::ParameterLayout JetAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    NormalisableRange<float> rate (0.01f, 5.0f, 0.001f);
    rate.setSkewForCentre (0.3f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "rate", 1 }, "Rate", rate, 0.12f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "depth", 1 }, "Depth",
                    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 90.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));

    NormalisableRange<float> manual (0.2f, kMaxManualMs, 0.01f);
    manual.setSkewForCentre (2.5f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "manual", 1 }, "Manual", manual, 4.0f,
                    AudioParameterFloatAttributes().withLabel ("ms")));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "feedback", 1 }, "Feedback",
                    NormalisableRange<float> (-95.0f, 95.0f, 0.1f), 70.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));

    NormalisableRange<float> tone (500.0f, 20000.0f, 1.0f);
    tone.setSkewForCentre (4000.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", tone, 9000.0f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "spread", 1 }, "Spread",
                    NormalisableRange<float> (0.0f, 180.0f, 1.0f), 90.0f,
                    AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("\xc2\xb0"))));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "tz", 1 }, "Through-Zero", true));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                    NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));

    return { p.begin(), p.end() };
}

JetAudioProcessor::JetAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Jet", createLayout())
{
    pRate     = apvts.getRawParameterValue ("rate");
    pDepth    = apvts.getRawParameterValue ("depth");
    pManual   = apvts.getRawParameterValue ("manual");
    pFeedback = apvts.getRawParameterValue ("feedback");
    pTone     = apvts.getRawParameterValue ("tone");
    pSpread   = apvts.getRawParameterValue ("spread");
    pTz       = apvts.getRawParameterValue ("tz");
    pMix      = apvts.getRawParameterValue ("mix");
    pOut      = apvts.getRawParameterValue ("output");
}

bool JetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void JetAudioProcessor::updateLatency()
{
    lastTz = pTz->load() > 0.5f;
    latency = lastTz ? (int) std::round (kTzRefMs * 0.001 * fs) : 0;
    setLatencySamples (latency);
}

void JetAudioProcessor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;
    const int maxSamples = (int) std::ceil ((kTzRefMs + kMaxManualMs * 2.0f + 2.0f) * 0.001 * fs) + 8;
    for (auto& l : lines) l.prepare (maxSamples);
    fbState.fill (0.0f);
    toneState.fill (0.0f);
    lfoPhase = 0.0;

    for (auto* sm : { &manualSm, &depthSm, &fbSm, &mixSm, &outSm })
        sm->reset (sampleRate, 0.05);
    manualSm.setCurrentAndTargetValue (pManual->load());
    depthSm.setCurrentAndTargetValue (pDepth->load() * 0.01f);
    fbSm.setCurrentAndTargetValue (pFeedback->load() * 0.01f);
    mixSm.setCurrentAndTargetValue (pMix->load() * 0.01f);
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    updateLatency();
}

void JetAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int c = numIn; c < numOut; ++c)
        buffer.clear (c, 0, numSamples);

    const bool tz = pTz->load() > 0.5f;
    if (tz != lastTz)
    {
        updateLatency();
        for (auto& l : lines) std::fill (l.buf.begin(), l.buf.end(), 0.0f);
        fbState.fill (0.0f);
    }

    manualSm.setTargetValue (pManual->load());
    depthSm.setTargetValue (pDepth->load() * 0.01f);
    fbSm.setTargetValue (pFeedback->load() * 0.01f);
    mixSm.setTargetValue (pMix->load() * 0.01f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    const double lfoInc = pRate->load() / fs;
    const double spread = pSpread->load() / 360.0;   // desfase del LFO derecho (en ciclos)
    const float toneCoef = std::exp (-juce::MathConstants<float>::twoPi * pTone->load() / (float) fs);
    const float msToSamples = (float) (0.001 * fs);
    const float refDelay = tz ? (float) latency : 0.0f;
    const float maxDelay = (float) (lines[0].mask - 4);

    const int chans = juce::jmin (2, numOut);
    float* io[2] = { buffer.getWritePointer (0), chans > 1 ? buffer.getWritePointer (1) : nullptr };
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0);

    for (int s = 0; s < numSamples; ++s)
    {
        const float manual = manualSm.getNextValue() * msToSamples;
        const float depth  = depthSm.getNextValue();
        const float fb     = fbSm.getNextValue();
        const float mix    = mixSm.getNextValue();
        const float gain   = outSm.getNextValue();
        const float fbComp = std::sqrt (1.0f - fb * fb);

        const float inputs[2] = { io[0][s], chans > 1 ? inR[s] : io[0][s] };

        for (int c = 0; c < chans; ++c)
        {
            double ph = lfoPhase + (c == 1 ? spread : 0.0);
            const float lfo = (float) std::sin (juce::MathConstants<double>::twoPi * ph);   // -1..1

            float wetDelay;
            if (tz)   // through-zero: la copia barre alrededor de la seca (adelante y atrás)
                wetDelay = refDelay + manual * depth * lfo;
            else      // clásico: la copia siempre va atrás, entre manual*(1-depth) y manual*(1+depth)
                wetDelay = manual * (1.0f + depth * lfo);
            wetDelay = juce::jlimit (kMinDelay, maxDelay, wetDelay);

            auto& line = lines[(size_t) c];
            // Feedback con pasabajos (Tone) y saturación suave
            toneState[(size_t) c] = toneCoef * toneState[(size_t) c] + (1.0f - toneCoef) * fbState[(size_t) c];
            line.push (inputs[c] + softClip (fb * toneState[(size_t) c]));

            const float wet = line.read (wetDelay);
            const float dry = tz ? line.read (refDelay) : inputs[c];   // en TZ la seca también va demorada
            fbState[(size_t) c] = wet;

            // Compensación de la ganancia del feedback (un peine con feedback g sube ~1/(1-g²) en potencia)
            const float wetComp = wet * fbComp;
            io[c][s] = (dry * (1.0f - mix) + wetComp * mix) * gain;
        }

        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
    }
}

juce::AudioProcessorEditor* JetAudioProcessor::createEditor()
{
    return new JetAudioProcessorEditor (*this);
}

void JetAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void JetAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JetAudioProcessor();
}
