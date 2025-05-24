#include "MouseInput.h"

MouseInput::MouseInput()
{
}

MouseInput::~MouseInput()
{
}

void MouseInput::setup(ofJson settings)
{
    GenericInput::setup(settings);
    ofAddListener(ofEvents().mousePressed, this, &MouseInput::mousePressed);
    ofAddListener(ofEvents().mouseDragged, this, &MouseInput::mouseMoved);
    ofAddListener(ofEvents().mouseReleased, this, &MouseInput::mouseReleased);
    ofAddListener(ofEvents().keyPressed,this,&MouseInput::keyPressed);
}

void MouseInput::mousePressed(ofMouseEventArgs &args)
{
    ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::down, args.x, args.y, currentId);
    
    interactionStart.notify(t);
    updateTexture(args);
}

void MouseInput::mouseReleased(ofMouseEventArgs &args)
{
    ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::up, args.x, args.y, currentId);
    interactionEnd.notify(t);
    updateTexture(args);
}

void MouseInput::mouseMoved(ofMouseEventArgs &args)
{

        ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::move, args.x, args.y, currentId);
        interactionMove.notify(t);
        updateTexture(args);

}

void MouseInput::keyPressed(ofKeyEventArgs &args)
{
    switch (args.key)
    {
    case '0':
        currentId = 0;
        break;
    case '1':
        currentId = 1;
        break;
    case '2':
        currentId = 2;
        break;
    case '3':
        currentId = 3;
        break;
    default:
        break;
    }
}

void MouseInput::updateTexture(ofMouseEventArgs &args)
{
    debugFbo.begin();
    ofClear(0,0);
    if(args.type != args.Exited){
        ofSetColor(255,0,0);
        ofDrawCircle(args.x,args.y,10);
        ofSetColor(255);
        ofDrawBitmapString(ofToString(currentId),args.x-4,args.y+4);
    }
    debugFbo.end();
}


