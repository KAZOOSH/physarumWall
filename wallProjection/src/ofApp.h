#pragma once

#include "ofMain.h"
#include "ofxQuadWarp.h"
#include "ofxOsc.h"
//#include "ofxOpenCv.h"
#include "Physarum.h"

#include "GenericInput.h"
#include "MouseInput.h"
#include "LidarController.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ofApp : public ofBaseApp{

public:

    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

        void drawWindow2(ofEventArgs& args);
    void drawWindow3(ofEventArgs& args);
    void drawWindow4(ofEventArgs& args);
    void keyPressedWindow2(ofKeyEventArgs& args);
    void keyPressedWindow3(ofKeyEventArgs& args);
    void keyPressedWindow4(ofKeyEventArgs& args);
    void exit();

    void onOscSendEvent(ofxOscMessage& m);
    void onMidiSyncEvent(ofxOscMessage& m);

    void drawScreen(int screenId);
    void processKeyPressedEvent(int key, int screenId);

    ofxOscReceiver receiver;
    vector<ofxOscSender> sender;


    vector<ofxQuadWarp> warper;
    ofJson settings;

    bool isDebug = false;
    ofPoint points[10];
    float blendIntensity = 1.0;
    float tx = 1.0;

    bool isCaptureTexture = false;
    int nImagesCaptured = 0;

  shared_ptr<Physarum> textureCreation;

  shared_ptr<LidarController> controller;
  shared_ptr<MouseInput> mouseInput;
 // ofxOscReceiver receiver;

    // --- FFmpeg RTSP streaming ---
    void startStreaming();
    void stopStreaming();
    void streamWorker();

    bool isStreaming = false;
    FILE* ffmpegPipe = nullptr;

    ofPixels streamPixels;          // GL thread writes here (under mutex)
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
