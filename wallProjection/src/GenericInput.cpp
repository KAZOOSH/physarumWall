#include "GenericInput.h"

GenericInput::GenericInput()
{
}

GenericInput::~GenericInput()
{
}

void GenericInput::setup(ofJson settings_)
{
    settings = settings_;
    debugFbo.allocate(settings["textureDim"][0], settings["textureDim"][1]);

    ofAddListener(interactionStart, this, &GenericInput::onTouchOscTouch);
    ofAddListener(interactionMove, this, &GenericInput::onTouchOscTouch);
    ofAddListener(interactionEnd, this, &GenericInput::onTouchOscTouch);
}

/**
 * Osc messages to control ableton
 */
void GenericInput::onTouchOscTouch(ofTouchEventArgs &args)
{

    // get active screen
    int channel = 0;
    int noteMin = 0;
    int noteMax = 0;
    int vMin = 0;
    int vMax = 0;

    /* creating the midi notes
    *  if 3 screens then map each soundset to a single screen
    *  if not divide screens by 3
    */
   if (settings["screens"].size()==3)
   {
    for (auto &screen : settings["screens"])
    {
        if (args.x > screen["worldDimensions"]["x"].get<int>() && args.x < screen["worldDimensions"]["x"].get<int>() + screen["worldDimensions"]["width"].get<int>())
        {
            auto id = screen["id"].get<std::string>();
            channel = settings["network"]["messages"]["x"][id]["channel"].get<int>();
            noteMin = settings["network"]["messages"]["x"][id]["note"][0].get<int>();
            noteMax = settings["network"]["messages"]["x"][id]["note"][1].get<int>();
            vMin = screen["worldDimensions"]["x"].get<int>();
            vMax = screen["worldDimensions"]["x"].get<int>() + screen["worldDimensions"]["width"].get<int>();
        }
    }
   }else{
        int wMax = settings["screens"].back()["worldDimensions"]["x"].get<int>()+settings["screens"].back()["worldDimensions"]["width"].get<int>();
        std::string id = "left";
        vMin = 0;
        vMax = wMax*0.33333;
        if (args.x > wMax*0.66){
            id = "right";
            vMin = wMax*0.66;
            vMax = wMax;
        }else if (args.x > wMax*0.33){
            id = "center";
            vMin = wMax*0.33;
            vMax = wMax*0.66;
        }     
        channel = settings["network"]["messages"]["x"][id]["channel"].get<int>();
        noteMin = settings["network"]["messages"]["x"][id]["note"][0].get<int>();
        noteMax = settings["network"]["messages"]["x"][id]["note"][1].get<int>();
   }


    // x-value
    ofxOscMessage m;
    if (args.type == ofTouchEventArgs::down || args.type == ofTouchEventArgs::move)
    {
        m.setAddress("/midi/note_on");
    }
    else if (args.type == ofTouchEventArgs::up)
    {
        m.setAddress("/midi/note_off");
    }
    m.addIntArg(args.id);
    m.addIntArg(ofMap(args.x, vMin, vMax, noteMin, noteMax, true));
    m.addIntArg(100);
    m.addIntArg(channel);

    ofNotifyEvent(newOscMessageEvent,m,this);

    // y-value
    ofxOscMessage m2;
    m2.setAddress("/midi/cc");
    m2.addIntArg(1);
    m2.addIntArg(ofMap(args.y, 0, debugFbo.getHeight(), 0, 127, true));
    m2.addIntArg(channel);

    ofNotifyEvent(newOscMessageEvent,m2,this);
}