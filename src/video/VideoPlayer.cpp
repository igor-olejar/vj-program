#include "video/VideoPlayer.h"
#include "video/VideoClip.h"
#include <iostream>
#include <filesystem>

PlayingVideo::PlayingVideo(const std::string& path) 
    : clipPath(path), shouldStop(false) {
    
    if (!capture.open(path)) {
        throw std::runtime_error("Cannot open video file: " + path);
    }
    
    // Set some properties for better performance
    capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
}

PlayingVideo::~PlayingVideo() {
    shouldStop = true;
    if (playbackThread.joinable()) {
        playbackThread.join();
    }
    capture.release();
}

VideoPlayer::VideoPlayer() 
    : windowWidth(1920), windowHeight(1080), windowName("VJ Output") {
}

VideoPlayer::~VideoPlayer() {
    shutdown();
}

bool VideoPlayer::initialize() {
    std::cout << "Initializing video player..." << std::endl;
    
    // Create the output window
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowName, windowWidth, windowHeight);
    
    // Try to move window to second display (rough approach)
    cv::moveWindow(windowName, 1920, 0); // Assumes primary display is 1920px wide
    
    // Create black composite frame
    compositeFrame = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
    
    std::cout << "Video player initialized" << std::endl;
    return true;
}

void VideoPlayer::shutdown() {
    std::cout << "Shutting down video player..." << std::endl;
    stopAllClips();
    cv::destroyWindow(windowName);
}

bool VideoPlayer::startClip(VideoClip* clip) {
    if (!clip) return false;
    
    std::lock_guard<std::mutex> lock(videosMutex);
    
    // Check if already playing
    if (playingVideos.find(clip) != playingVideos.end()) {
        std::cout << "Clip already playing: " << clip->getPath() << std::endl;
        return true;
    }
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(clip->getPath())) {
            std::cerr << "Video file not found: " << clip->getPath() << std::endl;
            return false;
        }
        
        auto video = std::make_unique<PlayingVideo>(clip->getPath());
        
        // Start playback thread
        video->playbackThread = std::thread(&VideoPlayer::playbackLoop, this, video.get());
        
        playingVideos[clip] = std::move(video);
        
        std::cout << "Started playing: " << clip->getPath() << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error starting clip: " << e.what() << std::endl;
        return false;
    }
}

void VideoPlayer::stopClip(VideoClip* clip) {
    if (!clip) return;
    
    std::lock_guard<std::mutex> lock(videosMutex);
    
    auto it = playingVideos.find(clip);
    if (it != playingVideos.end()) {
        std::cout << "Stopping clip: " << clip->getPath() << std::endl;
        it->second->shouldStop = true;
        playingVideos.erase(it);
    }
}

void VideoPlayer::stopAllClips() {
    std::lock_guard<std::mutex> lock(videosMutex);
    
    std::cout << "Stopping all clips (" << playingVideos.size() << ")" << std::endl;
    
    for (auto& pair : playingVideos) {
        pair.second->shouldStop = true;
    }
    
    playingVideos.clear();
}

void VideoPlayer::render() {
    createCompositeFrame();

    if (!compositeFrame.empty()) {
        cv::imshow("VJ Preview", compositeFrame);  // Separate preview window for debugging
    }
}

void VideoPlayer::playbackLoop(PlayingVideo* video) {
    if (!video) return;
    
    cv::Mat frame;
    double fps = video->capture.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30;
    
    int frameDelay = static_cast<int>(1000.0 / fps);
    
    // Only show essential startup info
    std::cout << "🎬 Playing: " << video->clipPath << " (" << fps << " FPS)" << std::endl;
    
    if (!video->capture.isOpened()) {
        std::cerr << "❌ Cannot open: " << video->clipPath << std::endl;
        return;
    }
    
    while (!video->shouldStop) {
        if (!video->capture.read(frame)) {
            // Loop back to start silently
            video->capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            if (!video->capture.read(frame)) {
                std::cerr << "❌ Playback error: " << video->clipPath << std::endl;
                break;
            }
        }
        
        if (frame.empty()) continue;
        
        try {
            cv::resize(frame, video->currentFrame, cv::Size(windowWidth, windowHeight));
        } catch (const cv::Exception& e) {
            std::cerr << "❌ Frame resize error: " << e.what() << std::endl;
            continue;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
    }
    
    std::cout << "⏹️  Stopped: " << video->clipPath << std::endl;
}

void VideoPlayer::createCompositeFrame() {
    std::lock_guard<std::mutex> lock(videosMutex);
    
    // Start with black frame
    compositeFrame = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
    
    if (playingVideos.empty()) {
        return;
    }
    
    // Show the last playing video
    auto& lastVideo = playingVideos.rbegin()->second;
    if (!lastVideo->currentFrame.empty()) {
        try {
            lastVideo->currentFrame.copyTo(compositeFrame);
        } catch (const cv::Exception& e) {
            std::cerr << "❌ Frame copy error: " << e.what() << std::endl;
        }
    }
}

void VideoPlayer::getCompositeFrame(cv::Mat& frame) {
    createCompositeFrame();
    
    std::lock_guard<std::mutex> lock(videosMutex);
    
    if (compositeFrame.empty()) {
        frame = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
    } else {
        compositeFrame.copyTo(frame);
    }
}