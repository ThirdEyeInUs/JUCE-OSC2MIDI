#pragma once

#include <JuceHeader.h>

using CCValueChangedCallback = std::function<void(int ccNumber, float value)>;

class CCControlWindow : public juce::DocumentWindow
{
public:
    CCControlWindow(CCValueChangedCallback callback = nullptr)
        : DocumentWindow("CC Control Window",
            juce::Colours::darkgrey,
            DocumentWindow::allButtons),
        ccValueChangedCallback(callback),
        contentComponent(ccValueChangedCallback)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(400, 300, 1600, 1200); // Minimum and maximum sizes
        setContentOwned(&contentComponent, true);
        contentComponent.setSize(defaultWidth, defaultHeight);
        centreWithSize(getWidth(), getHeight());
    }

    ~CCControlWindow() override
    {
        setContentOwned(nullptr, true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    static constexpr int defaultWidth = 1200; // Updated default size
    static constexpr int defaultHeight = 800;

    CCValueChangedCallback ccValueChangedCallback;

    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent(CCValueChangedCallback callback)
            : ccValueChangedCallback(callback), currentOSCChannel(1)
        {
            // Label for OSC Channel
            oscChLabel.setText("OSC CH:", juce::dontSendNotification);
            oscChLabel.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(oscChLabel);

            // OSC Channel Selector
            channelSelector.addItemList({ "1", "2", "3", "4", "5", "6", "7", "8",
                                          "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
            channelSelector.setSelectedId(1);
            channelSelector.onChange = [this]()
                {
                    currentOSCChannel = channelSelector.getSelectedId();
                    juce::Logger::writeToLog("Selected OSC Channel: " + juce::String(currentOSCChannel));
                };
            addAndMakeVisible(channelSelector);

            // Reset Size Button
            resetSizeButton.setButtonText("Reset Size");
            resetSizeButton.onClick = [this]()
                {
                    if (auto* parent = dynamic_cast<CCControlWindow*>(getTopLevelComponent()))
                    {
                        parent->setBounds(juce::Rectangle<int>(0, 0, defaultWidth, defaultHeight)
                            .withCentre(parent->getScreenBounds().getCentre()));
                    }
                };
            addAndMakeVisible(resetSizeButton);

            // Reset CC Button
            resetCCButton.setButtonText("Reset CC");
            resetCCButton.onClick = [this]()
                {
                    // Reset all CC values to default (64)
                    for (auto* dial : radials)
                        dial->setValue(64.0);
                    for (auto* slider : sliders)
                        slider->setValue(64.0);
                };
            addAndMakeVisible(resetCCButton);

            // Initialize 24 Radial Dials (CC1 - CC24)
            for (int i = 0; i < 24; ++i)
            {
                int ccNumber = i + 1;
                auto* dial = new juce::Slider();
                dial->setSliderStyle(juce::Slider::Rotary);
                dial->setRange(0.0, 127.0, 1.0);
                dial->setValue(64.0);

                // Numeric value box below
                dial->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);

                dial->setName("CC" + juce::String(ccNumber));
                dial->onValueChange = [this, i, dial, ccNumber]()
                    {
                        float value = (float)dial->getValue();
                        if (ccValueChangedCallback)
                            ccValueChangedCallback(ccNumber, value);
                        sendCCMessage(currentOSCChannel, ccNumber, value);
                    };

                if (ccNumber >= 13)
                    dial->setColour(juce::Slider::thumbColourId, juce::Colours::orange);
                else
                    dial->setColour(juce::Slider::thumbColourId, juce::Colours::red);

                addAndMakeVisible(dial);
                radials.add(dial);

                // Label above dial shows CC name
                auto* rLabel = new juce::Label();
                rLabel->setText(dial->getName(), juce::dontSendNotification);
                rLabel->setJustificationType(juce::Justification::centred);
                addAndMakeVisible(rLabel);
                radialLabels.add(rLabel);
            }

            // Initialize 12 Sliders (CC25 - CC36)
            for (int i = 0; i < 12; ++i)
            {
                int ccNumber = i + 25;
                auto* slider = new juce::Slider();
                slider->setSliderStyle(juce::Slider::LinearHorizontal);
                slider->setRange(0.0, 127.0, 1.0);
                slider->setValue(64.0);

                // Numeric value box below
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);

                slider->setName("CC" + juce::String(ccNumber));
                slider->onValueChange = [this, i, slider, ccNumber]()
                    {
                        float value = (float)slider->getValue();
                        if (ccValueChangedCallback)
                            ccValueChangedCallback(ccNumber, value);
                        sendCCMessage(currentOSCChannel, ccNumber, value);
                    };

                if (ccNumber <= 30)
                    slider->setColour(juce::Slider::thumbColourId, juce::Colours::blue);
                else
                    slider->setColour(juce::Slider::thumbColourId, juce::Colours::green);

                addAndMakeVisible(slider);
                sliders.add(slider);

                // Label above slider shows CC name
                auto* sLabel = new juce::Label();
                sLabel->setText(slider->getName(), juce::dontSendNotification);
                sLabel->setJustificationType(juce::Justification::centred);
                addAndMakeVisible(sLabel);
                sliderLabels.add(sLabel);
            }
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(20);

            // Top row: OSC Channel selector and buttons
            auto topRow = area.removeFromTop(40);
            oscChLabel.setBounds(topRow.removeFromLeft(60));
            channelSelector.setBounds(topRow.removeFromLeft(100));
            resetSizeButton.setBounds(topRow.removeFromRight(100));
            resetCCButton.setBounds(topRow.removeFromRight(100));

            // Dials Area (CC1 - CC24)
            int totalDialRows = 4;
            int totalDialColumns = 6;
            auto dialsArea = area.removeFromTop(getHeight() * 0.6);
            int dialWidth = dialsArea.getWidth() / totalDialColumns;
            int dialHeight = dialsArea.getHeight() / totalDialRows;

            for (int i = 0; i < radials.size(); ++i)
            {
                int row = i / totalDialColumns;
                int col = i % totalDialColumns;
                auto x = dialsArea.getX() + col * dialWidth;
                auto y = dialsArea.getY() + row * dialHeight;

                radials[i]->setBounds(x, y + 20, dialWidth, dialHeight - 30);
                radialLabels[i]->setBounds(x, y, dialWidth, 20);
            }

            // Sliders Area (CC25 - CC36)
            int totalSliderRows = 2;
            int totalSliderColumns = 6;
            auto slidersArea = area;
            int sliderWidth = slidersArea.getWidth() / totalSliderColumns;
            int sliderHeight = slidersArea.getHeight() / totalSliderRows;

            for (int i = 0; i < sliders.size(); ++i)
            {
                int row = i / totalSliderColumns;
                int col = i % totalSliderColumns;
                auto x = slidersArea.getX() + col * sliderWidth;
                auto y = slidersArea.getY() + row * sliderHeight;

                sliders[i]->setBounds(x, y + 20, sliderWidth, sliderHeight - 20);
                sliderLabels[i]->setBounds(x, y, sliderWidth, 20);
            }
        }

    private:
        static constexpr int defaultWidth = 1200;
        static constexpr int defaultHeight = 800;

        juce::Label oscChLabel;
        juce::ComboBox channelSelector;
        juce::TextButton resetSizeButton;
        juce::TextButton resetCCButton;

        juce::OwnedArray<juce::Slider> radials;
        juce::OwnedArray<juce::Label> radialLabels;

        juce::OwnedArray<juce::Slider> sliders;
        juce::OwnedArray<juce::Label> sliderLabels;

        int currentOSCChannel;
        CCValueChangedCallback ccValueChangedCallback;

        void sendCCMessage(int channel, int ccNumber, float value)
        {
            juce::String ccAddress = "/ch" + juce::String(channel) + "cc";
            juce::String ccValueAddress = "/ch" + juce::String(channel) + "ccvalue";

            float normalizedValue = value / 127.0f;
            juce::Logger::writeToLog("OSC Sent: " + ccAddress + " " + juce::String(ccNumber));
            juce::Logger::writeToLog("OSC Sent: " + ccValueAddress + " " + juce::String(normalizedValue));
        }
    };

    ContentComponent contentComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CCControlWindow)
};
