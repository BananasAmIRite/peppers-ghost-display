#pragma once

#include "utils/color_utils.h"
#include "utils/sd_utils.h"
#include "rendering/screens/SpriteScreen.h"
#include "fonts/tiny512pt7b.h"
#include "packets.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

#define SLIDER_WIDTH 32
#define SLIDER_HEIGHT 3
#define SCALE 4
#define BUTTON_WIDTH 5

#define IMG_SCALE 3

#define MAX_SONGNAME_WIDTH 12
#define SCROLL_DELAY_MS 600   // pause at each end before scrolling back
#define SCROLL_SPEED_MS 120   // ms per character step

struct SongState {
    std::string name; 
    uint16_t length_seconds; 
    uint16_t elapsed_seconds; 
    long last_synchronized_millis; 
};

class SpotifyScreen : public SpriteScreen, public SPIStreamHandler, public UARTHandler {
    private:

        std::shared_ptr<GFXcanvas16> curBuffer; 

        SongState curSong; 

        // scroll state
        int scrollOffset = 0;        // current char offset into the name
        int scrollDir    = 1;        // +1 = forward, -1 = backward
        uint32_t lastScrollTime = 0;
        bool scrollPaused = false;
        uint32_t pauseStart = 0;

        // returns the visible slice of the song name based on current offset
        std::string getVisibleName() const {
            const std::string& name = curSong.name;
            if ((int)name.size() <= MAX_SONGNAME_WIDTH)
                return name;
            return name.substr(scrollOffset, MAX_SONGNAME_WIDTH);
        }

        void updateScroll() {
            const std::string& name = curSong.name;
            if ((int)name.size() <= MAX_SONGNAME_WIDTH) {
                scrollOffset = 0;
                return;
            }

            uint32_t now = millis();
            int maxOffset = (int)name.size() - MAX_SONGNAME_WIDTH;

            if (scrollPaused) {
                if (now - pauseStart >= SCROLL_DELAY_MS) {
                    scrollPaused = false;
                    lastScrollTime = now;
                }
                return;
            }

            if (now - lastScrollTime < SCROLL_SPEED_MS) return;
            lastScrollTime = now;

            scrollOffset += scrollDir;

            if (scrollOffset >= maxOffset) {
                scrollOffset = maxOffset;
                scrollDir = -1;
                scrollPaused = true;
                pauseStart = now;
            } else if (scrollOffset <= 0) {
                scrollOffset = 0;
                scrollDir = 1;
                scrollPaused = true;
                pauseStart = now;
            }
        }

        void resetScroll() {
            scrollOffset  = 0;
            scrollDir     = 1;
            scrollPaused  = true;          // pause at start when song changes
            pauseStart    = millis();
            lastScrollTime = millis();
        }

    public: 
        SpotifyScreen() : SpriteScreen({"slider0.bmp", "slider1.bmp", "sliderbtn.bmp"}) {
            setSong("Not Playing...", 100);
            curSong.last_synchronized_millis = millis();
        }

        void loadBuffer(uint16_t width, uint16_t height, uint8_t* data, uint32_t data_size) {
            curBuffer = std::make_shared<GFXcanvas16>(width, height);
            memcpy(curBuffer->getBuffer(), data, data_size);  
        }

        void setSong(std::string name, uint16_t length_seconds) {
            curSong.name = name; 
            curSong.length_seconds = length_seconds;
            resetScroll();
        }

        void updateSongState(uint16_t elapsed_seconds) {
            curSong.elapsed_seconds = elapsed_seconds;
            curSong.last_synchronized_millis = millis(); 
        }

        void render(Adafruit_GFX* tft) override {
            // updateSongState(millis() / 1000);
            
            updateScroll();

            // song image
            if (curBuffer != nullptr) {
                drawScaledRGBBitmap(*tft, *curBuffer, tft->width() / 2 - curBuffer->width() * IMG_SCALE / 2, tft->height() * 3 / 8 - curBuffer->height() * IMG_SCALE / 2, IMG_SCALE); 
            }

            // (scrolling) text 
            tft->setFont(&Tiny5_Regular12pt7b);
            tft->setTextColor(COLOR_WHITE);
            tft->setTextSize(1); 

            drawCenteredText(tft, getVisibleName().c_str(), tft->width() / 2, tft->height() * 6 / 8); 


            // slider
            drawSprite(tft, 0, tft->width() / 2 - SLIDER_WIDTH * (SCALE + 1) / 2, tft->height() * 7 / 8 - SLIDER_HEIGHT * SCALE / 2, SCALE);
            drawSprite(tft, 1, tft->width() / 2 - SLIDER_WIDTH * (SCALE - 1) / 2, tft->height() * 7 / 8 - SLIDER_HEIGHT * SCALE / 2, SCALE);


            // slider head
            int left = tft->width() / 2 - SLIDER_WIDTH * (SCALE + 1) / 2; 
            int right = tft->width() / 2 - SLIDER_WIDTH * (SCALE - 1) / 2 + SLIDER_WIDTH * SCALE; 
            uint16_t trueElapsedSecs = curSong.elapsed_seconds + (millis() - curSong.last_synchronized_millis) / 1000;

            float progress = constrain(((float) trueElapsedSecs) / curSong.length_seconds, 0, 1);
            drawSprite(tft, 2, left + (right - left) * progress - BUTTON_WIDTH * SCALE / 2, tft->height() * 7 / 8 - BUTTON_WIDTH * SCALE / 2, SCALE); 
        }


        // handlers
        void onSPIData(uint8_t type, uint32_t size, uint8_t* data) override {
            
            LOGLN(type);
            if (type == SPOTIFY_SET_IMAGE) {
                if (size < 4) return; 

                uint16_t width = ((uint16_t)data[1] << 8) | data[0]; 
                uint16_t height = ((uint16_t)data[3] << 8) | data[2];

                LOG("WIDTH: ");
                LOG(width); 
                LOG(", HEIGHT: "); 
                LOGLN(height);

                if (size < 4 + width * height * 2) return; 

                LOGLN(length); 


                loadBuffer(width, height, data + 4, width * height * 2);


            }
        }

        void onUARTData(uint8_t type, uint8_t* data, uint8_t size) override {

            LOGLN(type);
            if (type == SPOTIFY_SET_SONG) {
                if (size < 6) return; 
                uint16_t nameSize;
                uint16_t duration;
                uint16_t progress;

                memcpy(&nameSize, data, 2);
                memcpy(&duration, data + 2, 2);
                memcpy(&progress, data + 4, 2);

                std::string songName(
                    reinterpret_cast<char*>(data + 6),
                    nameSize
                );

                setSong(songName, duration); 
                updateSongState(progress);
            } else if (type == SPOTIFY_UPDATE_SONG) {
                if (size < 2) return; 
                uint16_t progress;
                memcpy(&progress, data, 2);
                updateSongState(progress); 
            }
        }
};