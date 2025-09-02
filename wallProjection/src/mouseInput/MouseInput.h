#ifndef MOUSEINPUT_H
#define MOUSEINPUT_H

#pragma once
#include "GenericInput.h"
#include "ofMain.h"

class MouseInput: public GenericInput
{
public:
    MouseInput();
    ~MouseInput();
    void setup(ofJson settings) override;
    


    void mousePressed(ofMouseEventArgs &args);
    void mouseMoved(ofMouseEventArgs &args);
    void mouseReleased(ofMouseEventArgs &args);

    void keyPressed(ofKeyEventArgs &args);

    void updateTexture(ofMouseEventArgs &args);

private:
    int currentId = 0;
    glm::vec2 dimensions;
    glm::vec2 screen;
};

#endif