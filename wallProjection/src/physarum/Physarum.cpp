// by Etienne Jacob, see license and crediting in main.cpp

#include "Physarum.h"

//--------------------------------------------------------------
void Physarum::setup(ofJson settings){
    TextureCreation::setup(settings);

    simulationWidth = settings["textureDim"][0];
    simulationHeight = settings["textureDim"][1];

    // number of particles
    nParticles = simulationWidth*simulationHeight*6.15*settings["physarum"]["particleMultiplicator"].get<float>();

    hWall = 0;
    wWall = 0;

    for (auto& w:settings["screens"])
    {
        if(hWall < w["worldDimensions"]["height"].get<int>()){
            hWall = w["worldDimensions"]["height"].get<int>();
        }
        wWall += w["worldDimensions"]["width"].get<int>();
    }
    //cout << "wh " << wWall << "  " << hWall << endl;

    ofSetFrameRate(FRAME_RATE);

    ofEnableAntiAliasing();

    u = float(ofGetHeight())/1080;

    myFont.load("fonts/Raleway-Regular.ttf",floor(22.0 * u));
    myFontBold.load("fonts/Raleway-Bold.ttf",floor(22.0 * u));

    counter.resize(simulationWidth*simulationHeight);
    counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

    trailReadBuffer.allocate(simulationWidth, simulationHeight, GL_RG16F);
    trailWriteBuffer.allocate(simulationWidth, simulationHeight, GL_RG16F);
    fboDisplay.allocate(simulationWidth, simulationHeight, GL_RGBA8);

    setterShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_setter.glsl");
    setterShader.linkProgram();

    depositShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_deposit.glsl");
    depositShader.linkProgram();

    moveShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_move.glsl");
    moveShader.linkProgram();

    blurShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_blur.glsl");
    blurShader.linkProgram();

    particles.resize(nParticles * PARTICLE_PARAMETERS_COUNT);
    float marginx = 3;
    float marginy = 3;

    for (int i = 0; i < nParticles; i++) {
        particles[PARTICLE_PARAMETERS_COUNT * i + 0] = floatAsUint16(ofRandom(marginx, simulationWidth - marginx) / simulationWidth);
        particles[PARTICLE_PARAMETERS_COUNT * i + 1] = floatAsUint16(ofRandom(marginy, simulationHeight - marginy) / simulationHeight);
        particles[PARTICLE_PARAMETERS_COUNT * i + 2] = floatAsUint16(ofRandom(1));
        particles[PARTICLE_PARAMETERS_COUNT * i + 3] = floatAsUint16(ofRandom(1));
        particles[PARTICLE_PARAMETERS_COUNT * i + 4] = 0;
        particles[PARTICLE_PARAMETERS_COUNT * i + 5] = 0;
    }
    particlesBuffer.allocate(particles,GL_DYNAMIC_DRAW);

    simulationParameters.resize(NUMBER_OF_USED_POINTS);
    simulationParametersBuffer.allocate(simulationParameters,GL_DYNAMIC_DRAW);
    simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

    trailReadBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    fboDisplay.getTexture().bindAsImage(4,GL_WRITE_ONLY);

    for(int i=0;i<MAX_NUMBER_OF_WAVES;i++)
    {
        waveXarray[i] = simulationWidth/2;
        waveYarray[i] = simulationHeight/2;
        waveTriggerTimes[i] = -12345;
        waveSavedSigmas[i] = 0.5;
    }

    for(int i=0;i<MAX_NUMBER_OF_RANDOM_SPAWN;i++)
    {
        randomSpawnXarray[i] = simulationWidth/2;
        randomSpawnYarray[i] = simulationHeight/2;
    }


    numberOfGamepads = 0;

    std::cout << "Number of points : " << pointsDataManager.getNumberOfPoints() << std::endl;

    paramsUpdate();


    minTScenarioChange = settings["physarum"]["tminChangeScenario"].get<int>()*1000;
    maxTScenarioChange = settings["physarum"]["tmaxCangeScenario"].get<int>()*1000;
    minActionSize = settings["physarum"]["action"]["size"][0].get<float>();
    maxActionSize = settings["physarum"]["action"]["size"][1].get<float>();


}

