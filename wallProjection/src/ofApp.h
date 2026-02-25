#pragma once

#include "ofMain.h"
#include "ofxQuadWarp.h"
#include "ofxOsc.h"
//#include "ofxOpenCv.h"
#include "Physarum.h"

#include "GenericInput.h"
#include "MouseInput.h"
#include "LidarController.h"
#include "StreamManager.h"



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

    void drawWindow(int screenId, ofEventArgs& args);
    void drawDebugWindow(ofEventArgs& args);
    void keyPressedWindow(int screenId, ofKeyEventArgs& args);
    void keyPressedDebugWindow(ofKeyEventArgs& args);
    void exit();

    void onOscSendEvent(ofxOscMessage& m);

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

    StreamManager streamManager;

};
