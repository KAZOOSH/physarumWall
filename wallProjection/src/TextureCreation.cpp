#include "TextureCreation.h"

TextureCreation::TextureCreation()
{
    
}

TextureCreation::~TextureCreation()
{

}

void TextureCreation::setup(ofJson settings){
    fbo.allocate(1920*3,1080);
}

ofTexture TextureCreation::getTexture()
{
    return fbo.getTexture();
}


void TextureCreation::registerInputs(shared_ptr<GenericInput> input)
{
    ofAddListener(input->interactionStart,this,&TextureCreation::onTouchDown);
    ofAddListener(input->interactionMove,this,&TextureCreation::onTouchMove);
    ofAddListener(input->interactionEnd,this,&TextureCreation::onTouchUp);
}
