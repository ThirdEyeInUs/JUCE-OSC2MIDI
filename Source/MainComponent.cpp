#include "MainComponent.h"
#include "CustomLookAndFeel.h"

//==============================================================================
MainComponent::MainComponent()
    : midiKeyboard(midiKeyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(1200, 700); // Larger default size

    //========================================================
    // IP and Port In
    addAndMakeVisible(ipInLabel);
    ipInLabel.setText("IP for OSC In:", juce::dontSendNotification);

    addAndMakeVisible(ipInEntry);
    ipInEntry.setText("127.0.0.1");

    addAndMakeVisible(portInLabel);
    portInLabel.setText("Port for OSC In:", juce::dontSendNotification);

    addAndMakeVisible(portInEntry);
    portInEntry.setText("5550");

    //========================================================
    // IP and Port Out
    addAndMakeVisible(ipOutLabel);
    ipOutLabel.setText("IP for OSC Out:", juce::dontSendNotification);

    addAndMakeVisible(ipOutEntry);
    ipOutEntry.setText("127.0.0.1");

    addAndMakeVisible(portOutLabel);
    portOutLabel.setText("Port for OSC Out:", juce::dontSendNotification);

    addAndMakeVisible(portOutEntry);
    portOutEntry.setText("3330");

    //========================================================
    // Start/Stop Button
    addAndMakeVisible(startButton);
    startButton.setButtonText("Start");
    startButton.onClick = [this]()
        {
            if (startButton.getButtonText() == "Start")
            {
                startOSCServer();
                startButton.setButtonText("Stop");
            }
            else
            {
                stopOSCServer();
                startButton.setButtonText("Start");
            }
        };

    //========================================================
    // MIDI Input label & combo
    addAndMakeVisible(midiInputLabel);
    midiInputLabel.setText("MIDI Input:", juce::dontSendNotification);

    addAndMakeVisible(midiInputComboBox);
    midiInputComboBox.onChange = [this]()
        {
            int selectedId = midiInputComboBox.getSelectedId();
            if (selectedId > 0 && selectedId <= midiInputIdentifiers.size())
                setMidiInput(midiInputIdentifiers[selectedId - 1]);
        };

    //========================================================
    // MIDI Output label & combo
    addAndMakeVisible(midiOutputLabel);
    midiOutputLabel.setText("MIDI Output:", juce::dontSendNotification);

    addAndMakeVisible(midiOutputComboBox);
    midiOutputComboBox.onChange = [this]()
        {
            int selectedId = midiOutputComboBox.getSelectedId();
            if (selectedId > 0 && selectedId <= midiOutputIdentifiers.size())
                setMidiOutput(midiOutputIdentifiers[selectedId - 1]);
        };

    //========================================================
    // MIDI Keyboard
    addAndMakeVisible(midiKeyboard);
    midiKeyboardState.addListener(this);

    //========================================================
    // Log ListBox
    addAndMakeVisible(log_list_box);
    log_list_box.setModel(&logListModel);
    log_list_box.setRowHeight(20);
    log_list_box.setMultipleSelectionEnabled(false);

    //========================================================
    // ARP UI
    addAndMakeVisible(arpButton);
    arpButton.setClickingTogglesState(true);
    arpButton.setToggleState(false, juce::dontSendNotification);
    arpButton.onClick = [this]()
        {
            arpEnabled = arpButton.getToggleState();
            logMessage(arpEnabled ? "ARP Enabled" : "ARP Disabled");
            updateArpState();
        };

    addAndMakeVisible(arpSpeedSlider);
    arpSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    arpSpeedSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    arpSpeedSlider.setRange(0.0, 1.0, 0.01);
    arpSpeedSlider.setValue(0.5);
    arpSpeedSlider.onValueChange = [this]() noexcept
        {
            double speedVal = arpSpeedSlider.getValue();
            // Map [0.0..1.0] to [0.1..20] Hz
            arpRateHz = 0.1 + speedVal * 19.9;
            if (arpEnabled)
                startTimerHz(static_cast<int>(arpRateHz));
        };

    //========================================================
    // Hold button
    addAndMakeVisible(holdButton);
    holdButton.setClickingTogglesState(true);
    holdButton.setToggleState(false, juce::dontSendNotification);
    holdButton.onClick = [this]()
        {
            bool oldHold = holdEnabled;
            holdEnabled = holdButton.getToggleState();
            logMessage(holdEnabled ? "Hold Enabled" : "Hold Disabled");

            if (!holdEnabled)
            {
                // Clear held notes if hold is turned off
                heldNotes.clear();
                if (lastArpNote >= 0)
                {
                    sendArpNoteOff(lastArpNote);
                    lastArpNote = -1;
                }
            }
        };

    //========================================================
    // OSC Channel Controls
    addAndMakeVisible(channelLabel);
    channelLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(oscChannelComboBox);
    oscChannelComboBox.addItemList({ "1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16" }, 1);
    oscChannelComboBox.setSelectedId(1);
    oscChannelComboBox.onChange = [this]()
        {
            currentOSCChannel = oscChannelComboBox.getSelectedId();
            logMessage("OSC channel changed to: " + juce::String(currentOSCChannel));
        };

    //========================================================
    // CC Channel Controls
    addAndMakeVisible(ccChannelLabel);
    ccChannelLabel.setText("CC Channel:", juce::dontSendNotification);

    addAndMakeVisible(ccChannelComboBox);
    ccChannelComboBox.addItemList({ "1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16" }, 1);
    ccChannelComboBox.setSelectedId(1);
    ccChannelComboBox.onChange = [this]()
        {
            currentCCChannel = ccChannelComboBox.getSelectedId();
            logMessage("CC channel changed to: " + juce::String(currentCCChannel));
        };

    //========================================================
    // CC Number Controls
    addAndMakeVisible(ccNumberLabel);
    ccNumberLabel.setText("CC Number:", juce::dontSendNotification);

    addAndMakeVisible(ccNumberComboBox);
    for (int i = 1; i <= 127; ++i)
        ccNumberComboBox.addItem("CC" + juce::String(i), i);
    ccNumberComboBox.setSelectedId(1);
    ccNumberComboBox.onChange = [this]()
        {
            currentCCNumber = ccNumberComboBox.getSelectedId();
            logMessage("CC Number selected: " + juce::String(currentCCNumber));
        };

    //========================================================
    // CC Value Slider
    addAndMakeVisible(ccValueLabel);
    ccValueLabel.setText("CC Value:", juce::dontSendNotification);

    addAndMakeVisible(ccValueSlider);
    ccValueSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    ccValueSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    ccValueSlider.setRange(0.0, 127.0, 1.0);
    ccValueSlider.setValue(0.0);
    ccValueSlider.onValueChange = [this]()
        {
            int ccVal = (int)ccValueSlider.getValue();
            sendCCMessage(currentCCChannel, currentCCNumber, ccVal);
        };

    //========================================================
    // Pitch Bend
    addAndMakeVisible(pitchBendLabel);
    pitchBendLabel.setText("Pitch Bend:", juce::dontSendNotification);

    addAndMakeVisible(pitchBendSlider);
    pitchBendSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchBendSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchBendSlider.setRange(0.0, 1.0, 0.001); // normalized range
    pitchBendSlider.setValue(0.5); // center is “no bend”
    pitchBendSlider.onValueChange = [this]()
        {
            float pitchValue = pitchBendSlider.getValue();
            sendPitchBendMessage(currentOSCChannel, pitchValue);
        };

    //========================================================
    // Channel Pressure (Aftertouch)
    addAndMakeVisible(channelPressureLabel);
    channelPressureLabel.setText("Channel Pressure:", juce::dontSendNotification);

    addAndMakeVisible(channelPressureSlider);
    channelPressureSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    channelPressureSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    channelPressureSlider.setRange(0.0, 127.0, 1.0);
    channelPressureSlider.setValue(0.0);
    channelPressureSlider.onValueChange = [this]()
        {
            int pressureVal = (int)channelPressureSlider.getValue();
            sendAftertouchMessage(currentOSCChannel, pressureVal);
        };

    //========================================================
    // Register for incoming OSC messages
    oscReceiver.addListener(this, "/ch1noteon");
    oscReceiver.addListener(this, "/ch1noteoff");
    oscReceiver.addListener(this, "/ch1nvalue");
    oscReceiver.addListener(this, "/ch1cc");
    oscReceiver.addListener(this, "/ch1ccvalue");
    oscReceiver.addListener(this, "/ch1pitch");
    oscReceiver.addListener(this, "/ch1pressure");

    //========================================================
    // Initialize MIDI devices
    updateMidiDevices();

    //========================================================
    // Automatically start the OSC server on launch
    startOSCServer();

    //========================================================
    // Initialize pitch bend slider position
    sendPitchBendMessage(currentOSCChannel, pitchBendSlider.getValue());

    //========================================================
    // Side Menu
    addAndMakeVisible(sideMenu);
    sideMenu.setVisible(false);

    // Hamburger button
    addAndMakeVisible(hamburgerButton);
    hamburgerButton.setButtonText(juce::String::fromUTF8("☰")); // Unicode
    hamburgerButton.setLookAndFeel(&customLookAndFeel);

    // Show/hide side menu with animation
    hamburgerButton.onClick = [this]()
        {
            if (isAnimating)
                return;

            isSideMenuVisible = !isSideMenuVisible;
            sideMenu.setVisible(true); // make it visible to animate
            isAnimating = true;
            startTimerHz(60); // 60 fps
        };

    // Side menu close button
    sideMenu.closeButton.onClick = [this]()
        {
            if (isAnimating)
                return;

            isSideMenuVisible = false;
            isAnimating = true;
            startTimerHz(60); // 60 fps
        };

    // What to do when side menu items are clicked
    sideMenu.onMenuItemClicked = [this](int menuItemId)
        {
            handleMenuItemClick(menuItemId);
        };
}

