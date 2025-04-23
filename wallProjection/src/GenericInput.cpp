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
    debugFbo.allocate(settings["textureDim"][0],settings["textureDim"][1]);
    sender.setup(settings["network"]["oscDst"].get<std::string>().c_str(),settings["network"]["oscPort"].get<int>());

    ofAddListener(interactionStart,this,&GenericInput::onTouchOscTouch);
    ofAddListener(interactionMove,this,&GenericInput::onTouchOscTouch);
    ofAddListener(interactionEnd,this,&GenericInput::onTouchOscTouch);
}

void GenericInput::onTouchOscTouch(ofTouchEventArgs& args){

    

    ofxOscMessage m;
    if(args.type == ofTouchEventArgs::down
    || args.type == ofTouchEventArgs::move){
        m.setAddress("/midi/note_on");
    }else if(args.type == ofTouchEventArgs::up){
        m.setAddress("/midi/note_off");
    }
        m.addIntArg(args.id);
        m.addIntArg(ofMap(args.x,0,debugFbo.getWidth(),0,127,true));
        m.addIntArg(127);
        m.addIntArg(1);

    sender.sendMessage(m, false);

    ofxOscMessage m2;
    m2.setAddress("/midi/cc");
    m2.addIntArg(0);
    m2.addIntArg(ofMap(args.y,0,debugFbo.getHeight(),0,127,true));
    m2.addIntArg(1);
    sender.sendMessage(m2, false);
}