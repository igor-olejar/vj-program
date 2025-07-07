#include "display/DisplayManager.h"
#include <iostream>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

DisplayManager::DisplayManager() 
    : windowName("VJ Output"), windowOpen(false), isFullscreen(false), currentDisplayIndex(0) {
}

DisplayManager::~DisplayManager() {
    shutdown();
}

bool DisplayManager::initialize(bool startFullscreen, int displayIndex) {
    std::cout << "Initializing display manager..." << std::endl;
    
    detectDisplays();
    
    // Create the window
    cv::namedWindow(windowName, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    windowOpen = true;
    
    // Determine which display to use
    if (displayIndex >= 0 && displayIndex < static_cast<int>(displays.size())) {
        currentDisplayIndex = displayIndex;
        std::cout << "Using specified display " << displayIndex << std::endl;
    } else if (displays.size() > 1) {
        currentDisplayIndex = 1; // Default to second display if available
        std::cout << "Multiple displays detected, using display 1" << std::endl;
    } else {
        currentDisplayIndex = 0;
        std::cout << "Using primary display" << std::endl;
    }
    
    // Position window on chosen display
    setOutputDisplay(currentDisplayIndex);
    
    // Set initial size (windowed mode)
    const auto& display = displays[currentDisplayIndex];
    cv::resizeWindow(windowName, display.width / 2, display.height / 2);
    
    // Go fullscreen if requested
    if (startFullscreen) {
        setFullscreen(true);
    }
    
    std::cout << "Display manager initialized" << std::endl;
    return true;
}

void DisplayManager::shutdown() {
    if (windowOpen) {
        cv::destroyWindow(windowName);
        windowOpen = false;
    }
}

std::vector<DisplayInfo> DisplayManager::getAvailableDisplays() {
    return displays;
}

void DisplayManager::detectDisplays() {
    displays.clear();
    
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Cannot open X display" << std::endl;
        // Fallback: assume single 1920x1080 display
        DisplayInfo primary;
        primary.x = 0;
        primary.y = 0;
        primary.width = 1920;
        primary.height = 1080;
        primary.name = "Primary";
        primary.isPrimary = true;
        displays.push_back(primary);
        return;
    }
    
    int numScreens = 1;
    XineramaScreenInfo* screenInfo = nullptr;
    
    // Try to get Xinerama info for multi-monitor setups
    if (XineramaIsActive(display)) {
        screenInfo = XineramaQueryScreens(display, &numScreens);
        std::cout << "Xinerama detected " << numScreens << " screens" << std::endl;
    }
    
    if (screenInfo) {
        for (int i = 0; i < numScreens; i++) {
            DisplayInfo info;
            info.x = screenInfo[i].x_org;
            info.y = screenInfo[i].y_org;
            info.width = screenInfo[i].width;
            info.height = screenInfo[i].height;
            info.name = "Display " + std::to_string(i);
            info.isPrimary = (i == 0);
            displays.push_back(info);
            
            std::cout << "Display " << i << ": " << info.width << "x" << info.height 
                      << " at (" << info.x << "," << info.y << ")" << std::endl;
        }
        XFree(screenInfo);
    } else {
        // Fallback: single screen
        int screenNum = DefaultScreen(display);
        DisplayInfo info;
        info.x = 0;
        info.y = 0;
        info.width = DisplayWidth(display, screenNum);
        info.height = DisplayHeight(display, screenNum);
        info.name = "Primary";
        info.isPrimary = true;
        displays.push_back(info);
        
        std::cout << "Single display: " << info.width << "x" << info.height << std::endl;
    }
    
    XCloseDisplay(display);
}

bool DisplayManager::setOutputDisplay(int displayIndex) {
    if (displayIndex < 0 || displayIndex >= static_cast<int>(displays.size())) {
        std::cerr << "Invalid display index: " << displayIndex << std::endl;
        return false;
    }
    
    currentDisplayIndex = displayIndex;
    
    if (windowOpen) {
        moveWindowToDisplay(displayIndex);
    }
    
    return true;
}

void DisplayManager::moveWindowToDisplay(int displayIndex) {
    if (displayIndex < 0 || displayIndex >= static_cast<int>(displays.size())) {
        return;
    }
    
    const auto& display = displays[displayIndex];
    
    std::cout << "Moving window to display " << displayIndex 
              << " (" << display.width << "x" << display.height << ")" << std::endl;
    
    // Move window
    cv::moveWindow(windowName, display.x + 50, display.y + 50); // Small offset from edge
}

void DisplayManager::setFullscreen(bool fullscreen) {
    if (!windowOpen) return;
    
    isFullscreen = fullscreen;
    
    if (fullscreen) {
        std::cout << "Switching to fullscreen mode" << std::endl;
        cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
        moveWindowToDisplay(currentDisplayIndex);
    } else {
        std::cout << "Switching to windowed mode" << std::endl;
        cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_NORMAL);
        const auto& display = displays[currentDisplayIndex];
        cv::resizeWindow(windowName, display.width / 2, display.height / 2);
        cv::moveWindow(windowName, display.x + 50, display.y + 50);
    }
}

void DisplayManager::toggleFullscreen() {
    setFullscreen(!isFullscreen);
}

void DisplayManager::showFrame(const cv::Mat& frame) {
    if (!windowOpen || frame.empty()) return;
    
    cv::Mat scaledFrame;
    
    if (isFullscreen) {
        const auto& display = displays[currentDisplayIndex];
        // Scale frame to match display resolution in fullscreen
        if (frame.cols != display.width || frame.rows != display.height) {
            cv::resize(frame, scaledFrame, cv::Size(display.width, display.height));
        } else {
            scaledFrame = frame;
        }
    } else {
        // In windowed mode, just show the frame as-is
        scaledFrame = frame;
    }
    
    cv::imshow(windowName, scaledFrame);
}

char DisplayManager::handleEvents() {
    return cv::waitKey(1) & 0xFF;
}