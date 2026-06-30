#pragma once

#include "rendering/Screen.h"
#include "rendering/screens/SpriteScreen.h"
#include "data/UARTComms.h"
#include "types/packets.h"
#include "types/device_screen.h"


#define CURSOR_RECT_WIDTH 3*8
#define CURSOR_RECT_THICKNESS 8

#define CURSOR_RECT_WIDTH_CLICK 3*6
#define CURSOR_RECT_THICKNESS_CLICK 6

struct CursorState {
    uint16_t cursorX = 0; 
    uint16_t cursorY = 0; 
    bool cursorVisible = false; 
    bool mouseDown = false; 

    // screen notifications
    bool notifVisible = false; 
    DeviceScreen notifScreen; 
};

class CursorScreen : public SpriteScreen, public UARTHandler {
    private:
        CursorState state; 

    public: 
        CursorScreen();

        CursorState* getCursor();

        void render(Adafruit_GFX* tft) override;

        
        // link handlePacket to packet comm
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len) override;
};