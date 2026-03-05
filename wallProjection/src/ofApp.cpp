#include "ofApp.h"
using namespace std;

//--------------------------------------------------------------
void ofApp::setup() {
	settings = ofLoadJson("settings.json");

	// init warper
	int w = 0;
	int h = 0;
	int i = 0;
	for (auto & s : settings["screens"]) {
		w = max(w, s["size"][0].get<int>());
		h += s["size"][1].get<int>();

		int ws = s["size"][0].get<int>();
		int hs = s["size"][1].get<int>();

		warper.push_back(ofxQuadWarp());
		warper.back().setSourceRect(ofRectangle(0, 0, ws, hs)); // this is the source rectangle which is the size of the image and located at ( 0, 0 )
		warper.back().setTopLeftCornerPosition(ofPoint(0, 0)); // this is position of the quad warp corners, centering the image on the screen.
		warper.back().setTopRightCornerPosition(ofPoint(ws, 0)); // this is position of the quad warp corners, centering the image on the screen.
		warper.back().setBottomLeftCornerPosition(ofPoint(0, hs)); // this is position of the quad warp corners, centering the image on the screen.
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
	for (auto & screen : settings["screens"]) {
		int texWt = screen["textureSize"][0].get<int>() + screen["texturePosition"][0].get<int>();
		int texHt = screen["textureSize"][1].get<int>() + screen["texturePosition"][1].get<int>();
		if (texWt > texW) {
			texW = texWt;
		}
		if (texHt > texH) {
			texH = texHt;
		}
	}
	settings["textureDim"] = { texW, texH };

	textureCreation = shared_ptr<Physarum>(new Physarum());
	textureCreation->setup(settings);

	streamManager.setup(settings);

	mouseInput = shared_ptr<MouseInput>(new MouseInput());
	mouseInput->setup(settings);
	for (auto & win : extraWindows) mouseInput->addWindow(win);

	controller = shared_ptr<LidarController>(new LidarController());
	controller->setup(settings);

	textureCreation->registerInputs(controller);
	textureCreation->registerInputs(mouseInput);

	receiver.setup(settings["network"]["oscPortIn"].get<int>());
	for (auto & s : settings["network"]["oscOut"]) {
		sender.push_back(ofxOscSender());
		sender.back().setup(s["ip"].get<std::string>().c_str(), s["port"].get<int>());
	}

	ofAddListener(textureCreation->newOscEvent, this, &ofApp::onOscSendEvent);
	ofAddListener(controller->newOscEvent, this, &ofApp::onOscSendEvent);
	ofAddListener(mouseInput->newOscEvent, this, &ofApp::onOscSendEvent);
}

//--------------------------------------------------------------
void ofApp::update() {

	textureCreation->update();
	controller->update();

	// prepare texture streaming
	for (auto& screen : settings["screens"]) {
		if (screen["screenType"] == "stream") {
			ofRectangle srcRect(
				screen["texturePosition"][0].get<int>(),
				screen["texturePosition"][1].get<int>(),
				screen["textureSize"][0].get<int>(),
				screen["textureSize"][1].get<int>()
			);
			streamManager.update(textureCreation->getDebugTexture("trailMap"), srcRect);
		}
	}



	// check for waiting messages
	while (receiver.hasWaitingMessages()) {
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(m);
		textureCreation->onOscMessage(m);
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	//draw ornament
	ofBackground(0);
	drawScreen(0);

	//draw debug view to see ornament part in camera picture
	if (isDebug) {
		ofPushStyle();

		// panel.draw();
		ofPopStyle();
	}
}

void ofApp::drawWindow(int screenId, ofEventArgs & args) {
	ofBackground(0);
	drawScreen(screenId);
}

void ofApp::drawDebugWindow(ofEventArgs& args) {
	float column1width = 0.7;
	ofBackground(0);
	auto t = textureCreation->getTexture();
	auto s = t.getSize();

	auto win = ofGetCurrentWindow();
    float w = win->getWidth()*column1width;
    float h = win->getHeight();

    float scale = min(w / s.x, h / s.y);

    float drawW = s.x * scale;
    float drawH = s.y * scale;

    t.draw(0, 0, drawW, drawH);

    ofPushStyle();
    ofNoFill();
    for (auto& screen : settings["screens"]) {
        float rx = screen["texturePosition"][0].get<int>() * scale;
        float ry = screen["texturePosition"][1].get<int>() * scale;
        float rw = screen["textureSize"][0].get<int>() * scale;
        float rh = screen["textureSize"][1].get<int>() * scale;

        if (screen.value("screenType", "") == "stream") {
        	if(streamManager.streaming()){
         	ofSetColor(0,255,0);
         }else {
         	ofSetColor(255,0,0);
         }
        }else {
        ofSetColor(255);
        }
        ofDrawRectangle(rx, ry, rw, rh);

        ofSetColor(255);
        string label = screen.value("id", "") + " [" + screen.value("screenType", "") + "]";
        ofDrawBitmapStringHighlight(label, rx + 5, ry + 15);
    }
    ofPopStyle();

    auto tInput = textureCreation->getObjectsFbo();
	s = tInput.getSize();

    w = win->getWidth()*(1.0-column1width-0.05);
    h = win->getHeight();
    scale = min(w / s.x, h / s.y);

    drawW = s.x * scale;
    drawH = s.y * scale;

    ofPushMatrix();
    // objects
    ofTranslate(win->getWidth()*(column1width+0.05)-2,0);
    ofDrawRectangle(0, 0, drawW+2, drawH+2);
    tInput.draw(1, 1, drawW, drawH);
    ofDrawBitmapStringHighlight("objects", 5,15);

    // distance
    ofTranslate(0,drawH +15);
    ofDrawRectangle(0, 0, drawW+2, drawH+2);
    textureCreation->getDebugTexture("distanceField").draw(1, 1, drawW, drawH);
    ofDrawBitmapStringHighlight("distanceField", 5,15);

    // trail
    ofTranslate(0,drawH +15);
    ofDrawRectangle(0, 0, drawW+2, drawH+2);
    textureCreation->getDebugTexture("trailMap").draw(1, 1, drawW, drawH);
    ofDrawBitmapStringHighlight("trailMap", 5,15);



    ofPopMatrix();




}

void ofApp::keyPressedWindow(int screenId, ofKeyEventArgs & args) {
	processKeyPressedEvent(args.key, screenId);
}

void ofApp::keyPressedDebugWindow(ofKeyEventArgs & args) {
	processKeyPressedEvent(args.key, -1);
}

void ofApp::exit() {
	streamManager.stop();
	for (size_t i = 0; i < warper.size(); i++) {
		warper[i].save(ofToString(i), "settings.json");
	}
}

void ofApp::onOscSendEvent(ofxOscMessage & m) {
	// maybe as threaded sender
	for (size_t i = 0; i < sender.size(); ++i) {
		if (ofIsStringInString(m.getAddress(), settings["network"]["oscOut"][i]["channel"].get<string>())) {
			sender[i].sendMessage(m, false);
		}
	}
}

void ofApp::drawScreen(int screenId) {
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
	if (isDebug) {
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

	for (int i = 0; i < 9; i++) {
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

void ofApp::processKeyPressedEvent(int key, int screenId) {
	if (key == 'h' || key == 'H') {
		for (size_t i = 0; i < warper.size(); i++) {
			warper[i].hide();
			isDebug = false;
		}
	}
	if (key == 'd' || key == 'D') {
		isDebug = true;
		for (size_t i = 0; i < warper.size(); i++) {
			if (i == screenId) {
				warper[i].enableKeyboardShortcuts();
				warper[i].enableMouseControls();
				warper[i].show();

			} else {
				warper[i].disableKeyboardShortcuts();
				warper[i].disableMouseControls();
				warper[i].hide();
			}
		}
	}

	if (key == 'l' || key == 'L') {
		for (size_t i = 0; i < warper.size(); i++) {
			warper[i].load(ofToString(i), "settings.json");
		}
	}

	if (key == 's' || key == 'S') {
		for (size_t i = 0; i < warper.size(); i++) {
			warper[i].save(ofToString(i), "settings.json");
		}
	}
	if (key == 'r') {
		settings = ofLoadJson("settings.json");
	}
	if (key == 'R') {
		textureCreation->reloadShaders();
	}
	if (key == 'f') {
		ofToggleFullscreen();
	}
	if (key == 't' || key == 'T') {
		ofRectangle srcRect;
		for (auto& screen : settings["screens"]) {
			if (screen["screenType"] == "stream") {
				srcRect.set(
					screen["texturePosition"][0].get<int>(),
					screen["texturePosition"][1].get<int>(),
					screen["textureSize"][0].get<int>(),
					screen["textureSize"][1].get<int>()
				);
				break;
			}
		}
		streamManager.toggle(textureCreation->getTexture(), srcRect);
	}
	if (key == 'q') {
		ofSetWindowPosition(0, 0);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	processKeyPressedEvent(key, 0);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}
