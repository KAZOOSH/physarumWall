#include "ofApp.h"

#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <csignal>
#endif

//--------------------------------------------------------------
void ofApp::setup(){
    settings = ofLoadJson("settings.json");

    // init warper
    int w = 0;
    int h = 0;
    int i = 0;
    for (auto &s : settings["screens"])
    {
        w = max(w, s["size"][0].get<int>());
        h += s["size"][1].get<int>();

        int ws = s["size"][0].get<int>();
        int hs = s["size"][1].get<int>();

        warper.push_back(ofxQuadWarp());
        warper.back().setSourceRect(ofRectangle(0, 0, ws, hs));      // this is the source rectangle which is the size of the image and located at ( 0, 0 )
        warper.back().setTopLeftCornerPosition(ofPoint(0, 0));       // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setTopRightCornerPosition(ofPoint(ws, 0));     // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setBottomLeftCornerPosition(ofPoint(0, hs));   // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setBottomRightCornerPosition(ofPoint(ws, hs)); // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setup();
        warper.back().disableKeyboardShortcuts();
        warper.back().disableMouseControls();
        warper.back().hide();
        warper.back().load(ofToString(i), "settings.json");
        ++i;
    }


    // calculate texture size
    int texW = 0;
    int texH = 0;
    for (auto& screen : settings["screens"]){
        int texWt = screen["textureSize"][0].get<int>() + screen["texturePosition"][0].get<int>();
        int texHt = screen["textureSize"][1].get<int>() + screen["texturePosition"][1].get<int>();
        if(texWt > texW){
            texW = texWt;
        }
        if(texHt > texH){
            texH = texHt;
        }
    }
    settings["textureDim"]={texW,texH};

    textureCreation = shared_ptr<Physarum>(new Physarum());
    textureCreation->setup(settings);

    // streaming config (optional block in settings.json)
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

    mouseInput = shared_ptr<MouseInput>(new MouseInput());
    mouseInput->setup(settings);


    controller = shared_ptr<LidarController>(new LidarController());
    controller->setup(settings);

    textureCreation->registerInputs(controller);
    textureCreation->registerInputs(mouseInput);


    receiver.setup(settings["network"]["oscPortIn"].get<int>());
    for (auto& s:settings["network"]["oscOut"])
    {
        sender.push_back(ofxOscSender());
        sender.back().setup(s["ip"].get<std::string>().c_str(),s["port"].get<int>());
    }

    ofAddListener(textureCreation->newOscEvent,this,&ofApp::onOscSendEvent);
    ofAddListener(controller->newOscEvent,this,&ofApp::onOscSendEvent);
    ofAddListener(mouseInput->newOscEvent,this,&ofApp::onOscSendEvent);

    //ofSetWindowPosition(-1920,0);
   // cout << ofg() <<endl;
}


//--------------------------------------------------------------
void ofApp::update(){

    textureCreation->update();
    controller->update();



    // --- streaming: rate-limited GPU readback + signal worker thread ---
    if (isStreaming && ffmpegPipe) {
        if (ofGetElapsedTimef() - lastStreamTime >= 1.0f / streamFps) {
            // try_lock: skip frame if worker is mid-copy to avoid blocking GL thread
            if (streamMutex.try_lock()) {
                lastStreamTime = ofGetElapsedTimef();
                textureCreation->getTexture().readToPixels(streamPixels);
                streamFrameReady = true;
                streamMutex.unlock();
                streamCv.notify_one();
            }
        }
    }

   // check for waiting messages
	while(receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(m);
        textureCreation->onOscMessage(m);
	}

}

