#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct DisplayInfo {
    int x, y;          // Position
    int width, height; // Resolution
    std::string name;  // Display name
    bool isPrimary;
};

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    bool initialize(bool startFullscreen = false, int displayIndex = -1);
    void shutdown();
    
    std::vector<DisplayInfo> getAvailableDisplays();
    bool setOutputDisplay(int displayIndex);
    void toggleFullscreen();
    
    void showFrame(const cv::Mat& frame);
    bool isWindowOpen() const { return windowOpen; }
    
    // Window event handling
    char handleEvents(); // Returns key pressed, 27 for ESC
    
private:
    std::string windowName;
    bool windowOpen;
    bool isFullscreen;
    int currentDisplayIndex;
    std::vector<DisplayInfo> displays;
    
    void detectDisplays();
    void moveWindowToDisplay(int displayIndex);
    void setFullscreen(bool fullscreen);
};