#pragma once

#include "../../../../utils/color_utils.h"
#include "../../../../utils/sd_utils.h"
#include "../../SpriteScreen.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>



class SpotifyScreen : public Renderable {
    private:

        std::shared_ptr<GFXcanvas16> curBuffer; 

    public: 
        SpotifyScreen() : curBuffer(nullptr) {
            
        }

        void loadBuffer(uint16_t width, uint16_t height, uint8_t* data, uint32_t data_size) {
            curBuffer = std::make_shared<GFXcanvas16>(width, height);
            memcpy(curBuffer->getBuffer(), data, data_size);  
        }


        void render(Adafruit_GFX* tft) override {

            if (curBuffer != nullptr) drawScaledRGBBitmap(*tft, *curBuffer, 0, 0, 1); 

        }
};