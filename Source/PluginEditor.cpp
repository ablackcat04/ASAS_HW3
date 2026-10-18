#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      keyboard (processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      volumeAttachment (processor.parameters, "volume", volume)
{
    keyboard.setAvailableRange (48, 96);
    keyboard.setLowestVisibleKey (48);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setKeyWidth (20.0f);
    keyboard.setKeyPressBaseOctave (5);
    keyboard.setVelocity (0.8f, false);
    keyboard.setWantsKeyboardFocus (true);
    keyboard.clearKeyMappings();
    const juce::String keys = "awsedftgyhujk";
    for (int i = 0; i < keys.length(); ++i)
        keyboard.setKeyPressForNote (juce::KeyPress (static_cast<int> (keys[i])), i);
    addAndMakeVisible (keyboard);

    volume.setSliderStyle (juce::Slider::LinearHorizontal);
    volume.setTextBoxStyle (juce::Slider::TextBoxRight, false, 74, 22);
    volume.setTextValueSuffix (" dB");
    volume.setWantsKeyboardFocus (false);
    addAndMakeVisible (volume);
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    volumeLabel.attachToComponent (&volume, true);
    addAndMakeVisible (volumeLabel);
    setSize (620, 180);
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff202329));
    g.setColour (juce::Colours::whitesmoke);
    g.setFont (juce::FontOptions (19.0f));
    g.drawText ("Shepard Keyboard", 20, 12, 240, 25, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffbbc0c8));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Click keys or use A W S E D F T G Y H U J K. Each octave repeats.",
                20, 42, 580, 22, juce::Justification::centredLeft);
}

void AudioPluginAudioProcessorEditor::resized()
{
    volume.setBounds (350, 14, 250, 24);
    keyboard.setBounds (20, 76, 580, 84);
}
