#pragma once

#include <JuceHeader.h>

class SideMenu : public juce::Component
{
public:
    SideMenu()
    {
        // Initialize menu buttons
        addAndMakeVisible(buttonCC);
        buttonCC.setButtonText("CC");
        buttonCC.onClick = [this]() { handleMenuClick(1); };

        addAndMakeVisible(buttonFileBrowser);
        buttonFileBrowser.setButtonText("File Browser");
        buttonFileBrowser.onClick = [this]() { handleMenuClick(2); };

        addAndMakeVisible(buttonMixer);
        buttonMixer.setButtonText("Mixer");
        buttonMixer.onClick = [this]() { handleMenuClick(3); };

        // Initialize Close Button
        addAndMakeVisible(closeButton);
        closeButton.setButtonText(juce::String::fromUTF8("✕"));
        closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::lightcoral);
        closeButton.setTooltip("Close Menu");
    }

    void setActiveButton(juce::TextButton* button)
    {
        activeButton = button;

        // Update button colors to highlight the active button
        buttonCC.setColour(juce::TextButton::buttonColourId, button == &buttonCC ? juce::Colours::yellow : juce::Colours::grey);
        buttonFileBrowser.setColour(juce::TextButton::buttonColourId, button == &buttonFileBrowser ? juce::Colours::yellow : juce::Colours::grey);
        buttonMixer.setColour(juce::TextButton::buttonColourId, button == &buttonMixer ? juce::Colours::yellow : juce::Colours::grey);

        repaint();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        int buttonHeight = 40;

        // Layout buttons
        buttonCC.setBounds(area.removeFromTop(buttonHeight).reduced(0, 5));
        buttonFileBrowser.setBounds(area.removeFromTop(buttonHeight).reduced(0, 5));
        buttonMixer.setBounds(area.removeFromTop(buttonHeight).reduced(0, 5));

        // Position close button
        closeButton.setBounds(area.removeFromTop(buttonHeight).withWidth(30).withPosition(area.getRight() - 40, 10));
    }

    // Callback to notify MainComponent when a menu item is clicked
    std::function<void(int)> onMenuItemClicked;

    // Make closeButton public for MainComponent to access
    juce::TextButton closeButton;

private:
    void handleMenuClick(int menuId)
    {
        if (onMenuItemClicked)
            onMenuItemClicked(menuId);

        switch (menuId)
        {
        case 1: setActiveButton(&buttonCC); break;
        case 2: setActiveButton(&buttonFileBrowser); break;
        case 3: setActiveButton(&buttonMixer); break;
        default: break;
        }
    }

    juce::TextButton buttonCC{ "CC" };
    juce::TextButton buttonFileBrowser{ "File Browser" };
    juce::TextButton buttonMixer{ "Mixer" };

    juce::TextButton* activeButton = nullptr; // Tracks the currently active button
};
