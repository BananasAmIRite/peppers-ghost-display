#pragma once

#include "rendering/screens/SpriteScreen.h"
#include <vector>
#include <string>
#include "Adafruit_GFX.h"
#include "data/SPIStream.h"
#include "data/UARTComms.h"

enum HeadphoneType {
    BOSE = 0x00
}; 

struct HeadphoneData {
    uint8_t id; 
    uint8_t battery; 
    virtual HeadphoneType getType() = 0; 

    HeadphoneData(uint8_t hId, uint8_t hBatt): id(hId), battery(hBatt) {
        
    }
};

struct BoseHeadphoneData : HeadphoneData {
    std::string device_name;
    uint8_t mode_cnt;
    uint8_t cur_mode;  
    HeadphoneType getType() override {return HeadphoneType::BOSE; }

    BoseHeadphoneData(uint8_t hId, uint8_t hBatt, std::string name, uint8_t mdcnt, uint8_t curmode): HeadphoneData(hId, hBatt), device_name(name), mode_cnt(mdcnt), cur_mode(curmode) {
        
    }
};


class HeadphoneScreen : public SpriteScreen, public UARTHandler {
    private:
        std::vector<std::shared_ptr<HeadphoneData>> headphones; 
        size_t cur_idx = 0; 

    public: 
        HeadphoneScreen();

        void render(Adafruit_GFX* tft) override;


        // handlers

        void onUARTData(uint8_t type, uint8_t* data, uint8_t size, std::string name) override;

        void click(Adafruit_GFX* display, uint16_t x, uint16_t y) override;

};