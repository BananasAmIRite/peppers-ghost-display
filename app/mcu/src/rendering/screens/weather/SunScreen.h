#pragma once

#include "rendering/screens/SpriteScreen.h"



// haha get it? sunscreen
class SunScreen : public SpriteScreen {
    private:
        uint8_t curSunFrame = 0; 
        uint32_t renderStart = 0; 

        uint16_t frameTime; 
        uint16_t moonFrameTime; 
        bool* isNight; 

    public: 
        SunScreen(bool* nightPtr, uint16_t sunFrameTime = 350, uint16_t moonFrameTime = 1000);

        void render(Adafruit_GFX* tft) override;
};