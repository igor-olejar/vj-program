#pragma once
#include <string>

class VideoClip {
public:
    VideoClip(const std::string& path, int startNote, int stopNote);
    
    const std::string& getPath() const { return videoPath; }
    int getStartNote() const { return startNote; }
    int getStopNote() const { return stopNote; }
    
    bool isPlaying() const { return playing; }
    void setPlaying(bool state) { playing = state; }
    
private:
    std::string videoPath;
    int startNote;
    int stopNote;
    bool playing;
};