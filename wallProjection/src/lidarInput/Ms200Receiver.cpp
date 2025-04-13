#include "Ms200Receiver.h"

using namespace std::chrono_literals;
using namespace std;

Ms200Receiver::Ms200Receiver()
{
}

Ms200Receiver::~Ms200Receiver()
{
    stop();
    waitForThread(false);
}

void Ms200Receiver::setup(ofJson settings)
{
    position = ofVec2f(settings["lidar"]["position"][0].get<float>(), settings["lidar"]["position"][1].get<float>());
    wallDimensions = ofRectangle(settings["screen"]["worldDimensions"]["x"].get<float>(),
                                 settings["screen"]["worldDimensions"]["y"].get<float>(),
                                 settings["screen"]["worldDimensions"]["width"].get<float>(),
                                 settings["screen"]["worldDimensions"]["height"].get<float>());
    rotation = settings["lidar"]["rotation"].get<float>();
    isMirror = settings["lidar"]["mirror"].get<bool>();

    // load environment
    ofBuffer buffer = ofBufferFromFile(settings["lidar"]["environmentFile"].get<string>().c_str());
    if (buffer.size())
    {
        for (auto &line : buffer.getLines())
        {
            // angle-id,angle,distance,quality
            auto parts = ofSplitString(line, ",");
            if (parts.size() == 4)
            {
                auto angleIndex = ofToInt(parts[0]);
                auto angle = static_cast<uint16_t>(ofToInt(parts[1]));
                auto dist = static_cast<u_int32_t>(ofToInt(parts[2]));
                auto quality = static_cast<int8_t>(ofToInt(parts[3]));
                environment.insert(make_pair(angleIndex,
                                             LidarRawSample{angle, dist, quality}));
            }
        }
    }
    else
    {
        isScanningEnvironment = true;
    }

    ofxUDPSettings usettings;
    usettings.receiveOn(settings["lidar"]["port"].get<int>());
    usettings.blocking = false;

    udpReceiver.Setup(usettings);
    start();
}

void Ms200Receiver::updateValues(map<int, LidarRawSample> &db, vector<LidarRawSample> newSamples)
{
    int div = 65536 / N_TRACKING_POINTS;
    for (auto &sample : newSamples)
    {
        int angle = sample.angle_z_q14 / div;
        if (db.contains(angle))
        {
            db[angle] = sample;
        }
        else
        {
            db.insert(make_pair(angle, sample));
        }
    }
}

ofVec2f Ms200Receiver::polarToCartesian(uint16_t angle, uint32_t distance)
{
    float alpha = angle * TWO_PI / 65536;
    return ofVec2f(round(distance * cos(alpha)), round(distance * sin(alpha)));
}

ofVec2f Ms200Receiver::calculatePointOnWall(bool &isOnWall, uint16_t anglePoint, uint32_t distance)
{
    float alpha;
    if(isMirror){
        alpha = TWO_PI-(anglePoint * TWO_PI / 65536) + (rotation * PI / 180);
    }else{
    alpha = (anglePoint * TWO_PI / 65536) + (rotation * PI / 180);
    } 
    auto pos = ofVec2f(round(distance*DIST_TO_MM * cos(alpha)), round(distance *DIST_TO_MM* sin(alpha))) + position;
    isOnWall = wallDimensions.inside(pos.x, pos.y);
    return pos;
}

vector<LidarRawSample> Ms200Receiver::convertCharArrayToLidarSamples(const char *buffer, size_t bufferSize)
{
    vector<LidarRawSample> samples;

    // Calculate how many complete LidarRawSample objects can fit in the buffer
    size_t sampleSize = sizeof(LidarRawSample);
    size_t numSamples = bufferSize / sampleSize;

    // Reserve space to avoid reallocations
    samples.reserve(numSamples);

    // Iterate through the buffer and extract each sample
    for (size_t i = 1; i < numSamples; i++)
    {
        LidarRawSample sample;
        // Calculate the offset in the buffer for this sample
        const char *sampleData = buffer + (i * sampleSize);

        // Copy the bytes from the buffer to the sample object
        memcpy(&sample, sampleData, sampleSize);

        // Add the sample to the vector
        samples.push_back(sample);
    }

    return samples;
}

void Ms200Receiver::updateEnvironment(const map<int, LidarRawSample> &samples, map<int, LidarRawSample> &environment)
{
    for (auto &sample : samples)
    {
        if (environment.contains(sample.first))
        {
            if (environment.at(sample.first).dist_mm_q2 > sample.second.dist_mm_q2)
            {
                environment[sample.first] = sample.second;
            }
        }
        else
        {
            environment.insert(sample);
        }
    }
}

