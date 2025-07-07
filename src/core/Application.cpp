#include "core/Application.h"
#include "utils/CsvParser.h"
#include "video/VideoClip.h"
#include "midi/MidiHandler.h"
#include "video/VideoPlayer.h"
#include "display/DisplayManager.h"
#include <iostream>
#include <thread>

Application::Application() : running(false) {
    midiHandler = std::make_unique<MidiHandler>();
    videoPlayer = std::make_unique<VideoPlayer>();
    displayManager = std::make_unique<DisplayManager>();
}

Application::~Application() {
    shutdown();
}

void Application::listMidiPorts() {
    midiHandler->listMidiPorts();
}

bool Application::initialize(const AppConfig& appConfig) {
    config = appConfig;
    
    // If just listing MIDI ports, do that and exit
    if (config.listMidiPorts) {
        listMidiPorts();
        return false; // Don't continue with full initialization
    }
    
    std::cout << "=== VJ Application Starting ===" << std::endl;
    std::cout << "Config: " << std::endl;
    std::cout << "  CSV file: " << config.csvPath << std::endl;
    std::cout << "  Fullscreen: " << (config.fullscreen ? "Yes" : "No") << std::endl;
    std::cout << "  Display: " << (config.displayIndex >= 0 ? std::to_string(config.displayIndex) : "Auto") << std::endl;
    std::cout << "  MIDI port: " << (config.midiPort >= 0 ? std::to_string(config.midiPort) : "Auto") << std::endl;
    std::cout << std::endl;
    
    // Load clips configuration
    std::cout << "Loading clips from: " << config.csvPath << std::endl;
    if (!loadClipsFromCSV(config.csvPath)) {
        std::cerr << "Failed to load clips from CSV" << std::endl;
        return false;
    }
    std::cout << "✓ Loaded " << videoClips.size() << " video clips" << std::endl;
    
    // Initialize display manager
    if (!displayManager->initialize(config.fullscreen, config.displayIndex)) {
        std::cerr << "Failed to initialize display manager" << std::endl;
        return false;
    }
    std::cout << "✓ Display manager initialized" << std::endl;
    
    // Initialize video player
    if (!videoPlayer->initialize()) {
        std::cerr << "Failed to initialize video player" << std::endl;
        return false;
    }
    std::cout << "✓ Video player initialized" << std::endl;
    
    // Initialize MIDI with specified port
    if (!midiHandler->initialize(config.midiPort)) {
        std::cerr << "⚠ Failed to initialize MIDI (continuing anyway)" << std::endl;
    } else {
        std::cout << "✓ MIDI handler initialized" << std::endl;
    }
    
    // Set up MIDI callbacks
    midiHandler->setNoteCallback([this](int note, bool isNoteOn) {
        this->onMidiNote(note, isNoteOn);
    });
    
    midiHandler->setStopCallback([this]() {
        this->onMidiStop();
    });
    
    running = true;
    std::cout << "=== Application Ready ===" << std::endl;
    return true;
}

void Application::run() {
    std::cout << "\n🎵 VJ Application Running" << std::endl;
    if (config.fullscreen) {
        std::cout << "📺 Video output window is fullscreen" << std::endl;
        std::cout << "⌨️  Press ESC in video window to quit" << std::endl;
    } else {
        std::cout << "📺 Video output window is windowed mode" << std::endl;
        std::cout << "⌨️  Press ESC in video window or F11 for fullscreen" << std::endl;
    }
    std::cout << "🎹 Waiting for MIDI input..." << std::endl;
    std::cout << std::endl;
    
    // Display available clips
    std::cout << "Available clips:" << std::endl;
    for (const auto& clip : videoClips) {
        std::cout << "  • " << clip->getPath() 
                  << " (Start: MIDI " << clip->getStartNote() 
                  << ", Stop: MIDI " << clip->getStopNote() << ")" << std::endl;
    }
    std::cout << std::endl;
    
    int frameCount = 0;
    
    // Main render loop
    while (running && displayManager->isWindowOpen()) {
        // Get current composite frame from video player
        cv::Mat frame;
        videoPlayer->getCompositeFrame(frame);
        
        // Debug: Print every 60 frames (about once per second)
        if (frameCount % 60 == 0) {
            std::cout << "DEBUG: Main loop frame " << frameCount 
                      << ", frame size: " << frame.cols << "x" << frame.rows << std::endl;
        }
        
        // Display frame
        displayManager->showFrame(frame);
        
        // Handle window events
        char key = displayManager->handleEvents();
        if (key == 27) { // ESC key
            std::cout << "ESC pressed, shutting down..." << std::endl;
            running = false;
            break;
        } else if (key == 122) { // F11 key (toggle fullscreen)
            displayManager->toggleFullscreen();
        }
        
        frameCount++;
        
        // Small delay to prevent 100% CPU usage (~60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Application stopped." << std::endl;
}

void Application::shutdown() {
    std::cout << "Shutting down application..." << std::endl;
    running = false;
    
    if (videoPlayer) {
        videoPlayer->shutdown();
    }
    if (displayManager) {
        displayManager->shutdown();
    }
    if (midiHandler) {
        midiHandler->shutdown();
    }
    
    videoClips.clear();
    std::cout << "Application shutdown complete." << std::endl;
}

void Application::onMidiNote(int note, bool isNoteOn) {
    if (isNoteOn) {
        // Look for clip with this start note
        VideoClip* clip = findClipByNote(note, true);
        if (clip) {
            if (!clip->isPlaying()) {
                std::cout << "▶️  Starting: " << clip->getPath() << std::endl;
                if (videoPlayer->startClip(clip)) {
                    clip->setPlaying(true);
                }
            } else {
                std::cout << "🔄 Clip already playing: " << clip->getPath() << std::endl;
            }
        }
    } 
    // Remove the "else" here - don't stop on note off for start notes
    
    // Only handle stop notes explicitly
    VideoClip* stopClip = findClipByNote(note, false);
    if (stopClip && stopClip->isPlaying() && !isNoteOn) {
        std::cout << "⏹️  Stopping: " << stopClip->getPath() << std::endl;
        videoPlayer->stopClip(stopClip);
        stopClip->setPlaying(false);
    }
}

void Application::onMidiStop() {
    std::cout << "🛑 MIDI Stop - stopping all clips" << std::endl;
    videoPlayer->stopAllClips();
    for (auto& clip : videoClips) {
        clip->setPlaying(false);
    }
}

VideoClip* Application::findClipByNote(int note, bool isStart) {
    for (auto& clip : videoClips) {
        if (isStart && clip->getStartNote() == note) {
            return clip.get();
        } else if (!isStart && clip->getStopNote() == note) {
            return clip.get();
        }
    }
    return nullptr;
}

bool Application::loadClipsFromCSV(const std::string& csvPath) {
    try {
        auto clipData = CsvParser::parseClipsFile(csvPath);
        
        for (const auto& data : clipData) {
            int startNote = CsvParser::noteStringToMidi(data.startNote);
            int stopNote = CsvParser::noteStringToMidi(data.stopNote);
            
            if (startNote >= 0 && stopNote >= 0) {
                videoClips.push_back(std::make_unique<VideoClip>(data.path, startNote, stopNote));
                std::cout << "  📹 " << data.path << " (MIDI " << startNote << "-" << stopNote << ")" << std::endl;
            } else {
                std::cerr << "  ❌ Invalid notes for clip: " << data.path << std::endl;
            }
        }
        
        return !videoClips.empty();
    } catch (const std::exception& e) {
        std::cerr << "Error loading CSV: " << e.what() << std::endl;
        return false;
    }
}