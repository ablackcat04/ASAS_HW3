#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

struct ShepardSound final : juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class ShepardVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<ShepardSound*> (sound) != nullptr;
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        // Only chroma changes: C in every MIDI octave has exactly the same spectrum.
        const auto pitchClass = midiNote % 12;
        constexpr double c0 = 16.351597831287414;
        const auto baseFrequency = c0 * std::exp2 (static_cast<double> (pitchClass) / 12.0);
        const auto sampleRate = getSampleRate();
        double amplitudeSum = 0.0;

        for (size_t k = 0; k < partials.size(); ++k)
        {
            auto& partial = partials[k];
            const auto frequency = std::ldexp (baseFrequency, static_cast<int> (k));
            // Fixed Gaussian over log2(f), fitted approximately to the reference.
            const auto distance = std::log2 (frequency / 440.0) / 1.25;
            auto amplitude = std::exp (-0.5 * distance * distance);
            // Fade before Nyquist; no aliased upper ranks at low sample rates.
            amplitude *= juce::jlimit (0.0, 1.0, (0.45 * sampleRate - frequency) / (0.05 * sampleRate));
            const auto angle = juce::MathConstants<double>::twoPi * frequency / sampleRate;
            partial = { 1.0, 0.0, std::cos (angle), std::sin (angle), amplitude };
            amplitudeSum += amplitude;
        }

        // L1 normalization bounds a single voice even if all ranks line up.
        for (auto& partial : partials)
            partial.amplitude *= static_cast<double> (velocity) / juce::jmax (amplitudeSum, 1.0e-12);

        envelope.setSampleRate (sampleRate);
        envelope.setParameters ({ 0.008f, 0.0f, 1.0f, 0.10f });
        envelope.reset();
        envelope.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            envelope.noteOff();
        else
        {
            envelope.reset();
            clearCurrentNote();
        }
    }

    // Pitch bend would move the fixed envelope away from its circular note bank.
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            double sum = 0.0;
            for (auto& partial : partials)
            {
                sum += partial.imaginary * partial.amplitude;
                // Quadrature recurrence avoids trigonometry in the audio loop.
                const auto nextReal = partial.real * partial.cosStep - partial.imaginary * partial.sinStep;
                partial.imaginary = partial.imaginary * partial.cosStep + partial.real * partial.sinStep;
                partial.real = nextReal;
            }
            const auto sample = static_cast<float> (sum) * envelope.getNextSample();
            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, startSample + i, sample);

            if (! envelope.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    struct Partial
    {
        double real = 1.0;
        double imaginary = 0.0;
        double cosStep = 1.0;
        double sinStep = 0.0;
        double amplitude = 0.0;
    };

    std::array<Partial, 10> partials;
    juce::ADSR envelope;
};
