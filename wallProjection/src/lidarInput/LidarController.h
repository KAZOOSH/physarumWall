#ifndef LidarController_H
#define LidarController_H

#pragma once
#include "ofMain.h"
#include "GenericInput.h"
#include "Ms200Receiver.h"


class ScreenMap{
    public:
        ofRectangle wallDim;
        ofRectangle screenDim;
};

struct PointCluster {
    ofVec2f center;
    std::vector<ofVec2f> points;
    
    // Constructor
    PointCluster(const ofVec2f& center) : center(center) {
        points.push_back(center);
    }
    
    // Add a point to the cluster
    void addPoint(const ofVec2f& p) {
        points.push_back(p);
    }
    
    // Calculate the optimal center (mean of all points)
    void recalculateCenter() {
        if (points.empty()) return;
        
        double sumX = 0, sumY = 0;
        for (const auto& p : points) {
            sumX += p.x;
            sumY += p.y;
        }
        
        center.x = sumX / points.size();
        center.y = sumY / points.size();
    }
    
    // Get the maximum distance from center to any point
    double getMaxRadius() const {
        double maxDist = 0;
        for (const auto& p : points) {
            double dist = center.distance(p);
            maxDist = std::max(maxDist, dist);
        }
        return maxDist;
    }
};

class LidarController : public GenericInput
{
public:
    LidarController();
    ~LidarController();

    void setup(ofJson settings) override;
    void update() override;

    void registerInputs(shared_ptr<GenericInput> input);

    void onTouchDown(ofTouchEventArgs& ev);
    void onTouchUp(ofTouchEventArgs& ev);
    void onTouchMove(ofTouchEventArgs& ev);

    void mapTouchToTexCoords(ofTouchEventArgs& ev);
    std::vector<PointCluster> clusterPointsOptimized(std::vector<ofVec2f> points, double maxDistance);

    void updateTexture();

private:
    vector<shared_ptr<Ms200Receiver>> sensors;

    map<int,ofTouchEventArgs> touches;
    vector<ScreenMap> screenMaps;

    int maxDistClustersTracking = 1000; // in mm
    int currentId = 0;

    vector<ofVec2f> values;
    

    
};

#endif