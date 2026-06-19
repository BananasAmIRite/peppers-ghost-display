#pragma once

#include "../../../../utils/color_utils.h"
#include "../../../../utils/sd_utils.h"
#include "../../SpriteScreen.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>


struct Cloud {
    float x; 
    float y; 

    uint16_t speed; 
    int16_t despawnLoc; 
    size_t frameIdx; 
};

struct CloudBoundingBox {
    int16_t topleft_x; 
    int16_t topleft_y;
    int16_t bottomright_x;
    int16_t bottomright_y;
};

// haha get it? sunscreen
class CloudScreen : public SpriteScreen {
    private:
        std::vector<Cloud> clouds;
        CloudBoundingBox spawnBox;  // spawnbox should be left of despawn box (clouds move to the right)
        CloudBoundingBox despawnBox; 
        uint16_t minSpeed; 
        uint16_t maxSpeed; 

        void initRandomCloud(Cloud* cloud, bool start) {
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

    public: 
        CloudScreen(uint16_t min_cloud_speed = 60, uint16_t max_cloud_speed = 300, uint8_t numClouds = 8, CloudBoundingBox spawn = {-200, 0, -100, 50}, CloudBoundingBox despawn = {480, 0, 600, 50}) : SpriteScreen({
            "/cloud0.bmp",
            "/cloud1.bmp",
            "/cloud2.bmp"
        }), clouds(numClouds), minSpeed(min_cloud_speed), maxSpeed(max_cloud_speed), spawnBox(spawn), despawnBox(despawn) {
            // spawn in clouds
            for (int i = 0; i < numClouds; i++) {
                initRandomCloud(&clouds[i], true); 
            }
        }

        
        void updateClouds() {
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


        void render(Adafruit_GFX* tft) override {


            SpriteScreen::render(tft); 

            updateClouds(); 
            for (Cloud& cloud : clouds) {
                drawSprite(tft, cloud.frameIdx, cloud.x, cloud.y, 4); 
            }

        }
};