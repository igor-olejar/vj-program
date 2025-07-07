#pragma once
#include <RtMidi.h>
#include <memory>
#include <functional>

class MidiHandler {
public:
    MidiHandler();
    ~MidiHandler();
    
    bool initialize(int portNumber = -1); // -1 = auto select
    void shutdown();
    
    // Callback function type for MIDI events
    using NoteCallback = std::function<void(int note, bool isNoteOn)>;
    using StopCallback = std::function<void()>;
    
    void setNoteCallback(NoteCallback callback) { noteCallback = callback; }
    void setStopCallback(StopCallback callback) { stopCallback = callback; }
    
    void listMidiPorts();
    bool connectToPort(int portNumber);
    int getPortCount() const;
    std::string getPortName(int portNumber) const;
    
private:
    std::unique_ptr<RtMidiIn> midiIn;
    NoteCallback noteCallback;
    StopCallback stopCallback;
    
    // Static callback for RtMidi (needs to be static)
    static void midiCallback(double deltatime, std::vector<unsigned char>* message, void* userData);
    
    // Instance method to process MIDI messages
    void processMidiMessage(const std::vector<unsigned char>& message);
};