//------------------------------------------------------------------------------
MainComponent::~MainComponent()
{
    // Clean up look and feel
    hamburgerButton.setLookAndFeel(nullptr);

    stopOSCServer();
    midiKeyboardState.removeListener(this);

    stopTimer(); // stop ARP timer if running

    if (currentMidiInput)
    {
        currentMidiInput->stop();
        currentMidiInput.reset();
    }

    if (currentMidiOutput)
    {
        currentMidiOutput.reset();
    }
}

//------------------------------------------------------------------------------
void MainComponent::logMessage(const juce::String& message)
{
    {
        juce::ScopedLock lock(logLock);
        pendingLogMessages += message + "\n";
    }
    triggerAsyncUpdate();
}

//------------------------------------------------------------------------------
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

//------------------------------------------------------------------------------
void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    const int labelHeight = 20;
    const int entryHeight = 20;
    const int comboBoxHeight = 30;
    const int buttonHeight = 30;
    const int sliderHeight = 80;
    const int smallSliderHeight = 50;
    const int keyboardHeight = 100;

    // Side menu width
    int sideMenuWidth = currentSideMenuWidth;
    sideMenu.setBounds(area.removeFromLeft(sideMenuWidth));

    // Hamburger button at top-left, offset by side menu width
    hamburgerButton.setBounds(sideMenuWidth + 10, 10, 30, 30);
    area.removeFromTop(40); // spacing beneath hamburger

    // Right area for ARP etc.
    auto rightArea = area.removeFromRight(400);

    // ARP controls
    arpSpeedSlider.setBounds(rightArea.removeFromTop(sliderHeight));
    arpButton.setBounds(rightArea.removeFromTop(buttonHeight));

    // Hold button
    holdButton.setBounds(rightArea.removeFromTop(buttonHeight));

    // OSC channel combo
    channelLabel.setBounds(rightArea.removeFromTop(labelHeight));
    oscChannelComboBox.setBounds(rightArea.removeFromTop(comboBoxHeight));

    // CC channel
    ccChannelLabel.setBounds(rightArea.removeFromTop(labelHeight));
    ccChannelComboBox.setBounds(rightArea.removeFromTop(comboBoxHeight));

    // CC number
    ccNumberLabel.setBounds(rightArea.removeFromTop(labelHeight));
    ccNumberComboBox.setBounds(rightArea.removeFromTop(comboBoxHeight));

    // CC value slider
    ccValueLabel.setBounds(rightArea.removeFromTop(labelHeight));
    ccValueSlider.setBounds(rightArea.removeFromTop(smallSliderHeight));

    // Pitch Bend
    pitchBendLabel.setBounds(rightArea.removeFromTop(labelHeight));
    pitchBendSlider.setBounds(rightArea.removeFromTop(100));

    // Channel Pressure
    channelPressureLabel.setBounds(rightArea.removeFromTop(labelHeight));
    channelPressureSlider.setBounds(rightArea.removeFromTop(smallSliderHeight));

    // IP In
    ipInLabel.setBounds(area.removeFromTop(labelHeight));
    ipInEntry.setBounds(area.removeFromTop(entryHeight));

    // Port In
    portInLabel.setBounds(area.removeFromTop(labelHeight));
    portInEntry.setBounds(area.removeFromTop(entryHeight));

    // IP Out
    ipOutLabel.setBounds(area.removeFromTop(labelHeight));
    ipOutEntry.setBounds(area.removeFromTop(entryHeight));

    // Port Out
    portOutLabel.setBounds(area.removeFromTop(labelHeight));
    portOutEntry.setBounds(area.removeFromTop(entryHeight));

    // Start/Stop button
    startButton.setBounds(area.removeFromTop(buttonHeight));

    // MIDI Input
    midiInputLabel.setBounds(area.removeFromTop(labelHeight));
    midiInputComboBox.setBounds(area.removeFromTop(comboBoxHeight));

    // MIDI Output
    midiOutputLabel.setBounds(area.removeFromTop(labelHeight));
    midiOutputComboBox.setBounds(area.removeFromTop(comboBoxHeight));

    // Keyboard at the bottom
    midiKeyboard.setBounds(area.removeFromBottom(keyboardHeight));

    // Log box fills the remainder
    log_list_box.setBounds(area);

    hamburgerButton.toFront(true);
}

