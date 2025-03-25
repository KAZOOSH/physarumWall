#pragma once



#include "ofMain.h"
#include "ofxNetwork.h"
#include "Ms200Receiver.h"


#define N_TRACKING_POINTS 448
#define MS200_MAX_DIST 9000

typedef struct point_data_st
{
    unsigned short distance;
    unsigned short intensity;
    float angle;
} point_data_t;




class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);		

		void onTouchEvent(ofTouchEventArgs& ev);

		std::vector<point_data_t> convertCharArrayToPointVector(const char* buffer, size_t bufferSize);
		std::vector<LidarRawSample> convertCharArrayToLidarSamples(const char* buffer, size_t bufferSize);
		void updateValues(std::map<int,LidarRawSample>& db, std::vector<LidarRawSample> newSamples);

		ofxUDPManager udpConnection;
		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;

		vector<glm::vec3> stroke;

		vector<uint16_t> values;
		std::vector<LidarRawSample> points;
		int indexT = 0;

		std::map<int,LidarRawSample> samples;

		vector<u_int16_t> angles;


		shared_ptr<Ms200Receiver> receiver;
};

