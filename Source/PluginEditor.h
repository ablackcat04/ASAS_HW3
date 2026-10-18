#pragma once

#include "PluginProcessor.h"

class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::MidiKeyboardComponent keyboard;
    juce::Slider volume;
    juce::Label volumeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