//------------------------------------------------------------------------------
void MainComponent::startOSCServer()
{
    oscConnected = false;

    // Try to connect the OSC receiver
    if (oscReceiver.connect(portInEntry.getText().getIntValue()))
        logMessage("OSC receiver connected on port " + portInEntry.getText());
    else
        logMessage("Failed to connect OSC receiver on port " + portInEntry.getText());

    // Connect the OSC sender
    if (oscSender.connect(ipOutEntry.getText(), portOutEntry.getText().getIntValue()))
    {
        logMessage("OSC sender connected to " + ipOutEntry.getText() + ":" + portOutEntry.getText());
        oscConnected = true;
    }
    else
    {
        logMessage("Failed to connect OSC sender to " + ipOutEntry.getText() + ":" + portOutEntry.getText());
    }
}

//------------------------------------------------------------------------------
void MainComponent::stopOSCServer()
{
    oscReceiver.disconnect();
    oscSender.disconnect();
    oscConnected = false;
    logMessage("OSC server stopped.");
}

//------------------------------------------------------------------------------
void MainComponent::updateMidiDevices()
{
    // Populate MIDI inputs
    auto inputs = juce::MidiInput::getAvailableDevices();
    midiInputComboBox.clear();
    midiInputIdentifiers.clear();

    for (int i = 0; i < inputs.size(); ++i)
    {
        midiInputComboBox.addItem(inputs[i].name, i + 1);
        midiInputIdentifiers.add(inputs[i].identifier);
        logMessage("Available MIDI Input [" + juce::String(i) + "]: " + inputs[i].name + " Identifier: " + inputs[i].identifier);
    }

    // Populate MIDI outputs
    auto outputs = juce::MidiOutput::getAvailableDevices();
    midiOutputComboBox.clear();
    midiOutputIdentifiers.clear();

    for (int i = 0; i < outputs.size(); ++i)
    {
        midiOutputComboBox.addItem(outputs[i].name, i + 1);
        midiOutputIdentifiers.add(outputs[i].identifier);
        logMessage("Available MIDI Output [" + juce::String(i) + "]: " + outputs[i].name + " Identifier: " + outputs[i].identifier);
    }

    // Auto-select first device if available
    if (inputs.size() > 0)
    {
        midiInputComboBox.setSelectedId(1);
        setMidiInput(midiInputIdentifiers[0]);
    }
    if (outputs.size() > 0)
    {
        midiOutputComboBox.setSelectedId(1);
        setMidiOutput(midiOutputIdentifiers[0]);
    }
}

