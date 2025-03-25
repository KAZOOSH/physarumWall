#ifndef MS200RECEIVER_H
#define MS200RECEIVER_H

#pragma once

#include <atomic>
#include <set>
#include <queue>
#include "ofMathConstants.h"
#include "ofxNetwork.h"
#include "ofUtils.h"
#include "ofEvent.h"
#include "ofEvents.h"
#include "ofVec2f.h"

#define N_TRACKING_POINTS 448
#define MS200_MAX_DIST 9000
#define SCAN_TIME_MS 5000
#define MIN_DIST_ENV 100

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

class Ms200Receiver : public ofThread
{
public:
    Ms200Receiver(int port, std::string environmentSettingsFile);
    ~Ms200Receiver();

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

    ofEvent<ofTouchEventArgs> interactionStart;
    ofEvent<ofTouchEventArgs> interactionEnd;
    ofEvent<ofTouchEventArgs> interactionMove;

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

    ofVec2f polarToCartesian(uint16_t angle, uint32_t distance);

    int maxDistPoints = 300;            // in mm
    int maxDistClustersTracking = 1000; // in mm

private:
    std::vector<LidarRawSample> convertCharArrayToLidarSamples(const char *buffer, size_t bufferSize);
    void updateValues(std::map<int, LidarRawSample> &db, std::vector<LidarRawSample> newSamples);
    void updateEnvironment(const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment);
    void updateClusters(std::map<u_int64_t, Cluster> &clusters, const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment);
    std::vector<std::vector<int>> createClusters(const std::map<int, int> &cpairs);

    void threadedFunction();

    std::map<int, LidarRawSample> samples;
    std::map<int, LidarRawSample> environment;
    std::map<u_int64_t, Cluster> clusters;
    ofxUDPManager udpReceiver;
    u_int64_t currentClusterId = 0;

    //
    u_int64_t tScanStarted = 0;
    bool isScanningEnvironment = false;
};

#endif