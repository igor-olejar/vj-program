#include <iostream>
#include <opencv2/opencv.hpp>
#include "core/Application.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [csv_file]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -f, --fullscreen    Start in fullscreen mode" << std::endl;
    std::cout << "  -d, --display N     Use display N (0=primary, 1=secondary, etc.)" << std::endl;
    std::cout << "  -m, --midi N        Use MIDI port N (see --list-midi for available ports)" << std::endl;
    std::cout << "  --list-midi         List available MIDI ports and exit" << std::endl;
    std::cout << "  -h, --help          Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --list-midi                    # List MIDI ports" << std::endl;
    std::cout << "  " << programName << " -m 1                           # Use MIDI port 1" << std::endl;
    std::cout << "  " << programName << " -f -d 1 -m 1                   # Fullscreen, display 1, MIDI port 1" << std::endl;
    std::cout << "  " << programName << " -m 1 my_clips.csv              # Custom CSV with MIDI port 1" << std::endl;
}

int main(int argc, char* argv[]) {
    AppConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--list-midi") {
            config.listMidiPorts = true;
        } else if (arg == "-f" || arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "-d" || arg == "--display") {
            if (i + 1 < argc) {
                config.displayIndex = std::atoi(argv[++i]);
            } else {
                std::cerr << "Error: --display requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-m" || arg == "--midi") {
            if (i + 1 < argc) {
                config.midiPort = std::atoi(argv[++i]);
            } else {
                std::cerr << "Error: --midi requires a number" << std::endl;
                return 1;
            }
        } else if (arg[0] != '-') {
            // Not a flag, assume it's the CSV file
            config.csvPath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    Application app;
    
    if (!app.initialize(config)) {
        if (config.listMidiPorts) {
            return 0; // Normal exit after listing MIDI ports
        }
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    std::cout << "Application initialized successfully" << std::endl;
    app.run();
    
    return 0;
}