void Ms200Receiver::updateClusters(map<u_int64_t, Cluster> &clusters, const map<int, LidarRawSample> &samples, map<int, LidarRawSample> &environment)
{
    map<int, LidarRawSample> nonEnvPoints;
    vector<int> indices;
    // check possible points
    for (auto &sample : samples)
    {
        if (environment.at(sample.first).dist_mm_q2 > sample.second.dist_mm_q2 &&
            environment.at(sample.first).dist_mm_q2 - sample.second.dist_mm_q2 > MIN_DIST_ENV)
        {
            nonEnvPoints.insert(sample);
            indices.push_back(sample.first);
        }
    }

    // find pairs
    map<int, int> cPairs;
    int currentId = 0;
    // cluster points
    for (size_t i = 0; i < indices.size(); i++)
    {
        auto r1 = nonEnvPoints[indices[i]].dist_mm_q2;
        float alpha1 = nonEnvPoints[indices[i]].angle_z_q14 * TWO_PI / 65536;
        for (size_t j = i + 1; j < indices.size(); j++)
        {
            auto r2 = nonEnvPoints[indices[j]].dist_mm_q2;
            float alpha2 = nonEnvPoints[indices[j]].angle_z_q14 * TWO_PI / 65536;
            float d = sqrt(r1 * r1 + r2 * r2 - 2 * r1 * r2 * cos(alpha1 - alpha2));
            if (d < maxDistPoints)
            {
                cPairs.insert(pair<int, int>(indices[i], indices[j]));
            }
        }
    }

    // cluster all pairs
    std::vector<std::vector<int>> nCluster = createClusters(cPairs);

    vector<Cluster> tClusters;
    // create final clusters
    for (auto &c : nCluster)
    {
        Cluster cl;

        float radius = 0;
        double alphaX = 0;
        double alphaY = 0;

        u_int64_t meanDist = 0;

        uint64_t minAngle = 12800000;
        uint64_t maxAngle = 0;
        uint64_t minDist = 99999999999;
        uint64_t maxDist = 0;

        int nElems = c.size();
        for (auto &e : c)
        {
            auto angle = nonEnvPoints[e].angle_z_q14;
            auto dist = nonEnvPoints[e].dist_mm_q2;
            cl.samples.insert(make_pair(e, nonEnvPoints[e]));

            double alpha = angle * TWO_PI / 65536;
            alphaX += cos(alpha);
            alphaY += sin(alpha);

            meanDist += (dist / nElems);

            if (angle < minAngle)
            {
                minAngle = angle;
            }
            if (angle > maxAngle)
            {
                maxAngle = angle;
            }
            if (dist < minDist)
            {
                minDist = dist;
            }
            if (dist > maxDist)
            {
                maxDist = dist;
            }
        }

        cl.meanAngle = atan2(alphaY, alphaX) * 65536 / TWO_PI;
        cl.meanDist = meanDist;

        float dl = maxDist - minDist;
        float meanD = minDist + dl * 0.5;
        float alpha1 = minAngle * TWO_PI / 65536;
        float alpha2 = maxAngle * TWO_PI / 65536;
        float dr = sqrt(meanD * meanD + meanD * meanD - 2 * meanD * meanD * cos(alpha1 - alpha2));
        cl.radius = max(dl, dr) * 0.5;

        tClusters.push_back(cl);
    }

    // compare clusters with old clusters for tracking
    vector<TrackingPoint> trackingPoints;
    for (int i = 0; i < tClusters.size(); i++)
    {
        u_int64_t closestId = 0;
        float dist = 9999999;
        auto r1 = tClusters[i].meanDist;
        float alpha1 = tClusters[i].meanAngle * TWO_PI / 65536;
        for (auto &cOld : clusters)
        {

            auto r2 = cOld.second.meanDist;
            float alpha2 = cOld.second.meanAngle * TWO_PI / 65536;
            float d = sqrt(r1 * r1 + r2 * r2 - 2 * r1 * r2 * cos(alpha1 - alpha2));

            // cout << i << "   " << closestId << " : " <<  dist <<endl;
            if (d < maxDistClustersTracking && d < dist)
            {
                closestId = cOld.first;
                dist = d;
            }
        }
        if (dist < maxDistClustersTracking)
        {
            bool isIn = false;
            for (auto &tp : trackingPoints)
            {
                if (tp.oldId == closestId)
                {
                    if (tp.dist > dist)
                    {
                        tp.newId = i;
                        tp.dist = dist;
                    }
                    isIn = true;
                }
            }
            trackingPoints.push_back(TrackingPoint(closestId, i, dist));
        }
    }

    // set ids for cluster and create new map
    std::vector<u_int64_t> oldIds;
    oldIds.reserve(clusters.size());
    for (const auto &pair : clusters)
    {
        oldIds.push_back(pair.first);
    }

    auto clustersOld = clusters;
    clusters.clear();

    for (int i = 0; i < tClusters.size(); i++)
    {
        bool isTracked = false;
        u_int64_t id = currentClusterId;
        for (auto &e : trackingPoints)
        {
            if (e.newId == i)
            {
                id = e.oldId;
                isTracked = true;
            }
        }
        if (!isTracked)
        {
            ++currentClusterId;
        }
        clusters.insert(make_pair(id, tClusters[i]));
    }

    std::vector<u_int64_t> newIds;
    newIds.reserve(clusters.size());
    for (const auto &pair : clusters)
    {
        newIds.push_back(pair.first);
    }

    // create events
    std::sort(oldIds.begin(), oldIds.end());
    std::sort(newIds.begin(), newIds.end());

    // Create output vectors
    std::vector<int> vEnd;
    std::vector<int> vEnter;
    std::vector<int> vMove;

    // Values only in oldIds (set difference)
    std::set_difference(
        oldIds.begin(), oldIds.end(),
        newIds.begin(), newIds.end(),
        std::back_inserter(vEnd));

    // Values only in newIds (set difference)
    std::set_difference(
        newIds.begin(), newIds.end(),
        oldIds.begin(), oldIds.end(),
        std::back_inserter(vEnter));

    // Values in both vectors (set intersection)
    std::set_intersection(
        oldIds.begin(), oldIds.end(),
        newIds.begin(), newIds.end(),
        std::back_inserter(vMove));

    bool isIn = false;
    for (auto &e : vEnd)
    {

        auto pos = calculatePointOnWall(isIn, clustersOld[e].meanAngle, clustersOld[e].meanDist);
        if (isIn)
        {
            ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::up, pos.x, pos.y, e);
            t.width = clusters[e].radius * 2;
            t.height = clusters[e].radius * 2;
            interactionEnd.notify(t);
        }
    }

    for (auto &e : vEnter)
    {
        auto pos = calculatePointOnWall(isIn, clusters[e].meanAngle, clusters[e].meanDist);
        if (isIn)
        {
            ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::down, pos.x, pos.y, e);
            t.width = clusters[e].radius * 2;
            t.height = clusters[e].radius * 2;
            interactionStart.notify(t);
        }
    }

    for (auto &e : vMove)
    {
        bool t = false;
        auto posOld = calculatePointOnWall(t, clustersOld[e].meanAngle, clustersOld[e].meanDist);
        auto pos = calculatePointOnWall(isIn, clusters[e].meanAngle, clusters[e].meanDist);
        if (isIn)
        {
            auto speed = pos - posOld;
            if (speed.length() > 0)
            {
                ofTouchEventArgs t = ofTouchEventArgs(ofTouchEventArgs::move, pos.x, pos.y, e);
                t.xspeed = speed.x;
                t.yspeed = speed.y;
                t.width = clusters[e].radius * 2;
                t.height = clusters[e].radius * 2;
                interactionMove.notify(t);
            }
        }
    }
}

