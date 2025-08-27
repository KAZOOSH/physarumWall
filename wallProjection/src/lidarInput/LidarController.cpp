#include "LidarController.h"
#include <type_traits>

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
                                                  screen["textureSize"][0].get<int>(),
                                                  screen["textureSize"][1].get<int>());
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

void LidarController::update()
{
    
    values.clear();
    for (auto &sensor : sensors)
    {
        sensor->update();
        auto v = sensor->getFilteredSamples();
        values.insert(values.end(), v.begin(), v.end());
    }

    double maxDistance = 400;
    //cout << values.size() <<endl;
    std::vector<PointCluster> clusters = clusterPointsOptimized(values, maxDistance);

    std::vector<int> touchEnter;
    std::vector<int> touchMove;

    // add touch to list

    std::vector<int> touchLeave;
    map<int, ofTouchEventArgs> touchesNew;

    touchLeave.reserve(touches.size()); // Pre-allocate for efficiency
    for (const auto &pair : touches)
    {
        touchLeave.push_back(pair.first);
    }

    for (auto &cluster : clusters)
    {
        int id = currentId;
        float d = 999999999999;
        int tempId = -1;

        // get closest touch
        for (auto &touch : touches)
        {
            ofVec2f touchP = ofVec2f(touch.second.x, touch.second.y);
            float dT = touchP.distance(cluster.center);
            if (dT <= d)
            {
                d = dT;
                tempId = touch.first;
            }
        }

        // sort touch
        if (d <= maxDistance)
        {
            id = tempId;
            touchMove.push_back(id);
            touchLeave.erase(std::remove(touchLeave.begin(), touchLeave.end(), id), touchLeave.end());

            auto speed = cluster.center - ofVec2f(touches[id].x, touches[id].y);

            ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::move, cluster.center.x, cluster.center.y, id);
            t.xspeed = speed.x;
            t.yspeed = speed.y;

            touchesNew.insert(make_pair(id, t));
        }
        else
        {
            touchEnter.push_back(id);
            currentId++;

            ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::down, cluster.center.x, cluster.center.y, id);
            touchesNew.insert(make_pair(id, t));
        }
    }
    for (auto &e : touchLeave)
        {
           // cout << "leave " << e <<endl;
            auto pos = touches[e].type = ofTouchEventArgs::up;
            ofTouchEventArgs t = ofTouchEventArgs(touches[e]);
            //mapTouchToTexCoords(t);
            interactionEnd.notify(t);
        }

        for (auto &e : touchEnter)
        {
          //  cout << "enter " << e <<endl;
            ofTouchEventArgs t = ofTouchEventArgs(touchesNew[e]);
           // mapTouchToTexCoords(t);
            interactionStart.notify(t);
        }

        for (auto &e : touchMove)
        {
            if (ofVec2f(touchesNew[e].xspeed,touchesNew[e].yspeed).length() > 0)
            {
                ofTouchEventArgs t = ofTouchEventArgs(touchesNew[e]);
               // mapTouchToTexCoords(t);
                interactionMove.notify(t);
            }
        }

    touches = touchesNew;
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
     cout << "move " << ev.x << "  " << ev.y << endl;
    mapTouchToTexCoords(ev);
    interactionMove.notify(ev);
    // updateTexture();
}

