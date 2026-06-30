#include "SunScreen.h"



SunScreen::SunScreen(bool* nightPtr, uint16_t sunFrameTime, uint16_t moonFrameTime) : SpriteScreen({
    "/sun0.bmp",
    "/sun1.bmp",
    "/moon0.bmp",
    "/moon1.bmp",
    "/moon2.bmp",
    "/moon3.bmp",
    "/moon4.bmp",
}), frameTime(sunFrameTime), moonFrameTime(moonFrameTime), isNight(nightPtr) {
    renderStart = millis(); 
}


void SunScreen::render(Adafruit_GFX* tft) {


    SpriteScreen::render(tft); 

    if (*isNight) {
        curSunFrame = 2 + ((millis() - renderStart) / moonFrameTime) % 5; 
        drawSprite(tft, curSunFrame, 80+15, 15, 4); 

    } else {
        curSunFrame = ((millis() - renderStart) / frameTime) % 2; 
        
        drawSprite(tft, curSunFrame, 80, 0, 4); 

    }


}