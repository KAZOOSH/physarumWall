#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	// we run at 60 fps!
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofEnableAntiAliasing();

	// create the socket and bind to port 11999
	// ofxUDPSettings settings;
	// settings.receiveOn(40001);
	// settings.blocking = false;

	// udpConnection.Setup(settings);

	ofSetBackgroundAuto(false);
	ofSetBackgroundColor(255);

	values.resize(6000);

	receiver = shared_ptr<Ms200Receiver>(new Ms200Receiver(40013, "env.txt"));

	ofAddListener(receiver->interactionStart,this,&ofApp::onTouchEvent);
	ofAddListener(receiver->interactionMove,this,&ofApp::onTouchEvent);
	ofAddListener(receiver->interactionEnd,this,&ofApp::onTouchEvent);
}

//--------------------------------------------------------------
void ofApp::update()
{
	/*
		int lMsg = 4096;
		char udpMessage[lMsg];
		std::memset(udpMessage, -127, sizeof(udpMessage));
		udpConnection.Receive(udpMessage,lMsg);




		std::vector<uint16_t> vals(
			reinterpret_cast<uint16_t*>(udpMessage),
			reinterpret_cast<uint16_t*>(udpMessage + (lMsg - (lMsg % 2)))
		);

		for(size_t i=0; i<50;i++){
			if(i<vals.size()){
				//cout << int(vals[i]) << " ";
			}
		}
		//cout << endl;
		//uint8_t vals[100000];
		//std::memcpy(vals, udpMessage, sizeof(udpMessage));

		auto tPoints = convertCharArrayToLidarSamples(udpMessage, lMsg);
		if(tPoints.size()>0){
			points = tPoints;
			updateValues(samples,tPoints);
			for(size_t i=0; i<15;i++){

				cout << "| " << points[i].angle_z_q14 << " " << points[i].dist_mm_q2 << " " << int(points[i].quality) <<endl;

			}
			cout <<endl;
		}


		int nn=0;
		//cout << "vals-> " << sizeof(vals) << endl;
		for(size_t i=0; i<vals.size();i++){
			if(vals[i]!= 0){
				nn = i;
			}
		}

		if(nn == 0){
			indexT = 0;
		}else{
			for(size_t i=0; i<vals.size();i++){
				if(vals[i]!= 0){
					values[indexT] = vals[i];
					indexT++;
				}
			}
			if(nn < 500){
				indexT = 0;
			}
		}

		cout << nn <<endl;



		/*string message=udpMessage;
		if(message!=""){
			cout <<message.size() << "  :  " << message <<endl;
			//stroke.clear();
			//float x,y;
			//vector<string> strPoints = ofSplitString(message,"[/p]");
			/*for(unsigned int i=0;i<strPoints.size();i++){
				vector<string> point = ofSplitString(strPoints[i],"|");
				if( point.size() == 2 ){
					x=atof(point[0].c_str());
					y=atof(point[1].c_str());
					stroke.push_back(glm::vec3(x,y,0));
				}
			}
	//	}//
	*/
}

