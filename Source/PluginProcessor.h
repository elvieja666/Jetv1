#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

// Jet: flanger "jet plane" con modo through-zero (estilo flanging de cinta).
// Una copia de la señal pasa por un delay muy corto que barre lentamente;
// al sumarla con la original se forma un filtro peine que se mueve. Con
// feedback alto suena metálico ("jet"); en modo through-zero la copia cruza
// a la original y las cancelaciones pasan por cero, como dos cintas.
class JetAudioProcessor : public juce::AudioProcessor
{
public:
    JetAudioProcessor();
    ~JetAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Jet"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    struct DelayLine
    {
        std::vector<float> buf;
        int mask = 0, write = 0;
        void prepare (int minSize)
        {
            int n = 1;
            while (n < minSize) n <<= 1;
            buf.assign ((size_t) n, 0.0f);
            mask = n - 1;
            write = 0;
        }
        inline void push (float x)
        {
            write = (write + 1) & mask;
            buf[(size_t) write] = x;
        }
        // Lectura fraccional con interpolación Hermite (4 puntos); delay >= 1 muestra
        inline float read (float delay) const
        {
            const int   di   = (int) delay;
            const float frac = delay - (float) di;
            const int   i0   = write - di;
            const float xm1 = buf[(size_t) ((i0 + 1) & mask)];
            const float x0  = buf[(size_t) ((i0)     & mask)];
            const float x1  = buf[(size_t) ((i0 - 1) & mask)];
            const float x2  = buf[(size_t) ((i0 - 2) & mask)];
            const float c1 = 0.5f * (x1 - xm1);
            const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
            const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
            return ((c3 * frac + c2) * frac + c1) * frac + x0;
        }
    };

    void updateLatency();

    double fs = 44100.0;
    std::array<DelayLine, 2> lines;
    std::array<float, 2> fbState {};     // última salida húmeda (feedback)
    std::array<float, 2> toneState {};   // pasabajos del feedback
    double lfoPhase = 0.0;
    bool lastTz = false;
    int  latency = 0;

    juce::SmoothedValue<float> manualSm, depthSm, fbSm, mixSm, outSm;

    std::atomic<float>* pRate;
    std::atomic<float>* pDepth;
    std::atomic<float>* pManual;
    std::atomic<float>* pFeedback;
    std::atomic<float>* pTone;
    std::atomic<float>* pSpread;
    std::atomic<float>* pTz;
    std::atomic<float>* pMix;
    std::atomic<float>* pOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JetAudioProcessor)
};