//------------------------------------------------------------------------------
void MainComponent::setMidiInput(const juce::String& identifier)
{
    if (currentMidiInput)
    {
        currentMidiInput->stop();
        currentMidiInput.reset();
        logMessage("MIDI Input stopped.");
    }

    currentMidiInput = juce::MidiInput::openDevice(identifier, this);
    if (currentMidiInput)
    {
        currentMidiInput->start();
        logMessage("MIDI Input set: " + identifier);
    }
    else
    {
        logMessage("Failed to set MIDI Input: " + identifier);
    }
}

//------------------------------------------------------------------------------
void MainComponent::setMidiOutput(const juce::String& identifier)
{
    if (currentMidiOutput)
    {
        currentMidiOutput.reset();
        logMessage("MIDI Output stopped.");
    }

    currentMidiOutput = juce::MidiOutput::openDevice(identifier);
    if (currentMidiOutput)
    {
        logMessage("MIDI Output set: " + identifier);
    }
    else
    {
        logMessage("Failed to set MIDI Output: " + identifier);
    }
}

//------------------------------------------------------------------------------
void MainComponent::sendOSCMessage(int midiNote, bool noteOn)
{
    if (!oscConnected)
        return;

    midiNote = juce::jlimit(0, 127, midiNote);
    juce::String address = "/ch" + juce::String(currentOSCChannel) + (noteOn ? "note" : "noteoff");
    oscSender.send(juce::OSCMessage(address, midiNote));
}

