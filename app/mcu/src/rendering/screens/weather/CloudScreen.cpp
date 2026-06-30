#include "CloudScreen.h"


void CloudScreen::initRandomCloud(Cloud* cloud, bool start) {
    int16_t randX; 
    int16_t randY; 
    if (start) {
        randX = random(spawnBox.topleft_x, despawnBox.bottomright_x); 
        randY = random(spawnBox.topleft_y, despawnBox.bottomright_y); 
    } else {
        randX = random(spawnBox.topleft_x, spawnBox.bottomright_x); 
        randY = random(spawnBox.topleft_y, spawnBox.bottomright_y);
    }
    
    cloud->x = randX; 
    cloud->y = randY; 
    cloud->despawnLoc = random(despawnBox.topleft_x, despawnBox.bottomright_x); 
    cloud->speed = random(minSpeed, maxSpeed); 
    cloud->frameIdx = random(0, 2); 
}

        
void CloudScreen::updateClouds() {
    // update each droplet
    for (Cloud& cloud : clouds) {
        float dx = cloud.speed / 60.0f; 
        float dy = 0; 

        
        cloud.x += dx; 
        cloud.y += dy;

        if (cloud.x > cloud.despawnLoc) {
            // respawn cloud
            initRandomCloud(&cloud, false); 
        }

    }
}


void CloudScreen::render(Adafruit_GFX* tft) {


    SpriteScreen::render(tft); 

    updateClouds(); 
    for (Cloud& cloud : clouds) {
        drawSprite(tft, cloud.frameIdx, cloud.x, cloud.y, 4); 
    }

}