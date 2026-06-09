#pragma once

#include "../../../../utils/color_utils.h"
#include "../../../../utils/sd_utils.h"
#include "../../SpriteScreen.h"
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

    public: 
        SunScreen(uint16_t sunFrameTime = 350) : SpriteScreen({
            "/sun0.bmp",
            "/sun1.bmp",
        }), frameTime(sunFrameTime) {
            renderStart = millis(); 
        }


        void render(Adafruit_GFX* tft) override {
            SpriteScreen::render(tft); 

            curSunFrame = ((millis() - renderStart) / frameTime) % 2; 

            drawSprite(tft, curSunFrame, 0, 0, 4); 
        }
};