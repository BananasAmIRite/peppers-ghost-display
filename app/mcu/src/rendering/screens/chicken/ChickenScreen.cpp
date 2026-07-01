#include "ChickenScreen.h"




void ChickenScreen::updateWalking()
{
    float dx = c.targetX - c.x;
    float dy = c.targetY - c.y;

    float dist = sqrt(dx*dx + dy*dy);

    if (dist < 1.0f)
    {
        c.state = CHICKEN_IDLE;
        c.stateStartTime = millis();
        return;
    }

    float speed = 15.0f; // pixels/sec

    float step = speed / 60.0f;

    c.x += dx / dist * step;
    c.y += dy / dist * step;


    if (abs(dx) > abs(dy))
    {
        c.direction = dx > 0 ? DIR_RIGHT : DIR_LEFT;
    }
    else
    {
        c.direction = dy > 0 ? DIR_DOWN : DIR_UP;
    }

    c.animFrame = (millis() / 150) % 3;
}

void ChickenScreen::updatePecking() {

    c.animFrame = (millis() / 150) % 4;

    if (c.animFrame == 3 && random(100) < 50) {

        c.state = CHICKEN_IDLE;
        c.stateStartTime = millis();
    } 
}

void ChickenScreen::updateSleeping() {
    c.animFrame = (millis() / 900) % 2;

    if (c.animFrame == 1 && random(100) < 2) {
        c.state = CHICKEN_IDLE; 
        c.stateStartTime = millis(); 
    }
}

size_t ChickenScreen::getPetEmoteSprite()
{
    uint32_t t = millis() - c.petStartTime;

    if (t < 300)
    {
        // fast burst (every ~75ms)
        return 28 + (t / 75) % 4;   // emoteheart0-3
    }
    else if (t < 1200)
    {
        // slower float (every ~225ms)
        return 32 + ((t - 300) / 225) % 4; // emoteheart20-23
    } else if (t < 1500) {
        return 28 + (3 - ((t - 1200) / 75)) % 4; // emoteheart3-0
    }

    c.isBeingPetted = false;
    return 0;
}


void ChickenScreen::runChickenFSM() {
    uint32_t now = millis();

    switch (c.state)
    {
        case CHICKEN_IDLE:

            if (now - c.stateStartTime > random(1000, 4000))
            {
                int r = random(100);

                if (r < 20)
                {
                    c.targetX = random(-6, 6);
                    c.targetY = random(-4, 4);

                    c.state = CHICKEN_WALKING;
                    c.stateStartTime = now;
                }
                else if (r < 30)
                {
                    c.state = CHICKEN_PECKING;
                    c.stateStartTime = now;
                } else if (r < 35) {
                    c.state = CHICKEN_SLEEPING; 
                    c.stateStartTime = now; 
                }

            }

            break;

        case CHICKEN_WALKING:
            updateWalking();
            break;

        case CHICKEN_PECKING:
            updatePecking();
            break;

        case CHICKEN_SLEEPING: 
            updateSleeping(); 
            break; 
    }

    
}

