#pragma once

#include <JuceHeader.h>

class FileBrowserWindow : public juce::DocumentWindow
{
public:
    using FileSelectedCallback = std::function<void(const juce::File&)>;

    FileBrowserWindow(FileSelectedCallback callback)
        : DocumentWindow("File Browser",
            juce::Colours::lightgrey,
            DocumentWindow::allButtons),
        fileSelectedCallback(std::move(callback))
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(600, 400, 1200, 800);

        setContentOwned(&contentComponent, true);
        contentComponent.setSize(600, 400);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent(FileSelectedCallback callback)
            : fileSelectedCallback(std::move(callback))
        {
            addAndMakeVisible(openButton);
            openButton.setButtonText("Open File");
            openButton.onClick = [this]() { openFile(); };

            addAndMakeVisible(filePathLabel);
            filePathLabel.setText("No file selected", juce::dontSendNotification);
            filePathLabel.setJustificationType(juce::Justification::centredLeft);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(10);
            openButton.setBounds(area.removeFromTop(40).reduced(0, 5));
            filePathLabel.setBounds(area.removeFromTop(30));
        }

        void openFile()
        {
            auto* chooser = new juce::FileChooser(
                "Select a file to open...",
                juce::File(),
                "*");

            chooser->launchAsync(juce::FileBrowserComponent::openMode,
                [this, chooser](const juce::FileChooser&)
                {
                    auto result = chooser->getResult();
                    if (result.existsAsFile())
                    {
                        filePathLabel.setText(result.getFullPathName(), juce::dontSendNotification);
                        if (fileSelectedCallback)
                            fileSelectedCallback(result);
                    }
                    else
                    {
                        filePathLabel.setText("No file selected", juce::dontSendNotification);
                    }

                    delete chooser;
                });
        }

    private:
        juce::TextButton openButton{ "Open" };
        juce::Label filePathLabel{ "Path", "No file selected" };
        FileSelectedCallback fileSelectedCallback;
    };

    ContentComponent contentComponent{ fileSelectedCallback };
    FileSelectedCallback fileSelectedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBrowserWindow)
};
