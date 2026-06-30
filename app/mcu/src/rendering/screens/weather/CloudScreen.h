#pragma once

#include "rendering/screens/SpriteScreen.h"
#include <vector>


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


class CloudScreen : public SpriteScreen {
    private:
        std::vector<Cloud> clouds;
        CloudBoundingBox spawnBox;  // spawnbox should be left of despawn box (clouds move to the right)
        CloudBoundingBox despawnBox; 
        uint16_t minSpeed; 
        uint16_t maxSpeed; 

        void initRandomCloud(Cloud* cloud, bool start);

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

        
        void updateClouds();


        void render(Adafruit_GFX* tft) override;
};