void Physarum::paramsUpdate()
{
    pointsDataManager.updateCurrentValuesFromTransitionProgress(currentTransitionProgress());

    for(int k=0;k<NUMBER_OF_USED_POINTS;k++)
    {
        simulationParameters[k] = pointsDataManager.getPointsParamsFromArray(pointsDataManager.currentPointValues[k]);
    }

    simulationParametersBuffer.updateData(simulationParameters);
}

//--------------------------------------------------------------
void Physarum::update(){
    float time = getTime();

    paramsUpdate();
    updateActionAreaSizeSigma();

    if((getTime() - latestPointSettingsActionTime) >= SETTINGS_DISAPPEAR_DURATION)
    {
        settingsChangeMode = 0;
    }

    if(numberOfGamepads == 0)
    {
        curL2 = -1; // L2 for no "inertia" effect, when using keyboard only
    }

    setterShader.begin();
    setterShader.setUniform1i("width",trailReadBuffer.getWidth());
    setterShader.setUniform1i("height",trailReadBuffer.getHeight());
    setterShader.setUniform1i("value",0);
    setterShader.dispatchCompute(simulationWidth / 32, simulationHeight / 32, 1);
    setterShader.end();


    trailReadBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);

    moveShader.begin();
    moveShader.setUniform1i("width",trailReadBuffer.getWidth());
    moveShader.setUniform1i("height",trailReadBuffer.getHeight());
    moveShader.setUniform1f("time",time);

    moveShader.setUniform1f("actionAreaSizeSigma",currentActionAreaSizeSigma);

    moveShader.setUniform1fv("actionsX", actionsX.data(), actionsX.size());
    moveShader.setUniform1fv("actionsY", actionsY.data(), actionsY.size());
    moveShader.setUniform1iv("spawn", spawn.data(), spawn.size());

    moveShader.setUniform1f("moveBiasActionX",curMoveBiasActionX);
    moveShader.setUniform1f("moveBiasActionY",curMoveBiasActionY);

    moveShader.setUniform1fv("waveXarray", waveXarray.data(), waveXarray.size());
    moveShader.setUniform1fv("waveYarray", waveYarray.data(), waveYarray.size());
    moveShader.setUniform1fv("waveTriggerTimes", waveTriggerTimes.data(), waveTriggerTimes.size());
    moveShader.setUniform1fv("waveSavedSigmas", waveSavedSigmas.data(), waveSavedSigmas.size());

    moveShader.setUniform1f("mouseXchange",1.0*ofGetMouseX()/ofGetWidth());
    moveShader.setUniform1f("L2Action",ofMap(curL2,-1,1,0,1.0,true));

    moveShader.setUniform1i("spawnParticles", int(particlesSpawn));
    moveShader.setUniform1f("spawnFraction",SPAWN_FRACTION);
    moveShader.setUniform1i("randomSpawnNumber",randomSpawnNumber);
    moveShader.setUniform1fv("randomSpawnXarray", randomSpawnXarray.data(), randomSpawnXarray.size());
    moveShader.setUniform1fv("randomSpawnYarray", randomSpawnYarray.data(), randomSpawnYarray.size());

    moveShader.setUniform1f("pixelScaleFactor",PIXEL_SCALE_FACTOR);

    moveShader.dispatchCompute(particles.size() / (128 * PARTICLE_PARAMETERS_COUNT), 1, 1);
    moveShader.end();



    depositShader.begin();
    depositShader.setUniform1i("width",trailReadBuffer.getWidth());
    depositShader.setUniform1i("height",trailReadBuffer.getHeight());
    depositShader.setUniform1f("depositFactor",DEPOSIT_FACTOR);
    depositShader.setUniform1i("colorModeType",colorModeType);
    depositShader.setUniform1i("numberOfColorModes",NUMBER_OF_COLOR_MODES);
    depositShader.dispatchCompute(simulationWidth / 32, simulationHeight / 32, 1);
    depositShader.end();

    trailReadBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);

    blurShader.begin();
    blurShader.setUniform1i("width",trailReadBuffer.getWidth());
    blurShader.setUniform1i("height",trailReadBuffer.getHeight());
    blurShader.setUniform1f("PI",PI);
    blurShader.setUniform1f("decayFactor",DECAY_FACTOR);
    blurShader.setUniform1f("time",time);
    blurShader.dispatchCompute(trailReadBuffer.getWidth()/32,trailReadBuffer.getHeight()/32,1);
    blurShader.end();


    spawn = {};
    if(particlesSpawn) particlesSpawn = 0;
    /*for (size_t i = 0; i < MAX_INPUT; i++)
    {
        spawn[i] = 0;
    }*/
    

    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    //ofSetWindowTitle(strm.str());


    if(ofGetElapsedTimeMillis() > nextChangeScenario){
        changeScenario();
    }

    fbo.begin();
    ofPushMatrix();
    fboDisplay.draw(0,0);
    ofPopMatrix();
    fbo.end();
}

