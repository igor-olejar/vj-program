#include "utils/CsvParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

std::vector<ClipData> CsvParser::parseClipsFile(const std::string& filename) {
    std::vector<ClipData> clips;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string line;
    bool isFirstLine = true;
    
    while (std::getline(file, line)) {
        if (isFirstLine) {
            isFirstLine = false; // Skip header line
            continue;
        }
        
        if (line.empty()) continue;
        
        auto parts = splitLine(line, ',');
        if (parts.size() >= 3) {
            ClipData clip;
            clip.path = parts[0];
            clip.startNote = parts[1];
            clip.stopNote = parts[2];
            clips.push_back(clip);
        }
    }
    
    return clips;
}

std::vector<std::string> CsvParser::splitLine(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// In CsvParser.cpp, update noteStringToMidi to handle numbers
int CsvParser::noteStringToMidi(const std::string& note) {
    // If it's already a number, just convert it
    try {
        int midiNote = std::stoi(note);
        if (midiNote >= 0 && midiNote <= 127) {
            return midiNote;
        }
    } catch (const std::exception&) {
        // Not a number, continue with note name parsing
    }
    
    // Original note name parsing code...
    if (note.length() < 2) return -1;
    
    std::map<char, int> noteMap = {
        {'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}
    };
    
    char noteName = note[0];
    if (noteMap.find(noteName) == noteMap.end()) return -1;
    
    int offset = noteMap[noteName];
    
    size_t sharpPos = note.find('#');
    if (sharpPos != std::string::npos) {
        offset += 1;
    }
    
    std::string octaveStr = note.substr(1);
    if (sharpPos != std::string::npos) {
        octaveStr = note.substr(2);
    }
    
    try {
        int octave = std::stoi(octaveStr);
        return (octave + 1) * 12 + offset;
    } catch (const std::exception&) {
        return -1;
    }
}