//------------------------------------------------------------------------------
void MainComponent::sendVelocityMessage(int midiNote, float velocity)
{
    if (!oscConnected)
        return;

    midiNote = juce::jlimit(0, 127, midiNote);
    juce::String address = "/ch" + juce::String(currentOSCChannel) + "nvalue";
    juce::OSCMessage message(address, midiNote, velocity);
    oscSender.send(message);

    logMessage("Sent OSC velocity for note " + juce::String(midiNote) + " = " + juce::String(velocity));
}

//------------------------------------------------------------------------------
void MainComponent::sendCCMessage(int channel, int ccNumber, int ccValue)
{
    channel = juce::jlimit(1, 16, channel);
    ccNumber = juce::jlimit(0, 127, ccNumber);
    ccValue = juce::jlimit(0, 127, ccValue);

    // MIDI CC
    if (currentMidiOutput)
        currentMidiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(channel, ccNumber, ccValue));

    // OSC: /chXcc & /chXccvalue
    if (oscConnected)
    {
        juce::String ccAddr = "/ch" + juce::String(channel) + "cc";
        juce::String ccValueAddr = "/ch" + juce::String(channel) + "ccvalue";

        oscSender.send(juce::OSCMessage(ccAddr, ccNumber));

        float normalizedVal = ccValue / 127.0f;
        oscSender.send(juce::OSCMessage(ccValueAddr, normalizedVal));

        logMessage("Sent OSC CC channel " + juce::String(channel) + ": CC#" + juce::String(ccNumber)
            + " Value: " + juce::String(normalizedVal));
    }
}

//------------------------------------------------------------------------------
void MainComponent::sendPitchBendMessage(int channel, float pitchValue)
{
    channel = juce::jlimit(1, 16, channel);
    pitchValue = juce::jlimit(0.0f, 1.0f, pitchValue);

    // Convert normalized [0..1] → MIDI pitch bend range [0..16383]
    int midiPB = (int)(pitchValue * 16383.0f + 0.5f);
    midiPB = juce::jlimit(0, 16383, midiPB);

    // Convert to float range [-8400..+8400] for OSC
    float oscPitchBend = ((midiPB / 16383.0f) * 2.0f - 1.0f) * 8400.0f;

    // Send MIDI pitch bend
    if (currentMidiOutput)
    {
        juce::MidiMessage pitchMsg = juce::MidiMessage::pitchWheel(channel, midiPB);
        currentMidiOutput->sendMessageNow(pitchMsg);
    }

    // Send OSC
    if (oscConnected)
    {
        juce::String pitchAddress = "/ch" + juce::String(channel) + "pitch";
        oscSender.send(juce::OSCMessage(pitchAddress, oscPitchBend));

        logMessage("Sent OSC Pitch Bend on channel " + juce::String(channel)
            + ": " + juce::String(oscPitchBend));
    }
}

//------------------------------------------------------------------------------
void MainComponent::sendAftertouchMessage(int channel, int pressureValue)
{
    channel = juce::jlimit(1, 16, channel);
    pressureValue = juce::jlimit(0, 127, pressureValue);

    // MIDI aftertouch
    if (currentMidiOutput)
        currentMidiOutput->sendMessageNow(juce::MidiMessage::channelPressureChange(channel, pressureValue));

    // OSC
    if (oscConnected)
    {
        juce::String pressureAddress = "/ch" + juce::String(channel) + "pressure";
        oscSender.send(juce::OSCMessage(pressureAddress, pressureValue));

        logMessage("Sent OSC Channel Pressure on channel " + juce::String(channel)
            + ": " + juce::String(pressureValue));
    }
}

//------------------------------------------------------------------------------
void MainComponent::oscMessageReceived(const juce::OSCMessage& message)
{
    handleIncomingOSCMessage(message);
}

//------------------------------------------------------------------------------
void MainComponent::handleIncomingOSCMessage(const juce::OSCMessage& message)
{
    logMessage("OSC Received: " + message.getAddressPattern().toString());
    // Currently no direct ARP logic from incoming OSC
}

//------------------------------------------------------------------------------
void MainComponent::handleNoteOn(juce::MidiKeyboardState*, int /*midiChannel*/, int midiNoteNumber, float velocity)
{
    {
        juce::ScopedLock lock(queueLock);
        MidiEvent event;
        event.type = MidiEvent::Type::NoteOn;
        event.channel = currentOSCChannel;
        event.parameter = midiNoteNumber;
        event.value = velocity;
        midiEventsQueue.add(event);
    }
    triggerAsyncUpdate();
    logMessage("Keyboard Note On: " + juce::String(midiNoteNumber)
        + " Velocity: " + juce::String(velocity));
}