//--------------------------------------------------------------
void ofApp::draw(){
    //draw ornament
    ofBackground(0);
    drawScreen(0);

    //draw debug view to see ornament part in camera picture
    if(isDebug){
        ofPushStyle();

       // panel.draw();
        ofPopStyle();
    }

    if (isCaptureTexture){
        string filename = "capture/";
        if(nImagesCaptured <10) filename+="0";
        if(nImagesCaptured <100) filename+="0";
        if(nImagesCaptured <1000) filename+="0";
        filename += (ofToString(nImagesCaptured));
        filename +=".png";
        textureCreation->saveTextureToFile(filename);
        nImagesCaptured++;
    }
}


void ofApp::drawWindow2(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(1);
}

void ofApp::drawWindow3(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(2);
}

void ofApp::drawWindow4(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(3);
}

void ofApp::keyPressedWindow2(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 1);
}

void ofApp::keyPressedWindow3(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 2);
}

void ofApp::keyPressedWindow4(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 3);
}

void ofApp::exit()
{
    stopStreaming();
    for (size_t i = 0; i < warper.size(); i++)
    {
        warper[i].save(ofToString(i), "settings.json");
    }
}

void ofApp::onOscSendEvent(ofxOscMessage &m)
{
    // maybe as threaded sender
    for (size_t i=0;i<sender.size();++i)
    {
        if(ofIsStringInString(m.getAddress(),settings["network"]["oscOut"][i]["channel"].get<string>()) ){
            sender[i].sendMessage(m, false);

        }

    }


}




void ofApp::startStreaming() {
    if (isStreaming) return;

    streamWidth  = (int)textureCreation->getTexture().getWidth();
    streamHeight = (int)textureCreation->getTexture().getHeight();
    if (streamWidth <= 0 || streamHeight <= 0) {
        ofLogError("ofApp") << "Texture not ready for streaming";
        return;
    }
    if (!streamPixels.isAllocated() ||
        (int)streamPixels.getWidth()  != streamWidth ||
        (int)streamPixels.getHeight() != streamHeight) {
        streamPixels.allocate(streamWidth, streamHeight, OF_PIXELS_RGBA);
    }

    // Encoder flags are for h264_nvenc (low-latency).
    // For CPU encoding swap encoder to libx264 and flags to:
    //   -preset ultrafast -tune zerolatency
    // Add -vf vflip if the stream appears upside-down.
    bool isNvenc = (streamEncoder.find("nvenc") != std::string::npos);
    bool isRtmp  = (streamUrl.rfind("rtmp", 0) == 0);

    std::string encoderFlags = isNvenc
        ? " -preset llhq -tune ll"
        : " -preset ultrafast -tune zerolatency";

    std::string outputFlags = isRtmp
        ? " -f flv"
        : " -rtsp_transport tcp -f rtsp";

    std::string cmd =
        "/usr/bin/ffmpeg -y"
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
        " 2>/tmp/ffmpeg_stream.log";

#ifdef __linux__
    // Mark all open FDs as close-on-exec so the FFmpeg child process does not
    // inherit the Wayland/X11 display connection (causes display errors).
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

    ofLogNotice("ofApp") << "FFmpeg cmd: " << cmd;
    signal(SIGPIPE, SIG_IGN);  // prevent broken pipe from killing the process
    ffmpegPipe = popen(cmd.c_str(), "w");
    if (!ffmpegPipe) {
        ofLogError("ofApp") << "Failed to open FFmpeg pipe: " << cmd;
        return;
    }
    streamRunning = true;
    isStreaming   = true;
    streamThread  = std::thread(&ofApp::streamWorker, this);
    ofLogNotice("ofApp") << "Streaming -> " << streamUrl
                         << " (" << streamWidth << "x" << streamHeight
                         << " @ " << streamFps << "fps, " << streamEncoder << ")";
}

void ofApp::stopStreaming() {
    if (!isStreaming) return;
    isStreaming   = false;
    streamRunning = false;
    streamCv.notify_all();
    if (streamThread.joinable()) streamThread.join();
    if (ffmpegPipe) { pclose(ffmpegPipe); ffmpegPipe = nullptr; }
    ofLogNotice("ofApp") << "Streaming stopped";
}

