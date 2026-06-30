#pragma once

#include "rendering/screens/SpriteScreen.h"
#include <vector>
#include <string>

#define RAIN_SCALE 2

struct RainDroplet {
    float x; 
    float y; 

    uint8_t rainFrame; 

    uint16_t splash_y; 

    uint32_t splash_start; 
};

struct RainBoundingBox {
    int16_t topleft_x; 
    int16_t topleft_y;
    int16_t bottomright_x;
    int16_t bottomright_y;
};

class RainScreen : public SpriteScreen {
    private:
        std::vector<RainDroplet> droplets;
        RainBoundingBox spawnBox; 
        RainBoundingBox splashBox; 
        uint16_t rainSpeed; 
        bool shouldSplash;

        
        void initRandRainDroplet(RainDroplet* droplet, bool start);

    public: 
    // TODO: replace with not magic numbers
        RainScreen(uint16_t rain_speed = 300, uint8_t numDroplets = 16, bool doSplash = false, RainBoundingBox spawn = {0, -56, 540, -16}, RainBoundingBox splash = {0, 230, 480, 280});

        void updateRain();

        void render(Adafruit_GFX* tft) override;
};