void Ms200Receiver::threadedFunction()
{
    while (isThreadRunning())
    {

        // read message
        int lMsg = 4096;
        char udpMessage[lMsg];
        memset(udpMessage, -127, sizeof(udpMessage));
        udpReceiver.Receive(udpMessage, lMsg);

        auto tPoints = convertCharArrayToLidarSamples(udpMessage, lMsg);
        if (tPoints.size() > 0)
        {
            unique_lock<std::mutex> lock(std::mutex);
            updateValues(samples, tPoints);

            // start scanning timer when first received data
            if (isScanningEnvironment && tScanStarted == 0)
            {
                tScanStarted = ofGetElapsedTimeMillis();
            }
            if (isScanningEnvironment)
            {
                if (ofGetElapsedTimeMillis() - tScanStarted >= SCAN_TIME_MS)
                {

                    isScanningEnvironment = false;
                    cout << "finished scan environment" <<endl;
                }
                else
                {
                    updateEnvironment(samples, environment);
                }
            }

            updateClusters(clusters, samples, environment);
        }

        this_thread::sleep_for(10ms);
    }
}

std::vector<std::vector<int>> Ms200Receiver::createClusters(const std::map<int, int> &cpairs)
{
    vector<set<int>> clusters;

    for (const auto &pair : cpairs)
    {
        bool inCluster = false;
        for (auto &cluster : clusters)
        {
            if (cluster.count(pair.first) > 0)
            {
                inCluster = true;
                cluster.insert(pair.second);
            }
        }
        if (!inCluster)
        {
            clusters.push_back(set<int>());
            clusters.back().insert(pair.first);
            clusters.back().insert(pair.second);
        }
    }

    vector<vector<int>> ret;

    // cout << "clusters   <<<<" <<endl;
    for (size_t i = 0; i < clusters.size(); i++)
    {
        ret.push_back(vector<int>(clusters[i].begin(), clusters[i].end()));
        for (auto &e : clusters[i])
        {
            //    cout << e << " ";
        }
        //  cout << endl;
    }
    // cout << " ----------------" <<endl;

    return ret;
}