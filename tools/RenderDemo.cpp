#include "../Source/PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
struct Event
{
    int sample;
    juce::MidiMessage message;
};

juce::AudioBuffer<float> render (double rate, int channels, int length,
                                const std::vector<Event>& events, bool maximumVolume = false,
                                int blockSize = 256)
{
    AudioPluginAudioProcessor processor;
    if (maximumVolume)
    {
        auto* volume = processor.parameters.getParameter ("volume");
        volume->setValueNotifyingHost (volume->convertTo0to1 (-18.0f));
    }
    processor.prepareToPlay (rate, blockSize);
    juce::AudioBuffer<float> result (channels, length);
    size_t eventIndex = 0;
    for (int position = 0; position < length; position += blockSize)
    {
        const auto count = std::min (blockSize, length - position);
        juce::AudioBuffer<float> block (channels, count);
        juce::MidiBuffer midi;
        while (eventIndex < events.size() && events[eventIndex].sample < position + count)
        {
            midi.addEvent (events[eventIndex].message, events[eventIndex].sample - position);
            ++eventIndex;
        }
        processor.processBlock (block, midi);
        for (int channel = 0; channel < channels; ++channel)
            result.copyFrom (channel, position, block, channel, 0, count);
    }
    processor.releaseResources();
    return result;
}

float checkLevels (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = buffer.getSample (channel, sample);
            if (! std::isfinite (value) || std::abs (value) > 0.200001f)
                throw std::runtime_error ("Non-finite sample or output ceiling exceeded");
            peak = std::max (peak, std::abs (value));
        }
    return peak;
}

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer, double rate)
{
    if (file.existsAsFile() && ! file.deleteFile())
        throw std::runtime_error ("Cannot replace WAV");
    auto stream = file.createOutputStream();
    if (! stream)
        throw std::runtime_error ("Cannot create WAV");
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        format.createWriterFor (stream.get(), rate,
                                static_cast<unsigned int> (buffer.getNumChannels()), 24, {}, 0));
    if (! writer)
        throw std::runtime_error ("Cannot create WAV writer");
    stream.release();
    if (! writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()))
        throw std::runtime_error ("Cannot write WAV");
    std::cout << file.getFileName() << ": peak " << checkLevels (buffer)
              << " (" << juce::Decibels::gainToDecibels (checkLevels (buffer)) << " dBFS)\n";
}

void renderScale (const juce::File& directory, bool descending)
{
    constexpr double rate = 48000.0;
    constexpr int step = 15360; // 320 ms, with a short gap after the release.
    constexpr int gate = 9600;  // 200 ms sustain; no automatic sound in the plugin.
    std::vector<Event> events;
    for (int i = 0; i < 37; ++i)
    {
        const auto note = descending ? 84 - i : 48 + i;
        events.push_back ({ i * step, juce::MidiMessage::noteOn (1, note, 0.8f) });
        events.push_back ({ i * step + gate, juce::MidiMessage::noteOff (1, note) });
    }
    const auto buffer = render (rate, 2, 37 * step + 24000, events);
    writeWav (directory.getChildFile (descending ? "shepard-descending.wav" : "shepard-ascending.wav"), buffer, rate);
}

void verifyOffline()
{
    float highestPeak = 0.0f;
    for (const auto rate : { 22050.0, 44100.0, 48000.0, 96000.0 })
    {
        const auto length = static_cast<int> (rate);
        const auto noteOff = static_cast<int> (rate * 0.5);
        const auto first = render (rate, 1, length,
                                  { { 0, juce::MidiMessage::noteOn (1, 60, 1.0f) },
                                    { noteOff, juce::MidiMessage::noteOff (1, 60) } });
        const auto octave = render (rate, 1, length,
                                   { { 0, juce::MidiMessage::noteOn (1, 72, 1.0f) },
                                     { noteOff, juce::MidiMessage::noteOff (1, 72) } });
        for (int sample = 0; sample < length; ++sample)
            if (first.getSample (0, sample) != octave.getSample (0, sample))
                throw std::runtime_error ("Octave-equivalent notes differ");
        if (first.getMagnitude (0, length - 1024, 1024) != 0.0f)
            throw std::runtime_error ("Note release did not reach silence");

        std::vector<Event> chordEvents;
        for (int note = 48; note < 80; ++note)
            chordEvents.push_back ({ 0, juce::MidiMessage::noteOn (1, note, 1.0f) });
        chordEvents.push_back ({ noteOff, juce::MidiMessage::allNotesOff (1) });
        const auto chord = render (rate, 2, length, chordEvents, true, 127);
        highestPeak = std::max (highestPeak, checkLevels (chord));
        if (chord.getMagnitude (0, length - 1024, 1024) != 0.0f)
            throw std::runtime_error ("All-notes-off did not reach silence");
    }
    std::cout << "Offline checks: octave identity, released silence, finite samples, "
                 "mono/stereo at 22.05/44.1/48/96 kHz. 32-note max-volume peak: "
              << highestPeak << " (" << juce::Decibels::gainToDecibels (highestPeak) << " dBFS).\n";
}

void writeEditorPreview (const juce::File& directory)
{
    AudioPluginAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
    juce::MemoryOutputStream imageBytes;
    if (! juce::PNGImageFormat().writeImageToStream (image, imageBytes)
        || ! directory.getChildFile ("keyboard.png").replaceWithData (imageBytes.getData(), imageBytes.getDataSize()))
        throw std::runtime_error ("Cannot save editor preview");
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    try
    {
        const auto directory = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                        : juce::File::getCurrentWorkingDirectory().getChildFile ("output/audio");
        if (! directory.createDirectory())
            throw std::runtime_error ("Cannot create output directory");
        verifyOffline();
        renderScale (directory, false);
        renderScale (directory, true);
        writeEditorPreview (directory);
        std::cout << "WAVs written to " << directory.getFullPathName() << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
