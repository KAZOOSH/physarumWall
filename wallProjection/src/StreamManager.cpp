#include "StreamManager.h"

#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <csignal>
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

void StreamManager::setup(const ofJson& settings) {
    if (settings.contains("streaming")) {
        auto& sc = settings["streaming"];
        streamWidth   = sc.value("width",    streamWidth);
        streamHeight  = sc.value("height",   streamHeight);
        streamFps     = sc.value("fps",      streamFps);
        streamEncoder = sc.value("encoder",  streamEncoder);
        streamUrl     = sc.value("url",      streamUrl);
        streamBitrate = sc.value("bitrate",  streamBitrate);
    }
    streamPixels.allocate(streamWidth, streamHeight, OF_PIXELS_RGBA);
}

void StreamManager::update(ofTexture& texture, ofRectangle srcRect) {
    if (!isStreaming || !ffmpegPipe) return;
    if (ofGetElapsedTimef() - lastStreamTime >= 1.0f / streamFps) {
        if (streamMutex.try_lock()) {
            lastStreamTime = ofGetElapsedTimef();
            if (srcRect.width > 0 && srcRect.height > 0) {
                if (!cropFbo.isAllocated() ||
                    (int)cropFbo.getWidth()  != streamWidth ||
                    (int)cropFbo.getHeight() != streamHeight) {
                    cropFbo.allocate(streamWidth, streamHeight, GL_RGBA);
                }
                cropFbo.begin();
                ofClear(0);
                texture.drawSubsection(0, 0, streamWidth, streamHeight,
                    srcRect.x, srcRect.y, srcRect.width, srcRect.height);
                cropFbo.end();
                cropFbo.readToPixels(streamPixels);
            } else {
                texture.readToPixels(streamPixels);
            }
            streamFrameReady = true;
            streamMutex.unlock();
            streamCv.notify_one();
        }
    }
}

void StreamManager::start(ofTexture& texture, ofRectangle srcRect) {
    if (isStreaming) return;

    if (srcRect.width > 0 && srcRect.height > 0) {
        streamWidth  = (int)srcRect.width;
        streamHeight = (int)srcRect.height;
    } else {
        streamWidth  = (int)texture.getWidth();
        streamHeight = (int)texture.getHeight();
    }
    if (streamWidth <= 0 || streamHeight <= 0) {
        ofLogError("StreamManager") << "Texture not ready for streaming";
        return;
    }
    if (!streamPixels.isAllocated() ||
        (int)streamPixels.getWidth()  != streamWidth ||
        (int)streamPixels.getHeight() != streamHeight) {
        streamPixels.allocate(streamWidth, streamHeight, OF_PIXELS_RGBA);
    }

    bool isNvenc = (streamEncoder.find("nvenc") != std::string::npos);
    bool isRtmp  = (streamUrl.rfind("rtmp", 0) == 0);

    std::string encoderFlags = isNvenc
        ? " -preset llhq -tune ll"
        : " -preset ultrafast -tune zerolatency";

    std::string outputFlags = isRtmp
        ? " -f flv"
        : " -rtsp_transport tcp -f rtsp";

#ifdef _WIN32
    const std::string ffmpegBin   = "ffmpeg";
    const std::string logRedirect = " 2>NUL";
#else
    const std::string ffmpegBin   = "/usr/bin/ffmpeg";
    const std::string logRedirect = " 2>/tmp/ffmpeg_stream.log";
#endif

    std::string cmd =
        ffmpegBin + " -y"
        " -f rawvideo -pix_fmt rgba"
        " -s " + ofToString(streamWidth) + "x" + ofToString(streamHeight) +
        " -r " + ofToString(streamFps) +
        " -i pipe:0"
        " -c:v " + streamEncoder +
        encoderFlags +
        " -bf 0"
        " -g " + ofToString(streamFps) +
        " -b:v " + streamBitrate +
        outputFlags + " " + streamUrl +
        logRedirect;

#ifdef __linux__
    {
        DIR* fd_dir = opendir("/proc/self/fd");
        if (fd_dir) {
            int dir_fd = dirfd(fd_dir);
            struct dirent* ent;
            while ((ent = readdir(fd_dir)) != nullptr) {
                int fd = atoi(ent->d_name);
                if (fd > 2 && fd != dir_fd) {
                    int flags = fcntl(fd, F_GETFD);
                    if (flags != -1)
                        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
                }
            }
            closedir(fd_dir);
        }
    }
#endif

    ofLogNotice("StreamManager") << "FFmpeg cmd: " << cmd;
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
#ifdef _WIN32
    ffmpegPipe = _popen(cmd.c_str(), "wb");
    if (ffmpegPipe) _setmode(_fileno(ffmpegPipe), _O_BINARY);
#else
    ffmpegPipe = popen(cmd.c_str(), "w");
#endif
    if (!ffmpegPipe) {
        ofLogError("StreamManager") << "Failed to open FFmpeg pipe: " << cmd;
        return;
    }
    streamRunning = true;
    isStreaming   = true;
    streamThread  = std::thread(&StreamManager::worker, this);
    ofLogNotice("StreamManager") << "Streaming -> " << streamUrl
                                 << " (" << streamWidth << "x" << streamHeight
                                 << " @ " << streamFps << "fps, " << streamEncoder << ")";
}

void StreamManager::stop() {
    if (!isStreaming) return;
    isStreaming   = false;
    streamRunning = false;
    streamCv.notify_all();
    if (streamThread.joinable()) streamThread.join();
#ifdef _WIN32
    if (ffmpegPipe) { _pclose(ffmpegPipe); ffmpegPipe = nullptr; }
#else
    if (ffmpegPipe) { pclose(ffmpegPipe); ffmpegPipe = nullptr; }
#endif
    ofLogNotice("StreamManager") << "Streaming stopped";
}

void StreamManager::toggle(ofTexture& texture, ofRectangle srcRect) {
    if (isStreaming) stop();
    else start(texture, srcRect);
}

void StreamManager::worker() {
    ofPixels localBuf;
    while (streamRunning) {
        {
            std::unique_lock<std::mutex> lock(streamMutex);
            streamCv.wait(lock, [this]{ return streamFrameReady || !streamRunning; });
            if (!streamRunning) break;
            localBuf = streamPixels;
            streamFrameReady = false;
        }
        size_t written = fwrite(localBuf.getData(), 1, localBuf.size(), ffmpegPipe);
        if (written == 0) {
            ofLogError("StreamManager") << "FFmpeg pipe write failed — stopping stream";
            streamRunning = false;
            isStreaming = false;
            break;
        }
        fflush(ffmpegPipe);
    }
}