//------------------------------------------------------------------------------
void MainComponent::handleNoteOff(juce::MidiKeyboardState*, int /*midiChannel*/, int midiNoteNumber, float velocity)
{
    {
        juce::ScopedLock lock(queueLock);
        MidiEvent event;
        event.type = MidiEvent::Type::NoteOff;
        event.channel = currentOSCChannel;
        event.parameter = midiNoteNumber;
        event.value = velocity;
        midiEventsQueue.add(event);
    }
    triggerAsyncUpdate();
    logMessage("Keyboard Note Off: " + juce::String(midiNoteNumber));
}

//------------------------------------------------------------------------------
void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        midiKeyboardState.noteOn(message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
        {
            juce::ScopedLock lock(queueLock);
            MidiEvent event;
            event.type = MidiEvent::Type::NoteOn;
            event.channel = message.getChannel();
            event.parameter = message.getNoteNumber();
            event.value = message.getFloatVelocity();
            midiEventsQueue.add(event);
        }
        triggerAsyncUpdate();
    }
    else if (message.isNoteOff())
    {
        midiKeyboardState.noteOff(message.getChannel(), message.getNoteNumber(), 0.0f);
        {
            juce::ScopedLock lock(queueLock);
            MidiEvent event;
            event.type = MidiEvent::Type::NoteOff;
            event.channel = message.getChannel();
            event.parameter = message.getNoteNumber();
            event.value = 0.0f;
            midiEventsQueue.add(event);
        }
        triggerAsyncUpdate();
    }
    else if (message.isController())
    {
        int channel = message.getChannel();
        int ccNumber = message.getControllerNumber();
        int ccValue = message.getControllerValue();
        logMessage("Received CC on channel " + juce::String(channel)
            + ": CC#" + juce::String(ccNumber)
            + " Value: " + juce::String(ccValue));

        {
            juce::ScopedLock lock(queueLock);
            MidiEvent event;
            event.type = MidiEvent::Type::ControlChange;
            event.channel = channel;
            event.parameter = ccNumber;
            event.value = (float)ccValue;
            midiEventsQueue.add(event);
        }
        triggerAsyncUpdate();
    }
    else if (message.isPitchWheel())
    {
        int channel = message.getChannel();
        int pitchValue = message.getPitchWheelValue(); // 0..16383
        logMessage("Received Pitch Bend on channel " + juce::String(channel)
            + ": " + juce::String(pitchValue));

        float normalizedPitch = pitchValue / 16383.0f;
        {
            juce::ScopedLock lock(queueLock);
            MidiEvent event;
            event.type = MidiEvent::Type::PitchBend;
            event.channel = channel;
            event.parameter = 0;
            event.value = normalizedPitch;
            midiEventsQueue.add(event);
        }
        triggerAsyncUpdate();
    }
    else if (message.isAftertouch())
    {
        int channel = message.getChannel();
        int pressureValue = message.getAfterTouchValue(); // 0..127
        logMessage("Received Aftertouch on channel " + juce::String(channel)
            + ": " + juce::String(pressureValue));

        {
            juce::ScopedLock lock(queueLock);
            MidiEvent event;
            event.type = MidiEvent::Type::Aftertouch;
            event.channel = channel;
            event.parameter = 0;
            event.value = (float)pressureValue;
            midiEventsQueue.add(event);
        }
        triggerAsyncUpdate();
    }
    // Add more MIDI handling logic if needed...
}