// DRAW

void Physarum::draw(){

}


// OTHER

void Physarum::updateActionAreaSizeSigma()
{
    float target = ofMap(sigmaCount,0,sigmaCountModulo,minActionSize,maxActionSize,true);
    float lerper = pow(ofMap(getTime() - latestSigmaChangeTime, 0, ACTION_SIGMA_CHANGE_DURATION, 0, 1, true),1.7);
    currentActionAreaSizeSigma = ofLerp(currentActionAreaSizeSigma, target, lerper);
}

void Physarum::setRandomSpawn()
{
    //randomSpawnNumber = floor(ofRandom(MAX_NUMBER_OF_RANDOM_SPAWN/2,MAX_NUMBER_OF_RANDOM_SPAWN));
    randomSpawnNumber = MAX_NUMBER_OF_RANDOM_SPAWN;

    for(int i=0;i<randomSpawnNumber;i++)
    {
        float theta = ofRandom(0,TWO_PI);
        float r = pow(ofRandom(0,1),0.5);
        float x = r*cos(theta);
        float y = r*sin(theta);
        randomSpawnXarray[i] = x;
        randomSpawnYarray[i] = y;
    }
}

float Physarum::getTime()
{
    return 1.0*ofGetFrameNum()/FRAME_RATE;
}

float Physarum::currentTransitionProgress()
{
    return ofMap(getTime() - transitionTriggerTime, 0, TRANSITION_DURATION, 0., 1., true);
}

bool Physarum::activeTransition()
{
    return (getTime() - transitionTriggerTime) <= TRANSITION_DURATION;
}



void Physarum::updateInputs(ofTouchEventArgs& t)
{
    // Create a vector from the map entries
    std::vector<std::pair<int, std::tuple<long, ofTouchEventArgs>>> vec(touches.begin(), touches.end());
    
    // Sort the vector by the long value in descending order (largest first)
    std::sort(vec.begin(), vec.end(), 
              [](const auto& a, const auto& b) {
                  return std::get<0>(a.second) > std::get<0>(b.second);
              });


    int count = 0;
    for(auto& touch:vec){
        if(count < MAX_NUMBER_OF_INPUTS){
            actionsX[count] = get<1>(touch.second).x;
            actionsY[count] = get<1>(touch.second).y;
            if(t.type == ofTouchEventArgs::down && get<1>(touch.second) == t){
                spawn[count] = round(ofRandom(1))+1;
                if(spawn[count] == 2){
                    setRandomSpawn();
                }
            }
            else{
                spawn[count] = 0;
            }
        }
        count++;
    }
    if(count < MAX_NUMBER_OF_INPUTS){
        for (size_t i = count; i < MAX_NUMBER_OF_INPUTS; i++)
        {
            actionsX[i] = 9999999;
            actionsY[i] = 9999999;
            spawn[i] = 0;
        }
    }
    //cout << actionsX[0] << " , " << actionsY[0] << endl;
}

