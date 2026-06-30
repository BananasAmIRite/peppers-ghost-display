#include "RainScreen.h"



        
void RainScreen::initRandRainDroplet(RainDroplet* droplet, bool start) {
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



RainScreen::RainScreen(uint16_t rain_speed, uint8_t numDroplets, bool doSplash, RainBoundingBox spawn, RainBoundingBox splash) : SpriteScreen({
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

void RainScreen::updateRain() {

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

void RainScreen::render(Adafruit_GFX* tft) {
    SpriteScreen::render(tft); 

    updateRain(); 
    for (RainDroplet& droplet : droplets) {
        drawSprite(tft, droplet.rainFrame, droplet.x, droplet.y, RAIN_SCALE); 
    }
}