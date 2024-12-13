#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

// Define a callback type for handling value changes
using MixerValueChangedCallback = std::function<void(const juce::String&, int, float)>;

class MixerControlWindow : public juce::DocumentWindow
{
public:
    MixerControlWindow(MixerValueChangedCallback callback);
    ~MixerControlWindow() override;

    void closeButtonPressed() override;

private:
    class MixerComponent : public juce::Component
    {
    public:
        MixerComponent(MixerValueChangedCallback callback);

        void resized() override;
        void toggleChannelCount();

    private:
        void addChannels(int numChannels);

        juce::TextButton resetButton;

        MixerValueChangedCallback valueChangedCallback;

        juce::OwnedArray<juce::Slider> gainSliders;
        juce::OwnedArray<juce::Slider> reverbSliders;
        juce::OwnedArray<juce::Slider> panDials;
        juce::OwnedArray<juce::Label> channelLabels;

        bool is16Channels;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
    };

    MixerComponent mixerComponent;

    MixerValueChangedCallback valueChangedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerControlWindow)
};
