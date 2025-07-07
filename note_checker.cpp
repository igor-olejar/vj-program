#include <iostream>
#include <string>
#include <map>

// Copy of the noteStringToMidi function to test
int noteStringToMidi(const std::string& note) {
    if (note.length() < 2) return -1;
    
    // Map note names to chromatic offsets
    std::map<char, int> noteMap = {
        {'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}
    };
    
    char noteName = note[0];
    if (noteMap.find(noteName) == noteMap.end()) return -1;
    
    int offset = noteMap[noteName];
    
    // Handle sharp (#)
    size_t sharpPos = note.find('#');
    if (sharpPos != std::string::npos) {
        offset += 1;
    }
    
    // Extract octave number
    std::string octaveStr = note.substr(1);
    if (sharpPos != std::string::npos) {
        octaveStr = note.substr(2); // Skip the '#'
    }
    
    try {
        int octave = std::stoi(octaveStr);
        // MIDI note calculation: C4 = 60, C1 = 24
        return (octave + 1) * 12 + offset;
    } catch (const std::exception&) {
        return -1;
    }
}

int main() {
    std::cout << "MIDI Note Mapping Test:" << std::endl;
    std::cout << "C1 = " << noteStringToMidi("C1") << std::endl;
    std::cout << "C#1 = " << noteStringToMidi("C#1") << std::endl;
    std::cout << "D1 = " << noteStringToMidi("D1") << std::endl;
    std::cout << "C4 = " << noteStringToMidi("C4") << std::endl;
    
    std::cout << "\nYour MIDI note 24 corresponds to:" << std::endl;
    
    // Find which note corresponds to MIDI 24
    for (int octave = 0; octave <= 9; octave++) {
        for (const auto& note : {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}) {
            std::string noteStr = std::string(note) + std::to_string(octave);
            if (noteStringToMidi(noteStr) == 24) {
                std::cout << "MIDI 24 = " << noteStr << std::endl;
            }
        }
    }
    
    return 0;
}