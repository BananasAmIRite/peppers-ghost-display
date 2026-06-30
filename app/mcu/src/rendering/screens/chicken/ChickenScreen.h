#pragma once

#include "Adafruit_GFX.h"
#include "rendering/screens/SpriteScreen.h"


enum ChickenState {
    CHICKEN_IDLE,
    CHICKEN_WALKING,
    CHICKEN_PECKING,
    CHICKEN_SLEEPING
};

enum ChickenDirection {
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP
};

struct Chicken {
    float x;
    float y;

    int16_t targetX;
    int16_t targetY;

    ChickenState state;
    ChickenDirection direction;

    uint8_t animFrame;

    uint32_t stateStartTime;
    uint32_t lastAnimTime;

    bool isBeingPetted; 
    uint32_t petStartTime;
};

class ChickenScreen : public SpriteScreen {
    private:
    static const uint8_t SPRITE_SCALE = 6; 
    static const uint16_t SPRITE_POS_INC = 16; 
    static const uint8_t SPRITE_SIZE = 16; 

    Chicken c;  



    void updateWalking();

    void updatePecking();

    void updateSleeping();

    size_t getPetEmoteSprite();

    
    void runChickenFSM();

    size_t getSpriteIndex();

    public: 
        ChickenScreen();


        void pet();


        void render(Adafruit_GFX* tft) override;

        void click(Adafruit_GFX* tft, uint16_t x, uint16_t y) override;
};