//------------------------------------------------------------------------------
void MainComponent::handleAsyncUpdate()
{
    // First flush pending log messages
    juce::String logsToAdd;
    {
        juce::ScopedLock lock(logLock);
        logsToAdd = pendingLogMessages;
        pendingLogMessages.clear();
    }

    if (!logsToAdd.isEmpty())
    {
        juce::StringArray lines = juce::StringArray::fromLines(logsToAdd);
        for (auto& line : lines)
        {
            if (!line.isEmpty())
                logListModel.addLog(line);
        }
    }

    // Process queued MIDI events
    juce::Array<MidiEvent> eventsToProcess;
    {
        juce::ScopedLock lock(queueLock);
        eventsToProcess = midiEventsQueue;
        midiEventsQueue.clear();
    }

    if (!arpEnabled)
    {
        // ARP disabled: process them directly
        for (auto& event : eventsToProcess)
        {
            juce::ScopedLock noteLock(activeNotesLock);
            int channel = juce::jlimit(1, 16, event.channel);
            int param = juce::jlimit(0, 127, event.parameter);
            float value = juce::jlimit(0.0f, 1.0f, event.value);

            switch (event.type)
            {
            case MidiEvent::Type::NoteOn:
            {
                // If the note is already active, send note-off first
                if (activeNotes.find(param) != activeNotes.end())
                {
                    sendOSCMessage(param, false);
                    logMessage("Duplicate Note On -> forced Note Off for " + juce::String(param));
                    if (currentMidiOutput)
                        currentMidiOutput->sendMessageNow(juce::MidiMessage::noteOff(channel, param));
                }

                sendOSCMessage(param, true);
                sendVelocityMessage(param, value);
                activeNotes.insert(param);

                if (currentMidiOutput)
                    currentMidiOutput->sendMessageNow(juce::MidiMessage::noteOn(channel, param, value));
            }
            break;

            case MidiEvent::Type::NoteOff:
            {
                sendOSCMessage(param, false);
                activeNotes.erase(param);

                if (currentMidiOutput)
                    currentMidiOutput->sendMessageNow(juce::MidiMessage::noteOff(channel, param));
            }
            break;

            case MidiEvent::Type::ControlChange:
            {
                // Parameter is CC number, value is 0..1, scale to 0..127
                int intValue = (int)(value * 127.0f);
                sendCCMessage(currentCCChannel, param, intValue);
            }
            break;

            case MidiEvent::Type::PitchBend:
            {
                sendPitchBendMessage(channel, value);
            }
            break;

            case MidiEvent::Type::Aftertouch:
            {
                int pressureVal = (int)value;
                sendAftertouchMessage(channel, pressureVal);
            }
            break;
            }
        }
    }
    else
    {
        // ARP enabled: only add notes to the heldNotes set, do not immediately send them
        for (auto& event : eventsToProcess)
        {
            int channel = juce::jlimit(1, 16, event.channel);
            int param = juce::jlimit(0, 127, event.parameter);
            float value = juce::jlimit(0.0f, 127.0f, event.value);

            switch (event.type)
            {
            case MidiEvent::Type::NoteOn:
                heldNotes.add(param);
                break;

            case MidiEvent::Type::NoteOff:
                if (!holdEnabled)
                {
                    heldNotes.removeValue(param);
                    if (lastArpNote == param)
                        lastArpNote = -1;
                }
                break;

            case MidiEvent::Type::ControlChange:
            {
                int intValue = (int)(value); // CC was 0..127
                sendCCMessage(currentCCChannel, param, intValue);
            }
            break;

            case MidiEvent::Type::PitchBend:
                sendPitchBendMessage(channel, value);
                break;

            case MidiEvent::Type::Aftertouch:
                sendAftertouchMessage(channel, (int)value);
                break;
            }
        }
    }

    // Update log UI
    log_list_box.updateContent();
    int totalRows = logListModel.getNumRows();
    if (totalRows > 0)
        log_list_box.scrollToEnsureRowIsOnscreen(totalRows - 1);
}

//------------------------------------------------------------------------------
void MainComponent::timerCallback()
{
    if (isAnimating)
    {
        // Expand or collapse side menu
        if (isSideMenuVisible)
        {
            currentSideMenuWidth += animationStep;
            if (currentSideMenuWidth >= targetSideMenuWidth)
            {
                currentSideMenuWidth = targetSideMenuWidth;
                isAnimating = false;
                stopTimer();
            }
        }
        else
        {
            currentSideMenuWidth -= animationStep;
            if (currentSideMenuWidth <= 0)
            {
                currentSideMenuWidth = 0;
                isAnimating = false;
                stopTimer();
                sideMenu.setVisible(false);
            }
        }
        resized(); // re-layout
    }

    if (arpEnabled)
    {
        if (heldNotes.size() == 0)
        {
            if (lastArpNote >= 0)
            {
                sendArpNoteOff(lastArpNote);
                lastArpNote = -1;
            }
            return;
        }
        advanceArp();
    }
}

//------------------------------------------------------------------------------
void MainComponent::updateArpState()
{
    if (arpEnabled)
    {
        startTimerHz((int)arpRateHz);
        currentArpIndex = 0;
        goingUp = true;
        if (lastArpNote >= 0)
        {
            sendArpNoteOff(lastArpNote);
            lastArpNote = -1;
        }
    }
    else
    {
        stopTimer();
        if (lastArpNote >= 0)
        {
            sendArpNoteOff(lastArpNote);
            lastArpNote = -1;
        }
    }
}

