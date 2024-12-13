// StateComponent.h
#pragma once

#include <JuceHeader.h>

class StateComponent : public juce::Component
{
public:
    StateComponent(int stateId)
    {
        // Initialize components based on stateId
        switch (stateId)
        {
        case 0:
            label.setText("State 0", juce::dontSendNotification);
            break;
        case 1:
            label.setText("State 1", juce::dontSendNotification);
            break;
        case 2:
            label.setText("State 2", juce::dontSendNotification);
            break;
        case 3:
            label.setText("State 3", juce::dontSendNotification);
            break;
        case 4:
            label.setText("State 4", juce::dontSendNotification);
            break;
        default:
            label.setText("Unknown State", juce::dontSendNotification);
            break;
        }

        label.setJustificationType(juce::Justification::centred);
        label.setFont(20.0f);
        addAndMakeVisible(label);

        // Add more components specific to each state as needed
    }

    void resized() override
    {
        label.setBounds(getLocalBounds());
    }

private:
    juce::Label label;
};
