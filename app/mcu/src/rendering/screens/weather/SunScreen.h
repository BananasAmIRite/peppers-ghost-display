#pragma once

#include "utils/color_utils.h"
#include "utils/sd_utils.h"
#include "rendering/screens/SpriteScreen.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>



// haha get it? sunscreen
class SunScreen : public SpriteScreen {
    private:
        uint8_t curSunFrame = 0; 
        uint32_t renderStart = 0; 

        uint16_t frameTime; 
        uint16_t moonFrameTime; 
        bool* isNight; 

    public: 
        SunScreen(bool* nightPtr, uint16_t sunFrameTime = 350, uint16_t moonFrameTime = 1000) : SpriteScreen({
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


        void render(Adafruit_GFX* tft) override {


            SpriteScreen::render(tft); 

            if (*isNight) {
                curSunFrame = 2 + ((millis() - renderStart) / moonFrameTime) % 5; 
                drawSprite(tft, curSunFrame, 15, 15, 4); 

            } else {
                curSunFrame = ((millis() - renderStart) / frameTime) % 2; 
                
                drawSprite(tft, curSunFrame, 0, 0, 4); 

            }


        }
};