//------------------------------------------------------------------------------
void MainComponent::advanceArp()
{
    int noteCount = heldNotes.size();
    if (noteCount == 0)
        return;

    if (noteCount == 1)
    {
        int singleNote = juce::jlimit(0, 127, heldNotes[0]);
        if (lastArpNote >= 0 && lastArpNote == singleNote)
            sendArpNoteOff(lastArpNote);

        sendArpNoteOn(singleNote);
        lastArpNote = singleNote;
        return;
    }

    // Multiple notes
    if (lastArpNote >= 0)
        sendArpNoteOff(lastArpNote);

    if (goingUp)
    {
        currentArpIndex++;
        if (currentArpIndex >= noteCount)
        {
            currentArpIndex = noteCount - 2;
            goingUp = false;
            if (currentArpIndex < 0)
                currentArpIndex = 0;
        }
    }
    else
    {
        currentArpIndex--;
        if (currentArpIndex < 0)
        {
            currentArpIndex = (noteCount > 1) ? 1 : 0;
            goingUp = true;
        }
    }

    if (currentArpIndex < 0 || currentArpIndex >= noteCount)
        currentArpIndex = 0;

    int rawNote = heldNotes[currentArpIndex];
    int noteToPlay = juce::jlimit(0, 127, rawNote);
    sendArpNoteOn(noteToPlay);
    lastArpNote = noteToPlay;
}

//------------------------------------------------------------------------------
void MainComponent::sendArpNoteOn(int noteNumber, float velocity)
{
    sendOSCMessage(noteNumber, true);
    sendVelocityMessage(noteNumber, velocity);
    if (currentMidiOutput)
        currentMidiOutput->sendMessageNow(juce::MidiMessage::noteOn(currentOSCChannel, noteNumber, velocity));

    logMessage("ARP Note On: " + juce::String(noteNumber) + " velocity=" + juce::String(velocity));
}

//------------------------------------------------------------------------------
void MainComponent::sendArpNoteOff(int noteNumber)
{
    sendOSCMessage(noteNumber, false);
    if (currentMidiOutput)
        currentMidiOutput->sendMessageNow(juce::MidiMessage::noteOff(currentOSCChannel, noteNumber));

    logMessage("ARP Note Off: " + juce::String(noteNumber));
}

//------------------------------------------------------------------------------
void MainComponent::handleMenuItemClick(int menuItemId)
{
    switch (menuItemId)
    {
    case 1:
        logMessage("Menu Item 1 clicked -> Opening CC Control Window");
        if (!ccControlWindow)
        {
            ccControlWindow = std::make_unique<CCControlWindow>(
                [this](int ccNumber, float value)
                {
                    logMessage("CC#" + juce::String(ccNumber) + " Value: " + juce::String(value));
                });
            ccControlWindow->setAlwaysOnTop(true);
        }
        ccControlWindow->setVisible(true);
        ccControlWindow->toFront(true);
        logMessage("CC Control Window is now visible.");
        break;

    case 2:
        logMessage("Menu Item 2 clicked -> Opening File Browser");
        if (!fileBrowserWindow)
        {
            fileBrowserWindow = std::make_unique<FileBrowserWindow>(
                [this](const juce::File& file)
                {
                    logMessage("File selected: " + file.getFullPathName());
                });
            fileBrowserWindow->setAlwaysOnTop(true);
        }
        fileBrowserWindow->setVisible(true);
        fileBrowserWindow->toFront(true);
        logMessage("File Browser Window is now visible.");
        break;

    case 3:
        logMessage("Menu Item 3 clicked -> Opening Mixer Control Window");
        if (!mixerControlWindow)
        {
            mixerControlWindow = std::make_unique<MixerControlWindow>(
                [this](const juce::String& parameter, int channel, float value)
                {
                    logMessage(parameter + " on channel " + juce::String(channel)
                        + " value = " + juce::String(value));
                    // You could send OSC for mixer controls here if needed
                });
            mixerControlWindow->setAlwaysOnTop(true);
        }
        mixerControlWindow->setVisible(true);
        mixerControlWindow->toFront(true);
        logMessage("Mixer Control Window is now visible.");
        break;

    default:
        logMessage("Unknown Menu Item Clicked: " + juce::String(menuItemId));
        break;
    }

    // Optionally close the side menu after a click
    isSideMenuVisible = false;
    sideMenu.setVisible(false);
    resized();
    logMessage("Side menu closed.");
}
