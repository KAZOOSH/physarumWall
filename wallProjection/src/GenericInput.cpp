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

    for (auto &screen : settings["screens"])
    {
        if (args.x > screen["position"][0].get<int>() && args.x < screen["position"][0].get<int>() + screen["size"][0].get<int>())
        {
            auto id = screen["id"].get<std::string>();
            channel = settings["network"]["messages"]["x"][id]["channel"].get<int>();
            noteMin = settings["network"]["messages"]["x"][id]["note"][0].get<int>();
            noteMax = settings["network"]["messages"]["x"][id]["note"][1].get<int>();
            vMin = screen["position"][0].get<int>();
            vMax = screen["position"][0].get<int>() + screen["size"][0].get<int>();
        }
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