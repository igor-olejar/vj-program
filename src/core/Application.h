#pragma once
#include <vector>
#include <memory>
#include <string>

struct AppConfig {
    std::string csvPath;
    bool fullscreen;
    int displayIndex;
    int midiPort;
    bool listMidiPorts;
    
    AppConfig() : csvPath("data/clips.csv"), fullscreen(false), displayIndex(-1), midiPort(-1), listMidiPorts(false) {}
};

class VideoClip;
class MidiHandler;
class VideoPlayer;
class DisplayManager;

class Application {
public:
    Application();
    ~Application();
    
    bool initialize(const AppConfig& config);
    void run();
    void shutdown();
    
    // Called by MidiHandler when notes are received
    void onMidiNote(int note, bool isNoteOn);
    void onMidiStop();
    
    void listMidiPorts(); // Public method to list MIDI ports
    
private:
    std::vector<std::unique_ptr<VideoClip>> videoClips;
    std::unique_ptr<MidiHandler> midiHandler;
    std::unique_ptr<VideoPlayer> videoPlayer;
    std::unique_ptr<DisplayManager> displayManager;
    AppConfig config;
    bool running;
    bool loadClipsFromCSV(const std::string& csvPath);
    void stopAllPlayingClips();
    
    VideoClip* findClipByNote(int note, bool isStart);
};