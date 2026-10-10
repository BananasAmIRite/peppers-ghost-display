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

#define STAT_IMG_SCALE 2

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

        bool device_connected;
        bool wifi_connected; 

        bool stat_visible = true; 

    public: 
        CursorScreen();

        CursorState* getCursor();

        
        void set_status(bool dev_conn, bool wifi_conn) {
            device_connected = dev_conn; 
            wifi_connected = wifi_conn; 
        }

        void set_stat_visible(bool visible) {
            stat_visible = visible; 
        }

        void render(Adafruit_GFX* tft) override;

        
        // link handlePacket to packet comm
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) override;
};