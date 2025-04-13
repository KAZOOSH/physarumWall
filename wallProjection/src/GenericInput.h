#ifndef GENERICINPUT_H
#define GENERICINPUT_H

#pragma once
#include "ofEvent.h"
#include "ofEvents.h"
#include "ofJson.h"
#include "ofTexture.h"
#include "ofFbo.h"
#include "ofxOsc.h"


class GenericInput
{
public:
    GenericInput();
    ~GenericInput();

    virtual void setup(ofJson settings);
    ofTexture getDebugTexture(){return debugFbo.getTexture();};

    ofEvent<ofTouchEventArgs> interactionStart;
    ofEvent<ofTouchEventArgs> interactionEnd;
    ofEvent<ofTouchEventArgs> interactionMove;

    void onTouchOscTouch(ofTouchEventArgs& args);
protected:
   

    ofJson settings;
    ofFbo debugFbo;

private:
    ofxOscSender sender;

};

#endif