#include "LidarController.h"

LidarController::LidarController()
{
}

LidarController::~LidarController()
{
}

void LidarController::setup(ofJson settings)
{
    GenericInput::setup(settings);
    for (auto &screen : settings["screens"])
    {
        screenMaps.push_back(ScreenMap());
        screenMaps.back().wallDim = ofRectangle(screen["worldDimensions"]["x"].get<int>(),
                                                  screen["worldDimensions"]["y"].get<int>(),
                                                  screen["worldDimensions"]["width"].get<int>(),
                                                  screen["worldDimensions"]["height"].get<int>());
        screenMaps.back().screenDim = ofRectangle(screen["texturePosition"][0].get<int>(),
                                                  screen["texturePosition"][1].get<int>(),
                                                  screen["size"][0].get<int>(),
                                                  screen["size"][1].get<int>());
    }
    for (auto &lidar : settings["lidars"])
    {
        sensors.push_back(shared_ptr<Ms200Receiver>(new Ms200Receiver()));
        ofJson stats = ofJson();
        stats["lidar"] = lidar;
        for (auto &screen : settings["screens"])
        {
            if (screen["id"] == lidar["screenId"])
            {
                stats["screen"] = screen;
            }
        }
        // cout << stats.dump(4) <<endl;
        sensors.back()->setup(stats);
        registerInputs(sensors.back());
    }
}

void LidarController::registerInputs(shared_ptr<GenericInput> input)
{
    for (auto &sensor : sensors)
    {
        ofAddListener(input->interactionStart, this, &LidarController::onTouchDown);
        ofAddListener(input->interactionMove, this, &LidarController::onTouchMove);
        ofAddListener(input->interactionEnd, this, &LidarController::onTouchUp);
    }
}

void LidarController::onTouchMove(ofTouchEventArgs &ev)
{
    touches[ev.id] = ev;
    //cout << "move " << ev.x << "  " << ev.y << endl;
    mapTouchToTexCoords(ev);
    interactionMove.notify(ev);
    // updateTexture();
}

void LidarController::mapTouchToTexCoords(ofTouchEventArgs &ev)
{
    ofVec2f pos = ofVec2f(ev.x,ev.y);
    for(auto& m:screenMaps){
        if(m.wallDim.inside(pos)){
            ev.x = ofMap(ev.x,m.wallDim.x,m.wallDim.x+m.wallDim.width,m.screenDim.x,m.screenDim.x+m.screenDim.width);
            ev.y = ofMap(ev.y,m.wallDim.y,m.wallDim.y+m.wallDim.height,m.screenDim.y,m.screenDim.y+m.screenDim.height);
        }
    }
}

void LidarController::updateTexture()
{
   debugFbo.begin();
    ofClear(0, 0);
    ofSetColor(255, 0, 0);

    
    auto samples = sensors[0]->getSamples();
    for (size_t i = 0; i < samples.size(); i++)
    {


        float angle0;
        if(sensors[0]->isMirror){
        angle0 = TWO_PI-(samples[i].angle_z_q14 *TWO_PI/65536) + (sensors[0]->rotation * PI / 180);
       }else{
            angle0 = (samples[i].angle_z_q14 *TWO_PI/65536) + (sensors[0]->rotation * PI / 180);
        }
        float angle1;
        if(sensors[0]->isMirror){
            angle1 = TWO_PI - (samples[(i+1)% samples.size()].angle_z_q14* TWO_PI/65536) + (sensors[0]->rotation * PI / 180);
        }else{
            angle1 = (samples[(i+1)% samples.size()].angle_z_q14* TWO_PI/65536)+ (sensors[0]->rotation * PI / 180);
        }
                  

        auto m = screenMaps[0];
        auto pos0 = ofVec2f(round(samples[i].dist_mm_q2*DIST_TO_MM * cos(angle0)), round(samples[i].dist_mm_q2 *DIST_TO_MM* sin(angle0))) + sensors[0]->position;
        pos0.x = ofMap(pos0.x,m.wallDim.x,m.wallDim.x+m.wallDim.width,m.screenDim.x,m.screenDim.x+m.screenDim.width);
        pos0.y = ofMap(pos0.y,m.wallDim.y,m.wallDim.y+m.wallDim.height,m.screenDim.y,m.screenDim.y+m.screenDim.height);

        auto pos1 = ofVec2f(round(samples[(i + 1) % samples.size()].dist_mm_q2*DIST_TO_MM * cos(angle1)), round(samples[(i + 1) % samples.size()].dist_mm_q2 *DIST_TO_MM* sin(angle1))) + sensors[0]->position;
        pos1.x = ofMap(pos1.x,m.wallDim.x,m.wallDim.x+m.wallDim.width,m.screenDim.x,m.screenDim.x+m.screenDim.width);
        pos1.y = ofMap(pos1.y,m.wallDim.y,m.wallDim.y+m.wallDim.height,m.screenDim.y,m.screenDim.y+m.screenDim.height);

        ofDrawLine(pos0.x,pos0.y,pos1.x,pos1.y);
    }
/*
    auto c = sensors[0]->getClusters();
    for (auto &touch : c)
    {
        float angle0 = ofMap(touch.second.meanAngle, 0, 65536, 0, TWO_PI)+ (sensors[0]->rotation * 180 / PI);

        auto m = screenMaps[0];
        auto pos0 = ofVec2f(round(touch.second.meanDist*DIST_TO_MM * cos(angle0)), round(touch.second.meanDist *DIST_TO_MM* sin(angle0))) + sensors[0]->position;
        pos0.x = ofMap(pos0.x,m.wallDim.x,m.wallDim.x+m.wallDim.width,m.screenDim.x,m.screenDim.x+m.screenDim.width);
        pos0.y = ofMap(pos0.y,m.wallDim.y,m.wallDim.y+m.wallDim.height,m.screenDim.y,m.screenDim.y+m.screenDim.height);

        ofSetColor(0, 255, 0);
        ofDrawCircle(pos0.x, pos0.y, 10);
        ofSetColor(255);
        ofDrawBitmapString(ofToString(touch.first), pos0.x - 4, pos0.y + 4);
    }
*/
    for (auto &touch : touches)
    {

        ofSetColor(255, 0, 0);
        ofDrawCircle(touch.second.x, touch.second.y, 10);
        ofSetColor(255);
        ofDrawBitmapString(ofToString(touch.first), touch.second.x - 4, touch.second.y + 4);
    }
    debugFbo.end();
}

void LidarController::onTouchUp(ofTouchEventArgs &ev)
{
    touches.erase(ev.id);
    mapTouchToTexCoords(ev);
    interactionEnd.notify(ev);
    // cout << "up " << ev.x << "  " << ev.y << endl;
    // updateTexture();
}

void LidarController::onTouchDown(ofTouchEventArgs &ev)
{
    touches[ev.id] = ev;
    cout << "down ->" << ev.id << " : "<<ev.x << "  " << ev.y << endl;
    mapTouchToTexCoords(ev);
    interactionStart.notify(ev);
    // updateTexture();
}
