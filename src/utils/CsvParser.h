#pragma once
#include <vector>
#include <string>

struct ClipData {
    std::string path;
    std::string startNote;
    std::string stopNote;
};

class CsvParser {
public:
    static std::vector<ClipData> parseClipsFile(const std::string& filename);
    static int noteStringToMidi(const std::string& note); // C1 -> 24, etc.
    
private:
    static std::vector<std::string> splitLine(const std::string& line, char delimiter);
};