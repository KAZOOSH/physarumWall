#include "GenericInput.h"

GenericInput::GenericInput()
{

}

GenericInput::~GenericInput()
{

}

void GenericInput::setup(ofJson settings_)
{
    settings = settings_;
    debugFbo.allocate(1920*3,1080);
    sender.setup(settings["network"]["oscDst"].get<std::string>().c_str(),settings["network"]["oscPort"].get<int>());

    ofAddListener(interactionStart,this,&GenericInput::onTouchOscTouch);
    ofAddListener(interactionMove,this,&GenericInput::onTouchOscTouch);
    ofAddListener(interactionEnd,this,&GenericInput::onTouchOscTouch);
}

void GenericInput::onTouchOscTouch(ofTouchEventArgs& args){
    ofxOscMessage m;
	//m.setAddress("/live/clip/fire");
    //m.addStringArg("Piano");
    //m.addStringArg("Clip Test");
    //std::cout << "send osc" <<std::endl;
	//m.addFloatArg(ofMap(ofGetMouseX(), 0, ofGetWidth(), 0.f, 1.f, true));
	//m.addFloatArg(ofMap(ofGetMouseY(), 0, ofGetHeight(), 0.f, 1.f, true));
	//sender.sendMessage(m, false);
}