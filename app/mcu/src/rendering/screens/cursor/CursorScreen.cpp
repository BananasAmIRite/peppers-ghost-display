#include "CursorScreen.h"
#include "utils/text_utils.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include "fonts/tiny512pt7b.h"
#include "utils/color_utils.h"
#include "utils/sd_utils.h"


CursorScreen::CursorScreen() : SpriteScreen({
    "/cursor.bmp"
}) {

}

CursorState* CursorScreen::getCursor() {
    return &state;  
}

void CursorScreen::render(Adafruit_GFX* tft) {
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

    if (state.notifVisible) {

        tft->setFont(&Tiny5_Regular12pt7b);
        tft->setTextColor(COLOR_WHITE);
        tft->setTextSize(1); 

        std::string text = screen_to_text(state.notifScreen) + " ready. ";
        std::string text2 = "Swipe up to see"; 

        drawCenteredText(tft, text.c_str(), tft->width() / 2, tft->height() - 40);
        drawCenteredText(tft, text2.c_str(), tft->width() / 2, tft->height() - 20);

    }

}

        
// link handlePacket to packet comm
void CursorScreen::onUARTData(uint8_t type, uint8_t* data, uint8_t len) {
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

        
        case SET_SCREEN_NOTIF: {
            if (len < 2) return; 
            bool clear = data[0] & 0x01; 
            if (clear) {
                getCursor()->notifVisible = false; 
            } else {
                getCursor()->notifVisible = true; 
                getCursor()->notifScreen = (DeviceScreen) data[1];
            }
            break; 
        }
        default:
            break;
    }
}