#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ShepardVoice.h"

#include <cmath>

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> { -48.0f, -18.0f, 0.1f }, -24.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}

float limitOutput (float sample) noexcept
{
    // Single notes stay linear. Dense chords approach, but never exceed, 0.20.
    constexpr auto knee = 0.12f;
    constexpr auto headroom = 0.08f;
    const auto magnitude = std::abs (sample);
    if (magnitude <= knee)
        return sample;
    return std::copysign (knee + headroom * std::tanh ((magnitude - knee) / headroom), sample);
}
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "ShepardKeyboard", createParameters())
{
    volumeDb = parameters.getRawParameterValue ("volume");
    for (int i = 0; i < 32; ++i)
        synth.addVoice (new ShepardVoice());
    synth.addSound (new ShepardSound());
    // Ignore excess notes instead of cutting off an active waveform abruptly.
    synth.setNoteStealingEnabled (false);
    synth.setMinimumRenderingSubdivisionSize (1, true);
}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.allNotesOff (0, false);
    keyboardState.reset();
    synth.setCurrentPlaybackSampleRate (sampleRate);
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volumeDb->load()));
}

void AudioPluginAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
    keyboardState.reset();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    midi.clear();

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (volumeDb->load()));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto gain = outputGain.getNextValue();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, limitOutput (buffer.getSample (channel, sample) * gain));
    }
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& data)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, data);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