//--------------------------------------------------------------
void ofApp::draw()
{
	/*if(points.size() > 0){

		for(unsigned int i=0;i<points.size();i++){
			int v = int(points[i].dist_mm_q2);
			ofDrawLine(i%nValsLine,h*(i/nValsLine),i%nValsLine,h*(i/nValsLine)+ofMap(v,0,256*256,0,h,true));
		}
	}*/

	ofBackground(255);

	ofSetColor(255, 128, 128);

	int maxSize = ofGetHeight() / 4;

	auto samples = receiver->getSamples();
	for (size_t i = 0; i < samples.size(); i++)
	{
		float angle0 = ofMap(samples[i].angle_z_q14, 0, 65536, 0, TWO_PI);
		float angle1 = ofMap(samples[(i + 1) % samples.size()].angle_z_q14, 0, 65536, 0, TWO_PI);

		int v0 = ofMap(samples[i].dist_mm_q2, MS200_MAX_DIST, 0, maxSize, true);
		int v1 = ofMap(samples[(i + 1) % samples.size()].dist_mm_q2, MS200_MAX_DIST, 0, maxSize, true);

		ofDrawLine(
			ofGetWidth() * 0.5 + cos(angle0) * v0, ofGetHeight() * 0.5 + sin(angle0) * v0,
			ofGetWidth() * 0.5 + cos(angle1) * v1, ofGetHeight() * 0.5 + sin(angle1) * v1);
	}

	ofSetColor(20);
	auto env = receiver->getEnvironment();
	for (size_t i = 0; i < env.size(); i++)
	{
		float angle0 = ofMap(env[i].angle_z_q14, 0, 65536, 0, TWO_PI);
		float angle1 = ofMap(env[(i + 1) % env.size()].angle_z_q14, 0, 65536, 0, TWO_PI);

		int v0 = ofMap(env[i].dist_mm_q2, MS200_MAX_DIST, 0, maxSize, true);
		int v1 = ofMap(env[(i + 1) % env.size()].dist_mm_q2, MS200_MAX_DIST, 0, maxSize, true);

		ofDrawLine(
			ofGetWidth() * 0.5 + cos(angle0) * v0, ofGetHeight() * 0.5 + sin(angle0) * v0,
			ofGetWidth() * 0.5 + cos(angle1) * v1, ofGetHeight() * 0.5 + sin(angle1) * v1);
	}

	auto cl = receiver->getClusters();
	ofNoFill();
	for (auto& c:cl)
	{
		float angle0 = ofMap(c.second.meanAngle, 0, 65536, 0, TWO_PI);
		int v0 = ofMap(c.second.meanDist, MS200_MAX_DIST, 0, maxSize, true);
		//ofDrawCircle(ofGetWidth() * 0.5 + cos(angle0) * v0, ofGetHeight() * 0.5 + sin(angle0) * v0, ofMap(c.second.radius, MS200_MAX_DIST, 0, maxSize, true));
		ofDrawBitmapStringHighlight(ofToString(c.first) + " -> " + ofToString(c.second.meanDist/100), ofGetWidth() * 0.5 + cos(angle0) * v0, ofGetHeight() * 0.5 + sin(angle0) * v0);
	}
	
	/*
	for (const auto& sample : receiver->getSamples()) {
		float angle = ofMap(sample.second.angle_z_q14,0,65536,0,TWO_PI);
		int v = ofMap(sample.second.dist_mm_q2,MS200_MAX_DIST,0,ofGetHeight()/2,true);
		ofDrawLine(ofGetWidth()*0.5,ofGetHeight()*0.5,ofGetWidth()*0.5 + cos(angle)*v,ofGetHeight()*0.5 + sin(angle)*v);
	}*/

	/*
		for (const auto& sample : samples) {
			int angle = sample.first;
			int v = sample.second.dist_mm_q2;
			ofDrawLine(0,angle,ofMap(v,0,256*256,0,ofGetWidth(),true),angle);
		}
	*/

	ofDrawBitmapStringHighlight(ofToString(receiver->maxDistPoints), 10, 20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	if(key == OF_KEY_RIGHT){
		receiver->maxDistPoints++;
	}
	if(key == OF_KEY_LEFT){
		receiver->maxDistPoints--;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
}

void ofApp::onTouchEvent(ofTouchEventArgs &ev)
{
	cout << ev.id << " : " <<ev.x << " ; " <<ev.y <<endl;
}

std::vector<point_data_t> ofApp::convertCharArrayToPointVector(const char *buffer, size_t bufferSize)
{
	std::vector<point_data_t> points;

	// Calculate how many complete point_data_t structures can fit in the buffer
	size_t structSize = sizeof(point_data_t);
	size_t numPoints = bufferSize / structSize;

	// Reserve space to avoid reallocations
	points.reserve(numPoints);

	// header to ignore
	size_t headerSize = 24 / structSize;

	// Iterate through the buffer and extract each point
	for (size_t i = headerSize; i < numPoints; i++)
	{
		point_data_t point;
		// Calculate the offset in the buffer for this point
		const char *pointData = buffer + (i * structSize);

		// Copy the bytes from the buffer to the point structure
		std::memcpy(&point, pointData, structSize);

		// Add the point to the vector
		points.push_back(point);
	}

	return points;
}

std::vector<LidarRawSample> ofApp::convertCharArrayToLidarSamples(const char *buffer, size_t bufferSize)
{
	std::vector<LidarRawSample> samples;

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
		std::memcpy(&sample, sampleData, sampleSize);

		// Add the sample to the vector
		samples.push_back(sample);
	}

	return samples;
}

void ofApp::updateValues(std::map<int, LidarRawSample> &db, std::vector<LidarRawSample> newSamples)
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