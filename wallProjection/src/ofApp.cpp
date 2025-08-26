#include "ofApp.h"

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

    //receiver.setup(settings["network"]["port"]); 

    // calculate texture size
    int texW = 0;
    int texH = 0;
    for (auto& screen : settings["screens"]){
        int texWt = screen["size"][0].get<int>() + screen["texturePosition"][0].get<int>(); 
        int texHt = screen["size"][1].get<int>() + screen["texturePosition"][1].get<int>();
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


    mouseInput = shared_ptr<MouseInput>(new MouseInput());
    mouseInput->setup(settings);

    
    controller = shared_ptr<LidarController>(new LidarController());
    controller->setup(settings);

    textureCreation->registerInputs(controller);
    textureCreation->registerInputs(mouseInput);


    receiver.setup(settings["network"]["oscPortIn"].get<int>());
    sender.setup(settings["network"]["oscIpOut"].get<std::string>().c_str(),settings["network"]["oscPortOut"].get<int>());
    

    ofAddListener(textureCreation->newOscMessageEvent,this,&ofApp::onOscSendEvent);
    ofAddListener(controller->newOscMessageEvent,this,&ofApp::onOscSendEvent);
    ofAddListener(mouseInput->newOscMessageEvent,this,&ofApp::onOscSendEvent);

    //ofSetWindowPosition(-1920,0);
   // cout << ofg() <<endl;
}


//--------------------------------------------------------------
void ofApp::update(){
    
    textureCreation->update();
    controller->update();
    
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
    for (size_t i = 0; i < warper.size(); i++)
    {
        warper[i].save(ofToString(i), "settings.json");
    }
}

void ofApp::onOscSendEvent(ofxOscMessage &m)
{
    // maybe as threaded sender
    sender.sendMessage(m, false);
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
        ofRectangle(settingsScreen["texturePosition"][0], settingsScreen["texturePosition"][1], settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()));
    //ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    //ofSetColor(255,blendIntensity*255);
    if(isDebug){
        ofSetColor(255);
        controller->updateTexture();
        controller->getDebugTexture().drawSubsection(
            ofRectangle(0, 0, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()),
            ofRectangle(settingsScreen["texturePosition"][0], settingsScreen["texturePosition"][1], settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()));
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
    if (key == 'q') {
        ofSetWindowPosition(0,0);
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

