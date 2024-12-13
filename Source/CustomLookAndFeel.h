#pragma once

#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Customize colors, fonts, etc.
        setColour(juce::TextButton::buttonColourId, juce::Colours::darkslategrey);
        setColour(juce::TextButton::buttonOnColourId, juce::Colours::teal);
        // Add more customizations as needed
    }

    // Override other LookAndFeel methods if needed
};