void LidarController::mapTouchToTexCoords(ofTouchEventArgs &ev)
{
    ofVec2f pos = ofVec2f(ev.x, ev.y);
    for (auto &m : screenMaps)
    {
        if (m.wallDim.inside(pos))
        {
            ev.x = ofMap(ev.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
            ev.y = ofMap(ev.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);
        }
    }
}

std::vector<PointCluster> LidarController::clusterPointsOptimized(std::vector<ofVec2f> points, double maxDistance)
{
    std::vector<PointCluster> clusters;
    std::vector<bool> assigned(points.size(), false);

    // Sort points by x coordinate for better initial assignments
    std::sort(points.begin(), points.end(), [](const ofVec2f &a, const ofVec2f &b)
              { return a.x < b.x; });

    while (true)
    {
        // Find the first unassigned point
        int firstUnassigned = -1;
        for (size_t i = 0; i < assigned.size(); ++i)
        {
            if (!assigned[i])
            {
                firstUnassigned = i;
                break;
            }
        }

        if (firstUnassigned == -1)
            break; // All points assigned

        // Try to find the best center for a new cluster
        ofVec2f bestCenter = points[firstUnassigned];
        int maxCovered = 1;

        // Try each unassigned point as a potential center
        for (size_t i = 0; i < points.size(); ++i)
        {
            if (assigned[i])
                continue;

            ofVec2f candidateCenter = points[i];
            int covered = 0;

            for (size_t j = 0; j < points.size(); ++j)
            {
                if (!assigned[j] && points[j].distance(candidateCenter) <= maxDistance)
                {
                    covered++;
                }
            }

            if (covered > maxCovered)
            {
                maxCovered = covered;
                bestCenter = candidateCenter;
            }
        }

        // Create new cluster with best center
        PointCluster newCluster(bestCenter);
        assigned[firstUnassigned] = true;

        // Add all points within maxDistance
        for (size_t i = 0; i < points.size(); ++i)
        {
            if (!assigned[i] && points[i].distance(bestCenter) <= maxDistance)
            {
                newCluster.addPoint(points[i]);
                assigned[i] = true;
            }
        }

        // Calculate optimal center
        newCluster.recalculateCenter();

        // Verify all points are within maxDistance of the new center
        std::vector<ofVec2f> validPoints;
        for (const auto &p : newCluster.points)
        {
            if (p.distance(newCluster.center) <= maxDistance)
            {
                validPoints.push_back(p);
            }
            else
            {
                // If point is no longer valid, unmark it as assigned
                size_t idx = &p - &newCluster.points[0];
                assigned[idx] = false;
            }
        }

        // Update cluster with only valid points
        newCluster.points = validPoints;

        // Only add non-empty clusters
        if (!newCluster.points.empty() && newCluster.getMaxRadius() > 10)
        {
            clusters.push_back(newCluster);
        }
    }

    return clusters;
}

void LidarController::updateTexture()
{
    debugFbo.begin();
    ofClear(0, 0);
    
    int nSensor = 0;
    // draw environment map
    for (auto &sensor : sensors)
    {
        switch (nSensor%3)
        {
        case 0:
            ofSetColor(46,134,73);
            break;
        case 1:
            ofSetColor(134,125,45);
            break;
        case 2:
            ofSetColor(134,57,45);
            break;
        default:
            break;
        }
        auto samples = sensor->getEnvironment();
        for (size_t i = 0; i < samples.size(); i++)
        {

            float angle0;
            if (sensor->isMirror)
            {
                angle0 = TWO_PI - (samples[i].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            else
            {
                angle0 = (samples[i].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            float angle1;
            if (sensor->isMirror)
            {
                angle1 = TWO_PI - (samples[(i + 1) % samples.size()].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            else
            {
                angle1 = (samples[(i + 1) % samples.size()].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }

            auto m = screenMaps[0];
            auto pos0 = ofVec2f(round(samples[i].dist_mm_q2 * DIST_TO_MM * cos(angle0)), round(samples[i].dist_mm_q2 * DIST_TO_MM * sin(angle0))) + sensor->position;
            pos0.x = ofMap(pos0.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
            pos0.y = ofMap(pos0.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);

            auto pos1 = ofVec2f(round(samples[(i + 1) % samples.size()].dist_mm_q2 * DIST_TO_MM * cos(angle1)), round(samples[(i + 1) % samples.size()].dist_mm_q2 * DIST_TO_MM * sin(angle1))) + sensor->position;
            pos1.x = ofMap(pos1.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
            pos1.y = ofMap(pos1.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);

            ofDrawLine(pos0.x, pos0.y, pos1.x, pos1.y);
        }
        nSensor++;
    }

    nSensor = 0;
    // draw environment map
    for (auto &sensor : sensors)
    {
        switch (nSensor%3)
        {
        case 0:
            ofSetColor(2,219,68);
            break;
        case 1:
            ofSetColor(219,196,0);
            break;
        case 2:
            ofSetColor(219,29,0);
            break;
        default:
            break;
        }
        auto samples = sensor->getSamples();
        for (size_t i = 0; i < samples.size(); i++)
        {

            float angle0;
            if (sensor->isMirror)
            {
                angle0 = TWO_PI - (samples[i].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            else
            {
                angle0 = (samples[i].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            float angle1;
            if (sensor->isMirror)
            {
                angle1 = TWO_PI - (samples[(i + 1) % samples.size()].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }
            else
            {
                angle1 = (samples[(i + 1) % samples.size()].angle_z_q14 * TWO_PI / 65536) + (sensor->rotation * PI / 180);
            }

            auto m = screenMaps[0];
            auto pos0 = ofVec2f(round(samples[i].dist_mm_q2 * DIST_TO_MM * cos(angle0)), round(samples[i].dist_mm_q2 * DIST_TO_MM * sin(angle0))) + sensor->position;
            pos0.x = ofMap(pos0.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
            pos0.y = ofMap(pos0.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);

            auto pos1 = ofVec2f(round(samples[(i + 1) % samples.size()].dist_mm_q2 * DIST_TO_MM * cos(angle1)), round(samples[(i + 1) % samples.size()].dist_mm_q2 * DIST_TO_MM * sin(angle1))) + sensor->position;
            pos1.x = ofMap(pos1.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
            pos1.y = ofMap(pos1.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);

            ofDrawLine(pos0.x, pos0.y, pos1.x, pos1.y);
        }
        nSensor++;
    }

    for(auto& v:values){
        for (auto &m : screenMaps)
        {
            if (m.wallDim.inside(v))
            {
                float x = ofMap(v.x, m.wallDim.x, m.wallDim.x + m.wallDim.width, m.screenDim.x, m.screenDim.x + m.screenDim.width);
                float y = ofMap(v.y, m.wallDim.y, m.wallDim.y + m.wallDim.height, m.screenDim.y, m.screenDim.y + m.screenDim.height);
                ofDrawRectangle(x,y,2,2);
            }
        }
    }

    ofDrawBitmapString(ofToString(ofGetFrameRate()),10,10);

    // draw touches
    int ty = 30;
    for (auto &touch : touches)
    {
        auto t = ofTouchEventArgs(touch.second);
        mapTouchToTexCoords(t);

        ofSetColor(255, 0, 0);
        ofDrawCircle(t.x, t.y, 10);
        ofSetColor(255);
        ofDrawBitmapString(ofToString(touch.first), t.x - 4, t.y + 4);

        ofDrawBitmapString(ofToString(touch.first) +" -> " +ofToString(t.x) + ","  +ofToString(t.y),10,ty);
        ty+=14;
    }
    debugFbo.end();
}

void LidarController::onTouchUp(ofTouchEventArgs &ev)
{
    touches.erase(ev.id);
    mapTouchToTexCoords(ev);
    interactionEnd.notify(ev);
     cout << "up " << ev.x << "  " << ev.y << endl;
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