// Worker thread: waits for a new frame, copies pixels under the mutex,
// then writes to the FFmpeg pipe without holding the lock.
void ofApp::streamWorker() {
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
            ofLogError("ofApp") << "FFmpeg pipe write failed — stopping stream";
            streamRunning = false;
            isStreaming = false;
            break;
        }
        fflush(ffmpegPipe);
    }
}

void ofApp::drawScreen(int screenId)
{
    auto settingsScreen = settings["screens"][screenId];

    //======================== get our quad warp matrix.

    ofMatrix4x4 mat = warper[screenId].getMatrix();

    //======================== use the matrix to transform our fbo.

    ofPushMatrix();
    ofMultMatrix(mat);

    ofSetColor(255);
    //textureCreation->getTexture().draw(0,0);
    //textureCreation->draw();


    //ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    textureCreation->getTexture().drawSubsection(
        ofRectangle(0, 0, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()),
        ofRectangle(settingsScreen["texturePosition"][0], settingsScreen["texturePosition"][1], settingsScreen["textureSize"][0].get<int>(), settingsScreen["textureSize"][1].get<int>()));
    //ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    //ofSetColor(255,blendIntensity*255);
    if(isDebug){
        ofSetColor(255);
        controller->updateTexture();
        controller->getDebugTexture().drawSubsection(
            ofRectangle(0, 0, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()),
            ofRectangle(settingsScreen["texturePosition"][0], settingsScreen["texturePosition"][1], settingsScreen["textureSize"][0].get<int>(), settingsScreen["textureSize"][1].get<int>()));
    }

    ofEnableAlphaBlending();
    ofPopMatrix();

    //======================== use the matrix to transform points.

    ofSetLineWidth(2);
    ofSetColor(ofColor::cyan);

    for (int i = 0; i < 9; i++)
    {
        int j = i + 1;

        ofVec3f p1 = mat.preMult(ofVec3f(points[i].x, points[i].y, 0));
        ofVec3f p2 = mat.preMult(ofVec3f(points[j].x, points[j].y, 0));

        ofDrawLine(p1.x, p1.y, p2.x, p2.y);
    }

    //======================== draw quad warp ui.

    ofSetColor(ofColor::magenta);
    warper[screenId].drawQuadOutline();

    ofSetColor(ofColor::yellow);
    warper[screenId].drawCorners();

    ofSetColor(ofColor::magenta);
    warper[screenId].drawHighlightedCorner();

    ofSetColor(ofColor::red);
    warper[screenId].drawSelectedCorner();
    ofSetColor(255);
}

void ofApp::processKeyPressedEvent(int key, int screenId)
{
    if (key == 'h' || key == 'H')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].hide();
            isDebug = false;
        }
    }
    if (key == 'd' || key == 'D')
    {
        isDebug = true;
        for (size_t i = 0; i < warper.size(); i++)
        {
            if (i == screenId)
            {
                warper[i].enableKeyboardShortcuts();
                warper[i].enableMouseControls();
                warper[i].show();

            }
            else
            {
                warper[i].disableKeyboardShortcuts();
                warper[i].disableMouseControls();
                warper[i].hide();
            }
        }
    }

    if (key == 'l' || key == 'L')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].load(ofToString(i), "settings.json");
        }
    }

    if (key == 's' || key == 'S')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].save(ofToString(i), "settings.json");
        }
    }
    if (key == 'r')
    {
        settings = ofLoadJson("settings.json");
    }
    if (key == 'f') {
        ofToggleFullscreen();
    }
    if (key == 't' || key == 'T') {
        if (isStreaming) stopStreaming();
        else startStreaming();
    }
    if (key == 'q') {
        ofSetWindowPosition(0,0);
    }

    if(key == 'l'){
        isCaptureTexture = !isCaptureTexture;
    }
}


//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
    processKeyPressedEvent(key,0);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
