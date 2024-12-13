#include "MixerControlWindow.h"

MixerControlWindow::MixerControlWindow(MixerValueChangedCallback callback)
    : DocumentWindow("Mixer Control", juce::Colours::darkgrey, DocumentWindow::allButtons),
    valueChangedCallback(callback),
    mixerComponent(callback)
{
    setUsingNativeTitleBar(true);
    setContentOwned(&mixerComponent, true);
    setResizable(true, true);
    setResizeLimits(600, 400, 1200, 800);
    centreWithSize(getWidth(), getHeight());
}

MixerControlWindow::~MixerControlWindow()
{
    setContentOwned(nullptr, true);
}

void MixerControlWindow::closeButtonPressed()
{
    setVisible(false);
}

MixerControlWindow::MixerComponent::MixerComponent(MixerValueChangedCallback callback)
    : valueChangedCallback(callback), is16Channels(false)
{
    resetButton.setButtonText("Reset CC");
    resetButton.onClick = [this]()
        {
            for (auto* slider : gainSliders) slider->setValue(0.5);
            for (auto* slider : reverbSliders) slider->setValue(0.0);
            for (auto* dial : panDials) dial->setValue(0.0);
        };
    addAndMakeVisible(resetButton);

    addChannels(8);
}

void MixerControlWindow::MixerComponent::addChannels(int numChannels)
{
    gainSliders.clear();
    reverbSliders.clear();
    panDials.clear();
    channelLabels.clear();

    for (int i = 0; i < numChannels; ++i)
    {
        auto* gainSlider = new juce::Slider();
        gainSlider->setSliderStyle(juce::Slider::LinearVertical);
        gainSlider->setRange(0.0, 1.0, 0.01);
        gainSlider->setValue(0.5);
        gainSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        gainSlider->onValueChange = [this, i]()
            {
                if (valueChangedCallback)
                    valueChangedCallback("Gain", i + 1, gainSliders[i]->getValue());
            };
        addAndMakeVisible(gainSlider);
        gainSliders.add(gainSlider);

        auto* reverbSlider = new juce::Slider();
        reverbSlider->setSliderStyle(juce::Slider::LinearVertical);
        reverbSlider->setRange(0.0, 1.0, 0.01);
        reverbSlider->setValue(0.0);
        reverbSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        reverbSlider->onValueChange = [this, i]()
            {
                if (valueChangedCallback)
                    valueChangedCallback("Reverb", i + 1, reverbSliders[i]->getValue());
            };
        addAndMakeVisible(reverbSlider);
        reverbSliders.add(reverbSlider);

        auto* panDial = new juce::Slider();
        panDial->setSliderStyle(juce::Slider::Rotary);
        panDial->setRange(-1.0, 1.0, 0.01);
        panDial->setValue(0.0);
        panDial->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        panDial->onValueChange = [this, i]()
            {
                if (valueChangedCallback)
                    valueChangedCallback("Pan", i + 1, panDials[i]->getValue());
            };
        addAndMakeVisible(panDial);
        panDials.add(panDial);

        auto* channelLabel = new juce::Label();
        channelLabel->setText("Channel " + juce::String(i + 1), juce::dontSendNotification);
        channelLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(channelLabel);
        channelLabels.add(channelLabel);
    }

    resized();
}

void MixerControlWindow::MixerComponent::toggleChannelCount()
{
    is16Channels = !is16Channels;
    addChannels(is16Channels ? 16 : 8);
}

void MixerControlWindow::MixerComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto topArea = area.removeFromTop(40);
    resetButton.setBounds(topArea.removeFromRight(100)); // Place reset button at the top-right

    int columnWidth = area.getWidth() / gainSliders.size();

    for (int i = 0; i < gainSliders.size(); ++i)
    {
        auto columnArea = area.removeFromLeft(columnWidth).reduced(5);
        channelLabels[i]->setBounds(columnArea.removeFromTop(20));
        gainSliders[i]->setBounds(columnArea.removeFromTop(columnArea.getHeight() / 3));
        reverbSliders[i]->setBounds(columnArea.removeFromTop(columnArea.getHeight() / 2));
        panDials[i]->setBounds(columnArea);
    }
}
