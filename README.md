# JUCE-OSC2MIDI

Install JUCE w Projucer and Unzip file. Open .jucer file and select Projucer to run. Build a solution using Projucer by opening your IDE with the "OPEN in IDE" button at the top of Projucer app. Build the solution and run. Lots of bugs with this yet but the idea is to eventually convert to plugin and develop standalone/plugin that can run on Mac/PC to act as device bridge for Patchworld.

The osc addresses are as follows...(X = channel 1-16)

/chXnote 0-127, /chXnvalue 0-1
/chXnoteoff 0-127, /chXnoffvalue 0-1
/chXpitch,-8200 to 8200 (sits at 0),  /chX pressure 0-127
/chXcc 0-127, /chXccvalue 0-1