void Physarum::remapTouchPosition(ofTouchEventArgs& t){
    t.x = ofMap(t.x, 0, wWall, 0, simulationWidth, true);
    t.y = ofMap(t.y, 0, hWall, 0, simulationHeight, true);
}


void Physarum::onTouchDown(ofTouchEventArgs &ev)
{
   // coudt << ev.x << " : " << ev.y;

    remapTouchPosition(ev);
<<<<<<< HEAD
    std::tuple<long,ofTouchEventArgs> t {ofGetElapsedTimeMillis(),ev};
=======
    tuple<long,ofTouchEventArgs> t {ofGetElapsedTimeMillis(),ev};
>>>>>>> parent of 96f0499 (live parameter change added via /live)
    touches[ev.id] = t;

   // cout << "   ->   " << ev.x << " : " << ev.y <<endl;
    
    updateInputs(ev);
    
    //curActionX = ofMap(ev.x, 0, ofGetWidth(), 0, simulationWidth, true);
    //curActionY = ofMap(ev.y, 0, ofGetHeight(), 0, simulationHeight, true);

    //actionTriggerWave();
    actionSpawnParticles(1);
}
void Physarum::onTouchUp(ofTouchEventArgs &ev)
{
    remapTouchPosition(ev);
    updateInputs(ev);

    touches.erase(ev.id);
   /* for (auto& touch:touches)
    {
        cout << touch.first << "  " << get<0>(touch.second) <<endl;
    }
    cout << endl;*/
    
    
}

void Physarum::onTouchMove(ofTouchEventArgs &ev)
{
   // cout << ev.x << " , " <<ev.y <<endl;
    remapTouchPosition(ev);
<<<<<<< HEAD
    std::tuple<long,ofTouchEventArgs> t {ofGetElapsedTimeMillis(),ev};
=======
    tuple<long,ofTouchEventArgs> t {ofGetElapsedTimeMillis(),ev};
>>>>>>> parent of 96f0499 (live parameter change added via /live)
    touches[ev.id] = t;



    updateInputs(ev);
}

void Physarum::onOscMessage(ofxOscMessage m)
{
		// check for mouse moved message
		if(m.getAddress() == "/physarum/nextParam"){
            sendChangeScenario();
            actionChangeParams(1);
		}
        else if(m.getAddress() == "/physarum/lastParam"){
            actionChangeParams(-1);
            sendChangeScenario();
		}
        else if(m.getAddress() == "/physarum/selectBg"){
            pointsDataManager.currentSelectionIndex = 0;
		}
        else if(m.getAddress() == "/physarum/selectFg"){
            pointsDataManager.currentSelectionIndex = 1;
		}
        else if(m.getAddress() == "/physarum/nextBg"){
            pointsDataManager.currentSelectionIndex = 0;
            actionChangeParams(1);
            sendChangeScenario();
		}
        else if(m.getAddress() == "/physarum/lastBg"){
            pointsDataManager.currentSelectionIndex = 0;
            actionChangeParams(-1);
            sendChangeScenario();
		}
        else if(m.getAddress() == "/physarum/nextFg"){
            pointsDataManager.currentSelectionIndex = 1;
            actionChangeParams(1);
            sendChangeScenario();
		}
        else if(m.getAddress() == "/physarum/lastFg"){
            pointsDataManager.currentSelectionIndex = 1;
            actionChangeParams(-1);
            sendChangeScenario();
		}
        else if(m.getAddress() == "/physarum/nextColor"){
            actionChangeColorMode();
		}
        else if(m.getAddress() == "/physarum/setMinTimeScenarioChange"){
            minTScenarioChange = m.getArgAsInt(0)*1000;
		}
        else if(m.getAddress() == "/physarum/setMaxTimeScenarioChange"){
            maxTScenarioChange = m.getArgAsInt(0)*1000;
            //cout << maxTScenarioChange <<endl;
		}else if(m.getAddress() == "/physarum/changeValue"){
            int index = m.getArgAsInt32(0);
            int value = m.getArgAsInt32(1);
		}
}

