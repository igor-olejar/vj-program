#include "midi/MidiHandler.h"
#include <iostream>
#include <iomanip>

MidiHandler::MidiHandler() {
    midiIn = std::make_unique<RtMidiIn>();
}

MidiHandler::~MidiHandler() {
    shutdown();
}

bool MidiHandler::initialize(int portNumber) {
    try {
        std::cout << "Initializing MIDI..." << std::endl;
        listMidiPorts();
        
        if (midiIn->getPortCount() == 0) {
            std::cout << "No MIDI ports found. You can still test without MIDI." << std::endl;
            return true;
        }
        
        int selectedPort = portNumber;
        
        // Auto-select port if not specified
        if (selectedPort < 0) {
            selectedPort = 0; // Default to first port
            std::cout << "No port specified, using default port 0" << std::endl;
        }
        
        return connectToPort(selectedPort);
        
    } catch (RtMidiError& error) {
        std::cerr << "MIDI Error: " << error.getMessage() << std::endl;
        return false;
    }
}

void MidiHandler::shutdown() {
    if (midiIn && midiIn->isPortOpen()) {
        midiIn->closePort();
    }
}

void MidiHandler::listMidiPorts() {
    unsigned int nPorts = midiIn->getPortCount();
    std::cout << "Available MIDI input ports:" << std::endl;
    
    for (unsigned int i = 0; i < nPorts; i++) {
        std::string portName = midiIn->getPortName(i);
        std::cout << "  " << i << ": " << portName << std::endl;
    }
    
    if (nPorts == 0) {
        std::cout << "  No MIDI input ports found." << std::endl;
    }
}

int MidiHandler::getPortCount() const {
    return static_cast<int>(midiIn->getPortCount());
}

std::string MidiHandler::getPortName(int portNumber) const {
    if (portNumber >= 0 && portNumber < getPortCount()) {
        return midiIn->getPortName(portNumber);
    }
    return "";
}

bool MidiHandler::connectToPort(int portNumber) {
    try {
        if (portNumber >= 0 && portNumber < static_cast<int>(midiIn->getPortCount())) {
            midiIn->openPort(portNumber);
            midiIn->setCallback(&MidiHandler::midiCallback, this);
            midiIn->ignoreTypes(false, false, false); // Don't ignore any message types
            
            std::string portName = midiIn->getPortName(portNumber);
            std::cout << "✓ Connected to MIDI port " << portNumber << ": " << portName << std::endl;
            return true;
        } else {
            std::cerr << "Invalid MIDI port number: " << portNumber << std::endl;
            std::cerr << "Available ports: 0 to " << (midiIn->getPortCount() - 1) << std::endl;
            return false;
        }
    } catch (RtMidiError& error) {
        std::cerr << "Error connecting to MIDI port: " << error.getMessage() << std::endl;
        return false;
    }
}

void MidiHandler::midiCallback(double deltatime, std::vector<unsigned char>* message, void* userData) {
    MidiHandler* handler = static_cast<MidiHandler*>(userData);
    if (handler && message) {
        handler->processMidiMessage(*message);
    }
}

void MidiHandler::processMidiMessage(const std::vector<unsigned char>& message) {
    if (message.size() < 1) return;
    
    unsigned char status = message[0];
    
    // Note On: 0x90-0x9F
    if ((status & 0xF0) == 0x90 && message.size() >= 3) {
        int note = message[1];
        int velocity = message[2];
        
        if (velocity > 0) {
            if (noteCallback) {
                noteCallback(note, true);
            }
        } else {
            // Note on with velocity 0 = note off
            if (noteCallback) {
                noteCallback(note, false);
            }
        }
    }
    // Note Off: 0x80-0x8F
    else if ((status & 0xF0) == 0x80 && message.size() >= 3) {
        int note = message[1];
        if (noteCallback) {
            noteCallback(note, false);
        }
    }
    // Control Change: 0xB0-0xBF (for stop messages)
    else if ((status & 0xF0) == 0xB0 && message.size() >= 3) {
        int controller = message[1];
        int value = message[2];
        
        // Common stop/panic controllers
        if (controller == 123 || controller == 120) {
            std::cout << "🛑 MIDI Stop/Panic (CC" << controller << ")" << std::endl;
            if (stopCallback) {
                stopCallback();
            }
        }
    }
}