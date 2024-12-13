#pragma once

#include <JuceHeader.h>

class HoverableTextButton : public juce::TextButton
{
public:
    HoverableTextButton(const juce::String& buttonName) : juce::TextButton(buttonName) {}

    std::function<void()> onMouseEnterCallback;
    std::function<void()> onMouseExitCallback;

    void mouseEnter(const juce::MouseEvent&) override
    {
        if (onMouseEnterCallback)
            onMouseEnterCallback();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (onMouseExitCallback)
            onMouseExitCallback();
    }
};
