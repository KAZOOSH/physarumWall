#ifndef TEXTURECREATION_H
#define TEXTURECREATION_H

#pragma once

#include "ofMain.h"
#include "ofFbo.h"
#include "ofEvents.h"
#include "GenericInput.h"
#include "ofEvent.h"
#include "ofxOsc.h"

class TextureCreation
{
public:
    TextureCreation();
    ~TextureCreation();

    virtual void setup(ofJson settings);
    virtual void update() = 0;
    virtual void draw() = 0;
    ofTexture getTexture();

    void registerInputs(shared_ptr<GenericInput> input);

    virtual void onTouchDown(ofTouchEventArgs& ev){};
    virtual void onTouchUp(ofTouchEventArgs& ev){};
    virtual void onTouchMove(ofTouchEventArgs& ev){};

    virtual void onOscMessage(ofxOscMessage m){};
    ofEvent<ofxOscMessage> newOscMessageEvent;

    void saveTextureToFile(string filename);

protected:
    ofFbo fbo;
    
};

#endif