size_t ChickenScreen::getSpriteIndex()
{
    switch (c.state)
    {
        case CHICKEN_IDLE:
            switch (c.direction) // 1 frame
            {
                case DIR_DOWN: return 0; 
                case DIR_RIGHT: return 4;
                case DIR_UP: return 8; 
                case DIR_LEFT:  return 12;
            }

        case CHICKEN_WALKING: // 3 frames
            switch (c.direction)
            {
                case DIR_DOWN: return 1 + c.animFrame; 
                case DIR_RIGHT: return 5 + c.animFrame;
                case DIR_UP: return 9 + c.animFrame; 
                case DIR_LEFT:  return 13 + c.animFrame;
            }

        case CHICKEN_SLEEPING: // 2 frames
            switch (c.direction) {
                case DIR_DOWN: return 16 + c.animFrame; 
                case DIR_RIGHT: return 18 + c.animFrame;
                case DIR_UP: return 20 + c.animFrame; 
                case DIR_LEFT:  return 22 + c.animFrame;
            }

        case CHICKEN_PECKING:
            return 24 + c.animFrame; // 4 frames

    }

    return 0;
}



    ChickenScreen::ChickenScreen() : SpriteScreen({
        "/chicken0.bmp", 
        "/chicken1.bmp", 
        "/chicken2.bmp", 
        "/chicken3.bmp", 
        "/chicken4.bmp", 
        "/chicken5.bmp", 
        "/chicken6.bmp", 
        "/chicken7.bmp", 
        "/chicken8.bmp", 
        "/chicken9.bmp", 
        "/chicken10.bmp", 
        "/chicken11.bmp", 
        "/chicken12.bmp", 
        "/chicken13.bmp", 
        "/chicken14.bmp", 
        "/chicken15.bmp", 
        "/chicken16.bmp", 
        "/chicken17.bmp", 
        "/chicken18.bmp", 
        "/chicken19.bmp", 
        "/chicken20.bmp", 
        "/chicken21.bmp", 
        "/chicken22.bmp", 
        "/chicken23.bmp", 
        "/chicken24.bmp", 
        "/chicken25.bmp", 
        "/chicken26.bmp", 
        "/chicken27.bmp",  // idx 27
        "/emoteheart0.bmp", // idx 28
        "/emoteheart1.bmp", 
        "/emoteheart2.bmp", 
        "/emoteheart3.bmp", 
        "/emoteheart20.bmp", 
        "/emoteheart21.bmp", 
        "/emoteheart22.bmp", 
        "/emoteheart23.bmp", 
    }) {
        c.x = 0; 
        c.y = 0; 
        c.targetX = 0; 
        c.targetY = 0; 
        c.state = CHICKEN_IDLE; 
        c.direction = DIR_LEFT; 

        c.animFrame = 0; 
        
        c.stateStartTime = millis(); 
        c.lastAnimTime = millis(); 
    }


    void ChickenScreen::pet()
    {
        c.isBeingPetted = true;
        c.petStartTime = millis();
    }


    void ChickenScreen::render(Adafruit_GFX* tft) {
        SpriteScreen::render(tft); 
        runChickenFSM(); 
        drawSprite(
            tft,
            getSpriteIndex(),
            tft->width() / 2 + SCRN_OFFSET - SPRITE_SIZE * SPRITE_SCALE / 2 + c.x * SPRITE_POS_INC,
            tft->height() / 2 - SPRITE_SIZE * SPRITE_SCALE / 2 + c.y * SPRITE_POS_INC,
            SPRITE_SCALE
        );

        if (c.isBeingPetted) {
            size_t idx = getPetEmoteSprite(); 
            if (idx != 0) {
                drawSprite(tft, idx, 
                    tft->width() / 2 + SCRN_OFFSET - SPRITE_SIZE * SPRITE_SCALE / 2 + c.x * SPRITE_POS_INC, 
                    tft->height() / 2 - 3 * SPRITE_SIZE * SPRITE_SCALE / 2 + c.y * SPRITE_POS_INC, 
                    SPRITE_SCALE
                );
            }
        }
    }

void ChickenScreen::click(Adafruit_GFX* tft, uint16_t x, uint16_t y) {
    // compute where the chicken starts and ends
    int16_t startX = tft->width() / 2 + SCRN_OFFSET - SPRITE_SIZE * SPRITE_SCALE / 2 + c.x * SPRITE_POS_INC; 
    int16_t startY = tft->height() / 2 - SPRITE_SIZE * SPRITE_SCALE / 2 + c.y * SPRITE_POS_INC; 
    int16_t endX = startX + SPRITE_SIZE * SPRITE_SCALE; 
    int16_t endY = startY + SPRITE_SIZE * SPRITE_SCALE; 

    if (x >= startX && y >= startY && x < endX && y < endY) {
        pet(); 
    }

}