void Physarum::changeScenario()
{
    int nextAction = ofRandom(10);
    // nextbg
    if(nextAction < 4){
        pointsDataManager.currentSelectionIndex = 0;
        actionChangeParams(1);
        sendChangeScenario();
<<<<<<< HEAD

    }
=======
    } 
>>>>>>> parent of 96f0499 (live parameter change added via /live)
    // lastFG
    else if(nextAction <8){
        pointsDataManager.currentSelectionIndex = 1;
            actionChangeParams(-1);
            sendChangeScenario();
    }
    // next color
    else{
        actionChangeColorMode();
    }
    nextChangeScenario = ofGetElapsedTimeMillis() + ofRandom(minTScenarioChange,maxTScenarioChange);
}

void Physarum::sendChangeScenario()
<<<<<<< HEAD
{

    // midi messages
    ofxOscMessage m;
    m.setAddress("/midi/cc");
    m.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()] + 1);
    m.addIntArg(127);
    m.addIntArg(3);
    ofNotifyEvent(newOscEvent, m, this);

    ofxOscMessage m2;
    m2.setAddress("/midi/cc");
    m2.addIntArg(0);
    m2.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
    m2.addIntArg(0);
    ofNotifyEvent(newOscEvent, m2, this);

    ofxOscMessage m3;
    m3.setAddress("/midi/cc");
    m3.addIntArg(0);
    m3.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
    m3.addIntArg(1);
    ofNotifyEvent(newOscEvent, m3, this);

    ofxOscMessage m4;
    m4.setAddress("/midi/cc");
    m4.addIntArg(0);
    m4.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
    m4.addIntArg(2);
    ofNotifyEvent(newOscEvent, m4, this);

    // webcontrol message
    for (size_t i = 0; i < parameterNames.size(); i++)
    {
        ofxOscMessage m5;
        m5.setAddress("/valueUpdate");
        m5.addStringArg(parameterNames[i]);
        m5.addIntArg(pointsDataManager.getValue(i));
        ofNotifyEvent(newOscEvent, m5, this);
    }

    // sync messages for other installations
    ofxOscMessage s1;
    string mode = "background";
    if(pointsDataManager.currentSelectionIndex == 1){
        mode = "foreground";
    }
    s1.setAddress("/sync/" + mode);
    s1.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.currentSelectionIndex ]);
    ofNotifyEvent(newOscEvent, s1, this);



=======
{            ofxOscMessage m;
            m.setAddress("/midi/cc");
            m.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]+1);
            m.addIntArg(127);
            m.addIntArg(3);
            ofNotifyEvent(newOscMessageEvent,m,this);

            ofxOscMessage m2;
            m2.setAddress("/midi/cc");
            m2.addIntArg(0);
            m2.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
            m2.addIntArg(0);
            ofNotifyEvent(newOscMessageEvent,m2,this);

            ofxOscMessage m3;
            m3.setAddress("/midi/cc");
            m3.addIntArg(0);
            m3.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
            m3.addIntArg(1);
            ofNotifyEvent(newOscMessageEvent,m3,this);

            ofxOscMessage m4;
            m4.setAddress("/midi/cc");
            m4.addIntArg(0);
            m4.addIntArg(pointsDataManager.selectedIndices[pointsDataManager.getSelectionIndex()]);
            m4.addIntArg(2);
            ofNotifyEvent(newOscMessageEvent,m4,this);
        
>>>>>>> parent of 96f0499 (live parameter change added via /live)
}
