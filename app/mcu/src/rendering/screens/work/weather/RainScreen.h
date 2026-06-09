#pragma once

#include "../../../../utils/color_utils.h"
#include "../../../../utils/sd_utils.h"
#include "../../SpriteScreen.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

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

        
        void initRandRainDroplet(RainDroplet* droplet, bool start) {
            int16_t randX; 
            int16_t randY; 
            if (start) {
                randX = random(spawnBox.topleft_x, spawnBox.bottomright_x); 
                randY = random(spawnBox.topleft_y, splashBox.bottomright_y); 
            } else {
                randX = random(spawnBox.topleft_x, spawnBox.bottomright_x); 
                randY = random(spawnBox.topleft_y, spawnBox.bottomright_y);
            }
            
            droplet->x = randX; 
            droplet->y = randY; 
            droplet->rainFrame = 0; 
            droplet->splash_y = random(splashBox.topleft_y, splashBox.bottomright_y); 
            droplet->splash_start = 0; 
        }

    public: 
        RainScreen(uint16_t rain_speed = 200, uint8_t numDroplets = 16, bool doSplash = false, RainBoundingBox spawn = {0, -56, 360, -16}, RainBoundingBox splash = {0, 160, 320, 200}) : SpriteScreen({
            "/rain0.bmp",
            "/rain1.bmp",
            "/rain2.bmp",
            "/rain3.bmp",
            "/rainc1.bmp",
            "/rainc2.bmp",
            "/rainc3.bmp",
            "/rainc4.bmp",
            "/rainc5.bmp",
        }), spawnBox(spawn), splashBox(splash), droplets(numDroplets), rainSpeed(rain_speed), shouldSplash(doSplash) {
            // spawn in rain
            for (int i = 0; i < numDroplets; i++) {
                initRandRainDroplet(&droplets[i], true); 
            }
        }

        void updateRain() {

            // compute saving
            float dx = rainSpeed / 60.0f;
            float dy = 2 * rainSpeed / 60.0f;

            // update each droplet
            for (RainDroplet& droplet : droplets) {
                if (droplet.rainFrame == 0) {
                    // falling rain

                    droplet.x -= dx; 
                    droplet.y += dy; 

                    if (droplet.y > droplet.splash_y) {
                        droplet.rainFrame = shouldSplash ? 1 : 4; 
                        droplet.splash_start = millis(); 
                    }
                } else {
                    // splashing rain
                    uint32_t t = millis() - droplet.splash_start;
                    if (t > (
                        shouldSplash ? 150 * 3 : 150 * 5
                    )) { // 3 frames at 300ms each
                        // last phase of rain droplet
                        initRandRainDroplet(&droplet, false); 
                        continue; 
                    }

                    droplet.rainFrame = 
                        shouldSplash ?     
                        1 + (t / 150) % 3
                        : 4 + (t / 150) % 5; 
                }

            }
        }

        void render(Adafruit_GFX* tft) override {
            SpriteScreen::render(tft); 

            updateRain(); 
            for (RainDroplet& droplet : droplets) {
                drawSprite(tft, droplet.rainFrame, droplet.x, droplet.y, RAIN_SCALE); 
            }
        }
};