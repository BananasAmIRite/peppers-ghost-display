#pragma once

#include "utils/color_utils.h"
#include "utils/sd_utils.h"
#include "rendering/Screen.h"
#include "rendering/screens/SpriteScreen.h"
#include "data/UARTComms.h"
#include "packets.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

#define CURSOR_RECT_WIDTH 3*8
#define CURSOR_RECT_THICKNESS 8

#define CURSOR_RECT_WIDTH_CLICK 3*6
#define CURSOR_RECT_THICKNESS_CLICK 6

struct CursorState {
    uint16_t cursorX = 0; 
    uint16_t cursorY = 0; 
    bool cursorVisible = false; 
    bool mouseDown = false; 
};

class CursorScreen : public SpriteScreen, public UARTHandler {
    private:
        CursorState state; 

    public: 
        CursorScreen() : SpriteScreen({
            "/cursor.bmp"
        }) {

        }

        CursorState* getCursor() {
            return &state;  
        }

        void render(Adafruit_GFX* tft) override {
            SpriteScreen::render(tft); 
            if (state.cursorVisible) {
            // drawCurrentSprite(
            //     tft,
            //     0,
            //     - SPRITE_SIZE * SPRITE_SCALE / 2 + state->cursorX,
            //     - SPRITE_SIZE * SPRITE_SCALE / 2 + state->cursorY,
            //     SPRITE_SCALE
            // );
                uint16_t RECT_WIDTH = state.mouseDown ? CURSOR_RECT_WIDTH_CLICK : CURSOR_RECT_WIDTH; 
                uint16_t RECT_THICK = state.mouseDown ? CURSOR_RECT_THICKNESS_CLICK : CURSOR_RECT_THICKNESS; 

                tft->fillRect(state.cursorX - RECT_WIDTH / 2, state.cursorY - RECT_THICK / 2, RECT_WIDTH, RECT_THICK, COLOR_WHITE); 
                tft->fillRect(state.cursorX - RECT_THICK / 2, state.cursorY - RECT_WIDTH / 2, RECT_THICK, RECT_WIDTH, COLOR_WHITE);
                // if (state->mouseDown) {
                //     tft->fillRect(state->cursorX - CURSOR_RECT_THICKNESS / 2, state->cursorY - CURSOR_RECT_THICKNESS / 2, CURSOR_RECT_THICKNESS, CURSOR_RECT_THICKNESS, COLOR_BLACK); 
                // }
            }
        }

        
        // link handlePacket to packet comm
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len) override {
            switch (type) {
                case CURSOR_SET:
                    if (len < 4) return; 
                    getCursor()->cursorX = data[0] << 8 | data[1]; 
                    getCursor()->cursorY = data[2] << 8 | data[3]; 
                    break;
                case CURSOR_VISIBLE:
                    if (len < 1) return; 
                    getCursor()->cursorVisible = data[0] & 0x01; 
                    break; 
                case CURSOR_DOWN: 
                    getCursor()->mouseDown = true; 
                    break; 
                case CURSOR_UP:
                    getCursor()->mouseDown = false; 
                    break; 

                default:
                    break;
            }
        }
};