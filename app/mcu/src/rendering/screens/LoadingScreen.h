#pragma once

#include "../../utils/color_utils.h"
#include "../../utils/text_utils.h"
#include "../Screen.h"
#include "../../fonts/tiny518pt7b.h"
#include <array>

#define LOADING_STRINGS_SIZE 3

class LoadingScreen : public Renderable {
    private:
        int loadRefreshPeriod;
        std::array<const char*, LOADING_STRINGS_SIZE> loadingStrings = {"Loading.", "Loading..", "Loading..."};
        int currentStringIndex;
        int lastUpdateTime = -1; 
    public: 
        // refreshPeriod is in milliseconds
        LoadingScreen(int refreshPeriod): loadRefreshPeriod(refreshPeriod), currentStringIndex(0) {
            
        }

        void render(Adafruit_GFX* tft) override {
            int16_t width = tft->width(); 
            int16_t height = tft->height();
            tft->setFont(&Tiny5_Regular18pt7b);
            tft->setTextColor(COLOR_WHITE);
            tft->setTextSize(1); 
            drawCenteredText(tft, loadingStrings[currentStringIndex], width / 2, height / 2); 
            (loadingStrings[currentStringIndex]);

            if (millis() - lastUpdateTime > loadRefreshPeriod) {
                // Update the loading string index for the next render
                currentStringIndex = (currentStringIndex + 1) % LOADING_STRINGS_SIZE;
                lastUpdateTime = millis(); 
            }
        }
};