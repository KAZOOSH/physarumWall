#include "MouseInput.h"
#include "ofVec2f.h"

MouseInput::MouseInput()
{
}

MouseInput::~MouseInput()
{
}

void MouseInput::setup(ofJson settings)
{
    GenericInput::setup(settings);
    /*ofAddListener(ofEvents().mousePressed, this, &MouseInput::mousePressed);
    ofAddListener(ofEvents().mouseDragged, this, &MouseInput::mouseMoved);
    ofAddListener(ofEvents().mouseReleased, this, &MouseInput::mouseReleased);
    ofAddListener(ofEvents().keyPressed, this, &MouseInput::keyPressed);
    ofAddListener(ofEvents().mouseScrolled, this, &MouseInput::mouseScrolled);*/

    auto dim = settings["screens"].back()["worldDimensions"];
    dimensions.x = dim["x"].get<int>() + dim["width"].get<int>();
    dimensions.y = dim["y"].get<int>() + dim["height"].get<int>();

    for (auto &s : settings["screens"])
    {
        screen.x += s["size"][0].get<int>();
        screen.y = s["size"][1].get<int>();
    }
}

void MouseInput::mousePressed(ofMouseEventArgs &args)
{
    //cout << "clicked" <<endl;
    ofVec2f p = getMousePosOnTexture(args);
    ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::down, p.x,p.y, currentId);
    t.width = rTouch;
    t.height = rTouch;

    lastPos = glm::vec2(p.x,p.y);

    interactionStart.notify(t);
    updateTexture(args);
}

void MouseInput::mouseReleased(ofMouseEventArgs &args)
{
    ofVec2f p = getMousePosOnTexture(args);
    ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::up, p.x,p.y, currentId);
    ofVec2f v = p-lastPos;
    t.xspeed = v.x;
    t.yspeed = v.y;

    interactionEnd.notify(t);
    updateTexture(args);
}

void MouseInput::mouseMoved(ofMouseEventArgs &args)
{
    ofVec2f p = getMousePosOnTexture(args);
    ofVec2f v = p-lastPos;
    ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::move, p.x,p.y, currentId);
    t.width = rTouch;
    t.height = rTouch;
    t.xspeed = v.x;
    t.yspeed = v.y;

    interactionMove.notify(t);
    updateTexture(args);

    lastPos = p;
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

void MouseInput::addWindow(shared_ptr<ofAppBaseWindow> window)
{
  ofAddListener(window->events().mouseScrolled, this, &MouseInput::mouseScrolled);
  ofAddListener(window->events().mousePressed, this, &MouseInput::mousePressed);
  ofAddListener(window->events().mouseDragged, this, &MouseInput::mouseMoved);
  ofAddListener(window->events().mouseReleased, this, &MouseInput::mouseReleased);
  ofAddListener(window->events().keyPressed, this, &MouseInput::keyPressed);

}

void MouseInput::mouseScrolled(ofMouseEventArgs &args)
{
    rTouch = std::max(1, rTouch + (int)args.scrollY*3);
}

ofVec2f MouseInput::getMousePosOnTexture(ofMouseEventArgs &args)
{
    glm::vec2 winPos = ofGetCurrentWindow() ? ofGetCurrentWindow()->getWindowPosition() : glm::vec2(0, 0);

    return ofVec2f(
        ofMap(args.x + winPos.x, 0, screen.x, 0, dimensions.x, true),
        ofMap(args.y + winPos.y, 0, screen.y, 0, dimensions.y, true)
    );
}

void MouseInput::updateTexture(ofMouseEventArgs &args)
{
    debugFbo.begin();
    ofClear(0, 0);
    if (args.type != args.Exited)
    {
        ofSetColor(255, 0, 0);
        ofDrawCircle(args.x, args.y, 10);
        ofSetColor(255);
        ofDrawBitmapString(ofToString(currentId), args.x - 4, args.y + 4);
    }
    debugFbo.end();
}
