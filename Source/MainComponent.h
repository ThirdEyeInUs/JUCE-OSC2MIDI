#pragma once

#include <JuceHeader.h>
#include <set>
#include "SideMenu.h"            // SideMenu UI
#include "CustomLookAndFeel.h"   // Custom LookAndFeel for the Hamburger Button
#include "CCControlWindow.h"     // Optional: Pop-up window for sending CC messages
#include "FileBrowserWindow.h"   // Optional: Pop-up window with file browser
#include "MixerControlWindow.h"  // Optional: Pop-up window for mixer controls

//==============================================================================
// A custom ListBoxModel to display logs efficiently.
class LogListModel : public juce::ListBoxModel
{
public:
    void addLog(const juce::String& log)
    {
        logs.add(log);
    }

    int getNumRows() override
    {
        return logs.size();
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= logs.size())
            return;

        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);

        g.setColour(juce::Colours::black);
        g.drawText(logs[rowNumber], 5, 0, width - 10, height, juce::Justification::centredLeft, true);
    }

    void clearLogs()
    {
        logs.clear();
    }

private:
    juce::Array<juce::String> logs;
};

//==============================================================================
// MainComponent handles all GUI elements, MIDI I/O, OSC I/O, side menu, ARP, etc.
class MainComponent
    : public juce::Component,
    private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>,
    private juce::MidiKeyboardStateListener,
    private juce::MidiInputCallback,
    private juce::AsyncUpdater,
    private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Called when an OSC message is received
    void oscMessageReceived(const juce::OSCMessage& message) override;

private:
    //==================================================================
    // Simple struct for capturing MIDI events so we can process them later.
    struct MidiEvent
    {
        enum class Type
        {
            NoteOn,
            NoteOff,
            ControlChange,
            PitchBend,
            Aftertouch
        } type;

        int channel;      // MIDI channel (1-16)
        int parameter;    // Note number or CC number, etc.
        float value;      // Velocity, CC value, pitch bend value, aftertouch, etc.
    };

    //==================================================================
    // UI components:

    // Labels and text entries for IP/Port In/Out
    juce::Label     ipInLabel, portInLabel, ipOutLabel, portOutLabel;
    juce::TextEditor ipInEntry, portInEntry, ipOutEntry, portOutEntry;

    // Button to start/stop the OSC server
    juce::TextButton startButton;

    // MIDI input & output controls
    juce::Label     midiInputLabel, midiOutputLabel;
    juce::ComboBox  midiInputComboBox, midiOutputComboBox;

    // The MIDI keyboard component
    juce::MidiKeyboardState midiKeyboardState;
    juce::MidiKeyboardComponent midiKeyboard{ midiKeyboardState,
                                               juce::MidiKeyboardComponent::horizontalKeyboard };

    // Logging listbox and model
    juce::ListBox   log_list_box;
    LogListModel    logListModel;

    // ARP UI:
    juce::TextButton arpButton{ "Arp Generator" };
    juce::Slider     arpSpeedSlider;

    // “Hold” button
    juce::TextButton holdButton{ "Hold" };

    // Channel selection for OSC notes
    juce::Label     channelLabel{ "channelLabel", "OSC Channel:" };
    juce::ComboBox  oscChannelComboBox;

    // CC controls
    juce::Label     ccChannelLabel{ "ccChannelLabel", "CC Channel:" };
    juce::ComboBox  ccChannelComboBox;
    juce::Label     ccNumberLabel{ "ccNumberLabel", "CC Number:" };
    juce::ComboBox  ccNumberComboBox;
    juce::Label     ccValueLabel{ "ccValueLabel", "CC Value:" };
    juce::Slider    ccValueSlider;

    // Pitch Bend controls
    juce::Label     pitchBendLabel{ "pitchBendLabel", "Pitch Bend:" };
    juce::Slider    pitchBendSlider;

    // Channel Pressure controls
    juce::Label     channelPressureLabel{ "channelPressureLabel", "Channel Pressure:" };
    juce::Slider    channelPressureSlider;

    // OSC sender and receiver
    juce::OSCSender    oscSender;
    juce::OSCReceiver  oscReceiver;
    bool               oscConnected = false;

    // Currently chosen MIDI in/out devices
    std::unique_ptr<juce::MidiInput>  currentMidiInput;
    std::unique_ptr<juce::MidiOutput> currentMidiOutput;

    // Device identifiers for populating combo boxes
    juce::StringArray midiInputIdentifiers;
    juce::StringArray midiOutputIdentifiers;

    //==================================================================
    // Thread-safety and queues:
    juce::Array<MidiEvent>  midiEventsQueue;
    juce::CriticalSection    queueLock;

    // Keep track of active notes so we avoid duplicates
    std::set<int> activeNotes;
    juce::CriticalSection activeNotesLock;

    // Pending log messages, for batch processing
    juce::String          pendingLogMessages;
    juce::CriticalSection logLock;

    //==================================================================
    // ARP variables:
    bool  arpEnabled = false;
    bool  goingUp = true;
    int   currentArpIndex = 0;
    int   lastArpNote = -1;
    double arpRateHz = 5.0;   // ARP stepping speed in Hz

    juce::SortedSet<int> heldNotes;  // notes held down
    bool holdEnabled = false;

    // Current OSC channel (1-16)
    int currentOSCChannel = 1;

    // Current CC channel & number
    int currentCCChannel = 1;
    int currentCCNumber = 1;

    //==================================================================
    // Side menu UI and animation:

    SideMenu sideMenu;
    juce::TextButton hamburgerButton;

    bool isSideMenuVisible = false;
    bool isAnimating = false;
    int  targetSideMenuWidth = 275; // The side menu’s max width
    int  currentSideMenuWidth = 0;
    int  animationStep = 25;  // How many pixels to expand/collapse per timer callback

    // Custom LookAndFeel for hamburger button
    CustomLookAndFeel customLookAndFeel;

    // Optional windows triggered by side menu
    std::unique_ptr<CCControlWindow>    ccControlWindow;
    std::unique_ptr<FileBrowserWindow>  fileBrowserWindow;
    std::unique_ptr<MixerControlWindow> mixerControlWindow;

    //==================================================================
    // Internal helpers:

    void handleMenuItemClick(int menuItemId);
    void logMessage(const juce::String& message);

    // OSC server start/stop
    void startOSCServer();
    void stopOSCServer();

    // MIDI device updates
    void updateMidiDevices();
    void setMidiInput(const juce::String& identifier);
    void setMidiOutput(const juce::String& identifier);

    // Sending messages
    void sendOSCMessage(int midiNote, bool noteOn);
    void sendVelocityMessage(int midiNote, float velocity);
    void sendCCMessage(int channel, int ccNumber, int ccValue);
    void sendPitchBendMessage(int channel, float pitchValue);
    void sendAftertouchMessage(int channel, int pressureValue);

    // Handling incoming OSC
    void handleIncomingOSCMessage(const juce::OSCMessage& message);

    // MIDI callbacks
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;

    // AsyncUpdater callback
    void handleAsyncUpdate() override;

    // Timer callback (ARP stepping + side menu animation)
    void timerCallback() override;

    // ARP helpers
    void updateArpState();
    void advanceArp();
    void sendArpNoteOn(int noteNumber, float velocity = 1.0f);
    void sendArpNoteOff(int noteNumber);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
