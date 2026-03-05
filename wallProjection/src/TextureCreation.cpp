#include "TextureCreation.h"

TextureCreation::TextureCreation()
{
}

TextureCreation::~TextureCreation()
{

}

void TextureCreation::setup(ofJson settings){
    outputFbo.allocate(settings["textureDim"][0],settings["textureDim"][1]);
    objectsFbo.allocate(settings["textureDim"][0],settings["textureDim"][1],GL_RG16F);
}

ofTexture& TextureCreation::getTexture()
{
    return outputFbo.getTexture();
}

ofTexture& TextureCreation::getObjectsFbo()
{
    return objectsFbo.getTexture();
}
ofTexture& TextureCreation::getDebugTexture(string id){
	ofLogError("not implemented for that class, showing texture");
	return outputFbo.getTexture();
}


void TextureCreation::registerInputs(shared_ptr<GenericInput> input)
{
    ofAddListener(input->interactionStart,this,&TextureCreation::onTouchDown);
    ofAddListener(input->interactionMove,this,&TextureCreation::onTouchMove);
    ofAddListener(input->interactionEnd,this,&TextureCreation::onTouchUp);
}

void TextureCreation::saveTextureToFile(string filename)
{
    // Create an ofPixels to read the data into
    ofPixels pixels;

    // Allocate pixels with the same size and format
    pixels.allocate(outputFbo.getWidth(), outputFbo.getHeight(), OF_IMAGE_COLOR);
    outputFbo.readToPixels(pixels);

    ofSaveImage(pixels, filename);
}
