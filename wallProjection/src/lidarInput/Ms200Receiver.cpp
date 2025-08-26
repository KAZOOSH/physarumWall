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

    if(settings["lidar"]["minDistEnvironment"] != nullptr){
        minDistEnvironment = settings["lidar"]["minDistEnvironment"].get<int>();
    }

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

void Ms200Receiver::update()
{
    newFrame = false;
    while(filtered.tryReceive(samplesFilteredCartesian)){
        newFrame = true;
    };
    

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
    alpha = fmod(alpha,TWO_PI);
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


void Ms200Receiver::threadedFunction()
{
    std::vector<ofVec2f> samplesFilteredCartesian;
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
            filterNonEnvironmentPoints(samples,environment,samplesFilteredCartesian);
            filtered.send(std::move(samplesFilteredCartesian));
        }

        this_thread::sleep_for(10ms);
    }
}

void Ms200Receiver::filterNonEnvironmentPoints(const std::map<int, LidarRawSample> &samples, std::map<int, LidarRawSample> &environment,std::vector<ofVec2f> &samplesFilteredCartesian)
{
    samplesFilteredCartesian.clear();
     for (auto &sample : samples)
     {
         if (environment.at(sample.first).dist_mm_q2 > sample.second.dist_mm_q2 &&
             environment.at(sample.first).dist_mm_q2 - sample.second.dist_mm_q2 > minDistEnvironment*10)
         {
            bool isOnWall = false;
            ofVec2f pos = calculatePointOnWall(isOnWall, sample.second.angle_z_q14,sample.second.dist_mm_q2);
             if(isOnWall){
                samplesFilteredCartesian.push_back(pos);
             } 
         }
     }
}
