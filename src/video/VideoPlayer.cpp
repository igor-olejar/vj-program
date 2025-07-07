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
    if (!video) {
        std::cerr << "playbackLoop: video pointer is null" << std::endl;
        return;
    }
    
    cv::Mat frame;
    double fps = video->capture.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30; // Default to 30 FPS if unknown
    
    int frameDelay = static_cast<int>(1000.0 / fps); // milliseconds
    
    std::cout << "Starting playback loop for: " << video->clipPath 
              << " (FPS: " << fps << ")" << std::endl;
    
    // Check if capture is actually opened
    if (!video->capture.isOpened()) {
        std::cerr << "ERROR: Video capture is not opened for: " << video->clipPath << std::endl;
        return;
    }
    
    // Get video properties for debugging
    int frameCount = static_cast<int>(video->capture.get(cv::CAP_PROP_FRAME_COUNT));
    double duration = frameCount / fps;
    int width = static_cast<int>(video->capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(video->capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    std::cout << "Video info: " << width << "x" << height 
              << ", " << frameCount << " frames, " << duration << "s duration" << std::endl;
    
    int frameNumber = 0;
    
    while (!video->shouldStop) {
        if (!video->capture.read(frame)) {
            if (frameNumber == 0) {
                std::cerr << "ERROR: Cannot read first frame from: " << video->clipPath << std::endl;
                break;
            }
            
            // End of video - loop back to start
            std::cout << "End of video reached, looping back to start..." << std::endl;
            video->capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            frameNumber = 0;
            
            if (!video->capture.read(frame)) {
                std::cerr << "ERROR: Cannot read video after loop reset: " << video->clipPath << std::endl;
                break;
            }
        }
        
        if (frame.empty()) {
            std::cerr << "ERROR: Empty frame read from: " << video->clipPath << std::endl;
            continue;
        }
        
        // Debug: Print frame info occasionally
        if (frameNumber % 30 == 0) { // Every second at 30fps
            std::cout << "Playing frame " << frameNumber << " of " << video->clipPath 
                      << " (size: " << frame.cols << "x" << frame.rows << ")" << std::endl;
        }
        
        // Resize frame to fit our output window
        try {
            cv::resize(frame, video->currentFrame, cv::Size(windowWidth, windowHeight));
        } catch (const cv::Exception& e) {
            std::cerr << "ERROR: Failed to resize frame: " << e.what() << std::endl;
            continue;
        }
        
        frameNumber++;
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
    }
    
    std::cout << "Playback loop ended for: " << video->clipPath 
              << " (played " << frameNumber << " frames)" << std::endl;
}

void VideoPlayer::createCompositeFrame() {
    std::lock_guard<std::mutex> lock(videosMutex);
    
    // Start with black frame
    compositeFrame = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
    
    // Debug: Check how many videos are playing
    if (playingVideos.empty()) {
        // No videos playing - keep black frame
        return;
    }
    
    std::cout << "DEBUG: " << playingVideos.size() << " videos currently playing" << std::endl;
    
    // For now, just show the last playing video (simple approach)
    auto& lastVideo = playingVideos.rbegin()->second;
    if (!lastVideo->currentFrame.empty()) {
        try {
            lastVideo->currentFrame.copyTo(compositeFrame);
            
            // Debug: Add text overlay showing which clip is playing
            std::string clipName = lastVideo->clipPath;
            size_t lastSlash = clipName.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                clipName = clipName.substr(lastSlash + 1);
            }
            
            cv::putText(compositeFrame, "Playing: " + clipName, 
                       cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                       1, cv::Scalar(0, 255, 0), 2);
                       
            std::cout << "DEBUG: Successfully copied frame from " << clipName << std::endl;
        } catch (const cv::Exception& e) {
            std::cerr << "ERROR: Failed to copy frame: " << e.what() << std::endl;
        }
    } else {
        std::cout << "WARNING: Video is playing but currentFrame is empty for " << lastVideo->clipPath << std::endl;
    }
}

void VideoPlayer::getCompositeFrame(cv::Mat& frame) {
    // IMPORTANT: We need to update the composite frame first!
    createCompositeFrame();
    
    std::lock_guard<std::mutex> lock(videosMutex);
    
    if (compositeFrame.empty()) {
        // Return black frame if nothing is playing
        frame = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
        std::cout << "DEBUG: Returning black frame (no composite)" << std::endl;
    } else {
        compositeFrame.copyTo(frame);
        std::cout << "DEBUG: Returning composite frame " << frame.cols << "x" << frame.rows << std::endl;
    }
}