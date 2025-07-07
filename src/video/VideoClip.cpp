#include "video/VideoClip.h"

VideoClip::VideoClip(const std::string& path, int startNote, int stopNote) 
    : videoPath(path), startNote(startNote), stopNote(stopNote), playing(false) {
}