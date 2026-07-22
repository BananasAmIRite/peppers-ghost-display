#include "HeadphoneScreen.h"

#include <typeinfo>

#include "utils/text_utils.h"
#include "fonts/tiny512pt7b.h"

#include "types/device_screen.h"
#include "types/packets.h"

#define HEADPHONE_GRAPHIC_SIZE 64
#define HEADPHONE_GRAPHIC_SCALE 3

#define BATT_IND_SIZE 16
#define BATT_IND_SCALE 2
#define BATT_IND_PADDING_PX 10

#define BOSE_MODE_CIRC_SPACING 15
#define BOSE_MODE_CIRC_PAD_BOT 100
#define BOSE_MODE_CIRC_RAD 5
#define BOSE_MODE_CIRC_COLOR COLOR_WHITE

HeadphoneScreen::HeadphoneScreen() : SpriteScreen({
    "headphones_0_0.bmp", // 0
    "headphones_0_1.bmp", // 1
    "headphones_0_2.bmp", // 2
    "headphones_0_3.bmp", // 3
    "headphones_0_4.bmp", // 4
    "headphones_0_5.bmp", // 5
    "battery3.bmp", // 6 - 25%
    "battery2.bmp", // 7 - 50%
    "battery1.bmp", // 8 - 75%
    "battery0.bmp" // 9 - 100%
}) {
    // headphones.push_back(std::make_shared<BoseHeadphoneData>(1, 100, "Jason's Headphones", 3, 1)); 
}

void HeadphoneScreen::render(Adafruit_GFX* tft) {
    if (headphones.size() == 0) return; 
    if (cur_idx >= headphones.size()) cur_idx = headphones.size() - 1; 

    // first render headphone graphic
    std::shared_ptr<HeadphoneData> data = headphones[cur_idx]; 
    if (data->getType() == HeadphoneType::BOSE) {
        // render generic headphones
        uint8_t headphone_frame = ((millis()) / 250) % 6;  // headphone_0_0, headphone_0_1, ...
        drawSprite(tft, headphone_frame, 
            tft->width() / 2 + SCRN_OFFSET - HEADPHONE_GRAPHIC_SIZE * HEADPHONE_GRAPHIC_SCALE/ 2, tft->height() * 4 / 8 - HEADPHONE_GRAPHIC_SIZE * HEADPHONE_GRAPHIC_SCALE / 2, HEADPHONE_GRAPHIC_SCALE); 
    }



    // render battery indicator (text)
    tft->setFont(&Tiny5_Regular12pt7b);
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(1); 

    int16_t batt_ind_x = tft->width() / 2 + SCRN_OFFSET; 
    int16_t batt_ind_y = tft->height() * 6 / 8; 
    std::string battery_text = std::to_string(data->battery) + "%";
    int16_t bound_x, bound_y;
    uint16_t twidth, theight;  
    tft->getTextBounds(battery_text.c_str(), batt_ind_x, batt_ind_y, &bound_x, &bound_y, &twidth, &theight); 
    batt_ind_x = tft->width() / 2 + SCRN_OFFSET + (BATT_IND_SIZE * BATT_IND_SCALE + BATT_IND_PADDING_PX) / 2; // this is probably not correct, all vibes, need to recompute later
    drawCenteredText(tft, battery_text.c_str(), batt_ind_x, batt_ind_y); 

    // render battery icon
    // data.battery is [0, 100], div by 25 -> [0, 4] -> clamp to [0, 3] -> +6 for [6, 9]
    uint8_t batt_frame = 6 + min(max((int) data->battery / 25, 0), 3);  
    drawSprite(tft, batt_frame, tft->width() / 2 + SCRN_OFFSET - (twidth + BATT_IND_PADDING_PX) / 2 - BATT_IND_SIZE * BATT_IND_SCALE / 2, batt_ind_y - BATT_IND_SIZE * BATT_IND_SCALE / 2, BATT_IND_SCALE);

    
    if (data->getType() == HeadphoneType::BOSE) {
        std::shared_ptr<BoseHeadphoneData> bData = static_pointer_cast<BoseHeadphoneData>(data); 
        // render device name
        drawCenteredText(tft, bData->device_name.c_str(), tft->width() / 2 + SCRN_OFFSET, tft->height() * 6.75 / 8); 
        size_t numModes = bData->mode_cnt;  
        size_t curModeIdx = bData->cur_mode; 

        
        // draw dots at the bottom
        int16_t width = tft->width(); 
        int16_t height = tft->height();
        
        for (size_t i = 0; i < numModes; i++) {
            int16_t circleX = width / 2 + SCRN_OFFSET + (i - (((int16_t) numModes) - 1) / 2.0) * BOSE_MODE_CIRC_SPACING; 
            int16_t circleY = tft->height() * 7.5 / 8; 
            tft->drawCircle(circleX, circleY, BOSE_MODE_CIRC_RAD, BOSE_MODE_CIRC_COLOR); 
            if (curModeIdx == i) {
                tft->fillCircle(circleX, circleY, BOSE_MODE_CIRC_RAD, BOSE_MODE_CIRC_COLOR);
            }
        }
    }
}

void HeadphoneScreen::onUARTData(uint8_t type, uint8_t* data, uint8_t size, std::string name) {
    if (type != HEADPHONE_UPDATE) return; 



    if (size < 3) return; 
    uint8_t id = data[0]; 
    uint8_t state = data[1]; 
    HeadphoneType headphoneType = (HeadphoneType) data[2];

    // find matching headphones
    int matching_idx = -1; 
    for (int i = 0; i < headphones.size(); i++) if (headphones[i]->id == id) matching_idx = i; 

    // check state
    if (!state && matching_idx != -1) {
        // headphone is off, remove from list
        headphones.erase(headphones.begin() + matching_idx); 
        return; 
    }
    
    if (size < 4) return; // make sure we have enough data (battery)

    // headphones on
    // 0: id
    // 1: state (available/not available)
    // 2: headphone type (int)
    // 3: batt
    // 4+: headphone-specific data
    uint8_t batt = data[3]; 
    // no matching headphones, create new one
    if (headphoneType == HeadphoneType::BOSE) {
        // 4: mode len
        // 5: cur mode idx
        // 6: name len
        // 7+: name
        // read data for bose headphones

        uint8_t modeLen = data[4]; 
        uint8_t curModeIdx = data[5];
        uint8_t nameSize = data[6];  
        
        std::string hpName(
            reinterpret_cast<char*>(data + 7),
            nameSize
        );
        
        if (matching_idx == -1) {
            headphones.push_back(std::make_shared<BoseHeadphoneData>(id, batt, hpName, modeLen, curModeIdx)); 
        } else {
            headphones[matching_idx]->battery = batt; 

            if (headphones[matching_idx]->getType() == HeadphoneType::BOSE) {
                std::shared_ptr<BoseHeadphoneData> bh = static_pointer_cast<BoseHeadphoneData>(headphones[matching_idx]); 
                bh->cur_mode = curModeIdx; 
                bh->mode_cnt = modeLen; 
                bh->device_name = hpName; 
            }
        }
    }
}

void HeadphoneScreen::click(Adafruit_GFX* display, uint16_t x, uint16_t y) {
    cur_idx++; 
    cur_idx %= headphones.size(); 
}