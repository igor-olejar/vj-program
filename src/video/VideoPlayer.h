#pragma once
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

class VideoClip;

struct PlayingVideo {
    cv::VideoCapture capture;
    cv::Mat currentFrame;
    std::thread playbackThread;
    std::atomic<bool> shouldStop;
    std::string clipPath;
    
    PlayingVideo(const std::string& path);
    ~PlayingVideo();
};

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();
    
    bool initialize();
    void shutdown();
    
    bool startClip(VideoClip* clip);
    void stopClip(VideoClip* clip);
    void stopAllClips();
    
    void render(); // Called from main loop to display current frame

    // Add this method to the public section of VideoPlayer class
    void getCompositeFrame(cv::Mat& frame);
    
private:
    std::map<VideoClip*, std::unique_ptr<PlayingVideo>> playingVideos;
    std::mutex videosMutex;
    
    cv::Mat compositeFrame;
    int windowWidth, windowHeight;
    std::string windowName;
    
    void playbackLoop(PlayingVideo* video);
    void createCompositeFrame();
};