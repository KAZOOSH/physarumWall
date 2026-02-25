#pragma once

#include "ofMain.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class StreamManager {
public:
    void setup(const ofJson& settings);

    void update(ofTexture& texture, ofRectangle srcRect = ofRectangle());

    void start(ofTexture& texture, ofRectangle srcRect = ofRectangle());
    void stop();

    bool streaming() const { return isStreaming; }
    void toggle(ofTexture& texture, ofRectangle srcRect = ofRectangle());

private:
    void worker();

    bool isStreaming = false;
    FILE* ffmpegPipe = nullptr;

    ofFbo cropFbo;
    ofPixels streamPixels;
    std::mutex streamMutex;
    std::condition_variable streamCv;
    std::thread streamThread;
    std::atomic<bool> streamRunning{false};
    bool streamFrameReady = false;
    float lastStreamTime = -999.f;

    int streamWidth = 1920;
    int streamHeight = 1080;
    int streamFps = 30;
    std::string streamEncoder = "h264_nvenc";
    std::string streamUrl = "rtsp://0.0.0.0:8554/live";
    std::string streamBitrate = "10M";
};
