#ifndef Ms200ReceiverSimple_H
#define Ms200ReceiverSimple_H

#pragma once

#include <atomic>
#include <set>
#include <queue>
#include "ofMathConstants.h"
#include "ofxNetwork.h"
#include "ofUtils.h"
#include "GenericInput.h"
#include "ofEvent.h"
#include "ofEvents.h"
#include "ofVec2f.h"
#include "ofRectangle.h"

#define N_TRACKING_POINTS 448
#define MS200_MAX_DIST 9000
#define SCAN_TIME_MS 5000
#define MIN_DIST_ENV 100
#define DIST_TO_MM 0.25

class LidarRawSample
{
public:
    uint16_t angle_z_q14;
    uint32_t dist_mm_q2;
    int8_t quality;
};

class Cluster
{
public:
    float radius;
    u_int64_t meanAngle;
    u_int64_t meanDist;
    std::map<int, LidarRawSample> samples;
};

class TrackingPoint
{
public:
    u_int64_t oldId;
    u_int64_t newId;
    float dist;
};

class Ms200ReceiverSimple : public ofThread, public GenericInput
{
public:
    Ms200ReceiverSimple();
    ~Ms200ReceiverSimple();

    void setup(ofJson settings) override;
    void update();

    /// Start the thread.
    void start()
    {
        startThread();
    }

    void stop()
    {
        std::unique_lock<std::mutex> lck(mutex);
        stopThread();
        // condition.notify_all();
    }

    const std::map<int, LidarRawSample> &getSamples() const
    {
        return samples;
    }

    const std::map<int, LidarRawSample> &getEnvironment() const
    {
        return environment;
    }

    const std::map<u_int64_t, Cluster> &getClusters() const
    {
        return clusters;
    }

    const std::vector<ofVec2f> &getFilteredSamples() const
    {
        return samplesFilteredCartesian;
    }

    ofVec2f polarToCartesian(uint16_t angle, uint32_t distance);
    ofVec2f calculatePointOnWall(bool& isOnWall, uint16_t angle, uint32_t distance);

    int maxDistPoints = 300;            // in mm
    int maxDistClustersTracking = 1000; // in mm

    ofRectangle wallDimensions;
    ofVec2f position;
    float rotation;
    bool isMirror;
    

private:
    std::vector<LidarRawSample> convertCharArrayToLidarSamples(const char *buffer, size_t bufferSize);
    void updateValues(std::map<int, LidarRawSample> &db, std::vector<LidarRawSample> newSamples);
    void updateEnvironment(const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment);
    void updateClusters(std::map<u_int64_t, Cluster> &clusters, const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment);
    std::vector<std::vector<int>> createClusters(const std::map<int, int> &cpairs);

    void filterNonEnvironmentPoints(const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment,std::vector<ofVec2f> &samplesFilteredCartesian);

    void threadedFunction();

    std::vector<ofVec2f> samplesFilteredCartesian;
    ofThreadChannel<std::vector<ofVec2f>> toFilter;
    ofThreadChannel<std::vector<ofVec2f>> filtered;
    std::map<int, LidarRawSample> samples;
    std::map<int, LidarRawSample> environment;
    std::map<u_int64_t, Cluster> clusters;
    ofxUDPManager udpReceiver;
    u_int64_t currentClusterId = 0;
    u_int64_t minClusterId = 0;
    u_int64_t maxClusterId = 65573;

    //
    u_int64_t tScanStarted = 0;
    bool isScanningEnvironment = false;
    bool newFrame;

    
};

#endif