#ifndef LIDARCONTROLLER_H
#define LIDARCONTROLLER_H

#pragma once
#include "ofMain.h"
#include "GenericInput.h"
#include "Ms200Receiver.h"


class ScreenMap{
    public:
        ofRectangle wallDim;
        ofRectangle screenDim;
};

class LidarController : public GenericInput
{
public:
    LidarController();
    ~LidarController();

    void setup(ofJson settings) override;

    void registerInputs(shared_ptr<GenericInput> input);

    void onTouchDown(ofTouchEventArgs& ev);
    void onTouchUp(ofTouchEventArgs& ev);
    void onTouchMove(ofTouchEventArgs& ev);

    void mapTouchToTexCoords(ofTouchEventArgs& ev);

    void updateTexture();

private:
    vector<shared_ptr<Ms200Receiver>> sensors;

    map<int,ofTouchEventArgs> touches;
    vector<ScreenMap> screenMaps;
};

#endif